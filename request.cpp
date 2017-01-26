#include "request.h"

#include <string>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <jansson.h>

#include "misc_functions.h"

using namespace std;

struct cqhttp_result dispatch_request(const struct cqhttp_request &request);

void cqhttp_main_handler(struct evhttp_request *req, void *_)
{
    enum evhttp_cmd_type method = evhttp_request_get_command(req);
    if (!(method & (EVHTTP_REQ_GET | EVHTTP_REQ_POST)))
    {
        // method not supported
        evhttp_send_reply(req, HTTP_BADMETHOD, "Bad Method", NULL);
        return;
    }

    // initialize args and form
    size_t evkeyvalq_size = sizeof(struct evkeyvalq);
    struct evkeyvalq *args = (struct evkeyvalq *)malloc(evkeyvalq_size);
    memset(args, 0, evkeyvalq_size);
    struct evkeyvalq *form = (struct evkeyvalq *)malloc(evkeyvalq_size);
    memset(form, 0, evkeyvalq_size);

    struct cqhttp_request request;
    request.args = args;
    request.form = form;

    const struct evhttp_uri *uri = evhttp_request_get_evhttp_uri(req);
    request.path = evhttp_uri_get_path(uri);

    LOG_D("HTTP����", string("�յ� HTTP ��������ʽ��") + (method == EVHTTP_REQ_GET ? "GET" : "POST") + "��·����" + request.path);

    // parse query arguments
    const char *encoded_args_str = evhttp_uri_get_query(uri);
    evhttp_parse_query_str(encoded_args_str ? encoded_args_str : "", args);

    if (method == EVHTTP_REQ_POST)
    {
        // check content type
        struct evkeyvalq *input_headers = evhttp_request_get_input_headers(req);
        if (string(evhttp_find_header(input_headers, "Content-Type")) == "application/x-www-form-urlencoded")
        {
            // read form data as string
            struct evbuffer *input_buffer = evhttp_request_get_input_buffer(req);
            size_t data_length = evbuffer_get_length(input_buffer);
            char *encoded_form_data_str;
            if (data_length > 0)
            {
                encoded_form_data_str = (char *)malloc(data_length + 1);
                memcpy(encoded_form_data_str, evbuffer_pullup(input_buffer, -1), data_length);
                encoded_form_data_str[data_length] = '\0';
            }
            else
            {
                encoded_form_data_str = (char *)malloc(1);
                encoded_form_data_str[0] = '\0'; // an empty string on heap
            }

            // parse form
            evhttp_parse_query_str(encoded_form_data_str, form);
            free(encoded_form_data_str);
        }
    }

    // dispatch
    struct cqhttp_result result = dispatch_request(request);

    // clear args and form
    evhttp_clear_headers(args);
    free(args);
    evhttp_clear_headers(form);
    free(form);

    // set headers
    struct evkeyvalq *output_headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_headers, "Server", "CoolQ HTTP API Plugin v1.0.0");
    evhttp_add_header(output_headers, "Content-Type", "application/json; charset=UTF-8");
    evhttp_add_header(output_headers, "Connection", "close");

    // write response
    struct evbuffer *buf = evbuffer_new();
    json_t *json = json_pack("{s:s,s:o?}", "status", cqhttp_status_strings[result.status], "data", result.data);
    char *json_str = json_dumps(json, JSON_COMPACT);
    json_decref(json);
    evbuffer_add_printf(buf, "%s", json_str);
    free(json_str);
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
    LOG_D("HTTP����", "��Ӧ���ݷ������");
}

char *cqhttp_get_param(const struct cqhttp_request &request, const char *key)
{
    char *value = NULL;
    const char *encoded_value = evhttp_find_header(request.args, key);
    if (!encoded_value)
    {
        encoded_value = evhttp_find_header(request.form, key);
    }
    if (encoded_value)
    {
        value = evhttp_uridecode(encoded_value, 0, NULL);
    }
    return value;
}

int64_t cqhttp_get_integer_param(const struct cqhttp_request &request, const char *key, int64_t default_val)
{
    char *str = cqhttp_get_param(request, key);
    int64_t result = default_val;
    if (str)
    {
        if (isnumber(str))
            result = atol(str);
        free(str);
    }
    return result;
}

bool cqhttp_get_bool_param(const struct cqhttp_request &request, const char *key, bool default_val)
{
    char *str = cqhttp_get_param(request, key);
    bool result = default_val;
    if (str)
    {
        if (string("true") == str || string("1") == str)
            result = true;
        else if (string("false") == str || string("0") == str)
            result = false;
        free(str);
    }
    return result;
}

cqhttp_request_handler_map request_handler_map;

struct cqhttp_result dispatch_request(const struct cqhttp_request &request)
{
    const char *path_without_slash = &request.path[1];
    if (request_handler_map.find(path_without_slash) != request_handler_map.end())
    {
        // the corresponding handler exists
        LOG_D("HTTP����", string("�ҵ� handler��") + path_without_slash);
        return request_handler_map[path_without_slash](request);
    }
    else
    {
        // the corresponding handler does not exist
        LOG_D("HTTP����", string("û���ҵ� handler��") + path_without_slash);
        struct cqhttp_result result;
        result.status = CQHTTP_STATUS_FAILED;
        return result;
    }
}
