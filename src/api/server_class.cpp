#include "./server_class.h"

#include "app.h"

#include "types.h"

using namespace std;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using Response = HttpServer::Response;
using Request = HttpServer::Request;

extern ApiHandlerMap api_handlers;

class ResponseHelper {
public:
    unsigned short status_code;
    bytes body;
    SimpleWeb::CaseInsensitiveMultimap headers;

    void write(const shared_ptr<Response> response) {
        *response << "HTTP/1.1 " << SimpleWeb::status_code(SimpleWeb::StatusCode(status_code)) << "\r\n";
        headers.erase("Content-Length");
        headers.emplace("Content-Length", to_string(body.size()));
        for (const auto &kv : headers) {
            *response << kv.first << ": " << kv.second << "\r\n";
        }
        *response << "\r\n" << body;
    }
};

static const auto TAG = u8"API����";

void ApiServer::init() {
    Log::d(TAG, u8"��ʼ�� API ������");

    server_.default_resource["GET"]
            = server_.default_resource["POST"]
            = [](shared_ptr<Response> response, shared_ptr<Request> request) {
                ResponseHelper{404 /* Not Found */}.write(response);
            };

    for (const auto &handler_kv : api_handlers) {
        const auto path_regex = "^/" + handler_kv.first + "$";
        server_.resource[path_regex]["GET"] = server_.resource[path_regex]["POST"]
                = [&handler_kv](shared_ptr<Response> response, shared_ptr<Request> request) {
                    Log::d(TAG, u8"�յ� API ����" + request->method
                           + u8" " + request->path + u8"?" + request->query_string);

                    auto json_params = json::object();
                    json args = request->parse_query_string(), form;

                    if (request->method == "POST") {
                        string content_type;
                        if (const auto it = request->header.find("Content-Type");
                            it != request->header.end()) {
                            content_type = it->second;
                            Log::d(TAG, u8"Content-Type: " + content_type);
                        }

                        if (content_type == "application/x-www-form-urlencoded") {
                            form = SimpleWeb::QueryString::parse(request->content.string());
                        } else if (content_type == "application/json") {
                            try {
                                json_params = json::parse(request->content.string()); // may throw invalid_argument
                                if (!json_params.is_object()) {
                                    throw invalid_argument("must be a JSON object");
                                }
                            } catch (invalid_argument &) {
                                Log::d(TAG, u8"HTTP ���ĵ� JSON ��Ч���߲��Ƕ���");
                                ResponseHelper{400 /* Bad Request */}.write(response);
                                return;
                            }
                        } else {
                            Log::d(TAG, u8"Content-Type ��֧��");
                            ResponseHelper{406 /* Not Acceptable */}.write(response);
                            return;
                        }
                    }

                    // merge form and args to json params
                    for (auto data : {form, args}) {
                        if (data.is_object()) {
                            for (auto it = data.begin(); it != data.end(); ++it) {
                                json_params[it.key()] = it.value();
                            }
                        }
                    }

                    Log::d(TAG, u8"API ������ " + handler_kv.first + u8" ��ʼ��������");
                    ApiResult result;
                    ApiParams params(json_params);
                    handler_kv.second(params, result);

                    decltype(request->header) headers{
                        {"Server", CQAPP_SERVER},
                        {"Content-Type", "application/json; charset=UTF-8"}
                    };
                    auto resp_body = result.json().dump();
                    Log::d(TAG, u8"��Ӧ������׼����ϣ�" + resp_body);
                    response->write(resp_body, headers);
                    Log::d(TAG, u8"��Ӧ�����ѷ���");
                    Log::i(TAG, u8"�ѳɹ�����һ�� API ����" + request->path);
                };
    }

    initiated_ = true;
}

void ApiServer::start(const string &host, const unsigned short port) {
    server_.config.address = host;
    server_.config.port = port;
    thread_ = std::thread([&]() {
        server_.start();
    });
    Log::d(TAG, u8"���� API ����ɹ�����ʼ���� http://" + host + ":" + to_string(port));
}

void ApiServer::stop() {
    server_.stop();
    if (thread_.joinable()) {
        thread_.join();
    }
    Log::d(TAG, u8"�ѹر� API ����");
}
