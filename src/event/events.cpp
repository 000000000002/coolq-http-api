// 
// events.cpp : Implement event handlers.
// 
// Copyright (C) 2017  Richard Chien <richardchienthebest@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 

#include "events.h"

#include "app.h"

#include "utils/rest_client.h"
#include "utils/params_class.h"
#include "utils/crypt.h"
#include "message/message_class.h"
#include "structs.h"
#include "service/hub_class.h"

using namespace std;

static const auto TAG = u8"�ϱ�";

#define ENSURE_POST_NEEDED \
    if (config.post_url.empty() && !ServiceHub::instance().has_pushable_services()) { \
        return CQEVENT_IGNORE; \
    }

static pplx::task<json> http_post(const json &json_body) {
    http_request request(http::methods::POST);
    request.headers().add(L"User-Agent", CQAPP_USER_AGENT);
    request.headers().add(L"Content-Type", L"application/json; charset=UTF-8");
    const auto body = json_body.dump();
    request.set_body(body);
    if (!config.secret.empty()) {
        request.headers().add(L"X-Signature", s2ws("sha1=" + hmac_sha1_hex(config.secret, body)));
    }
    return http_client(s2ws(config.post_url))
            .request(request)
            .then([](pplx::task<http_response> task) {
                auto next_task = pplx::task_from_result<string>("");
                try {
                    auto resp = task.get();
                    auto succeeded = false;
                    if (resp.status_code() < 300 && resp.status_code() >= 200) {
                        succeeded = true;
                        next_task = resp.extract_utf8string(true);
                    }
                    Log::d(TAG,
                           u8"ͨ�� HTTP �ϱ����ݵ� " + config.post_url + (succeeded ? u8" �ɹ�" : u8" ʧ��")
                           + u8"��״̬�룺" + to_string(resp.status_code()));
                } catch (http_exception &) {
                    // failed to request
                    Log::d(TAG, u8"HTTP �ϱ���ַ " + config.post_url + u8" �޷�����");
                }
                return next_task;
            })
            .then([](string &body) {
                if (!body.empty()) {
                    Log::d(TAG, u8"�յ���Ӧ " + body);
                    return pplx::task_from_result(json::parse(body)); // may throw invalid_argument due to invalid json
                }
                return pplx::task_from_result(json());
            });
}

static int32_t post_event(const json &payload, const function<void(const Params &)> response_handler = nullptr) {
    auto should_block = false;

    if (!config.post_url.empty()) {
        // do http post and handle response
        Log::d(TAG, u8"��ʼͨ�� HTTP �ϱ��¼�");
        try {
            if (const auto resp_payload = http_post(payload).get(); resp_payload.is_object()) {
                Params params;
                if (resp_payload.is_object()) {
                    params = Params(resp_payload);
                }

                // custom handler
                if (response_handler) response_handler(params);

                if (params.get_bool("block", false)) {
                    should_block = true;
                }
            }
        } catch (invalid_argument &) {
            // failed to parse json
            Log::d(TAG, u8"�ϱ���Ӧ������Ч�� JSON���Ѻ���");
        }
    }

    ServiceHub::instance().push_event(payload);

    return should_block ? CQEVENT_BLOCK : CQEVENT_IGNORE;
}

int32_t event_private_msg(int32_t sub_type, int32_t msg_id, int64_t from_qq, const string &msg, int32_t font) {
    ENSURE_POST_NEEDED;

    const auto sub_type_str = [&sub_type]() {
        switch (sub_type) {
        case 11:
            return "friend";
        case 1:
            return "other";
        case 2:
            return "group";
        case 3:
            return "discuss";
        default:
            return "unknown";
        }
    }();
    const json payload = {
        {"post_type", "message"},
        {"message_type", "private"},
        {"sub_type", sub_type_str},
        {"time", time(nullptr)},
        {"message_id", msg_id},
        {"user_id", from_qq},
        {"message", Message(msg).process_inward()},
        {"font", font}
    };

    return post_event(payload, [&](const Params &params) {
        const auto reply = params.get_message("reply");
        if (!reply.empty()) {
            sdk->send_private_msg(from_qq, reply);
        }
    });
}

int32_t event_group_msg(int32_t sub_type, int32_t msg_id, int64_t from_group, int64_t from_qq,
                        const string &from_anonymous, const string &msg, int32_t font) {
    ENSURE_POST_NEEDED;

    string anonymous;
    if (from_qq == 80000000 && !from_anonymous.empty()) {
        const auto anonymous_bin = base64_decode(from_anonymous);
        anonymous = Anonymous::from_bytes(anonymous_bin).name;
    }
    auto is_anonymous = !anonymous.empty();

    const auto sub_type_str = [&]() {
        if (from_qq == 80000000) {
            return "anonymous";
        }
        if (from_qq == 1000000) {
            return "notice";
        }
        if (sub_type == 1) {
            return "normal";
        }
        return "unknown";
    }();

    string final_msg;
    {
        auto prefix = "&#91;" + anonymous + "&#93;:";
        if (!anonymous.empty() && string_starts_with(msg, prefix)) {
            final_msg = msg.substr(prefix.length());
        } else {
            final_msg = move(msg);
        }
    }

    const json payload = {
        {"post_type", "message"},
        {"message_type", "group"},
        {"sub_type", sub_type_str},
        {"time", time(nullptr)},
        {"message_id", msg_id},
        {"group_id", from_group},
        {"user_id", from_qq},
        {"anonymous", anonymous},
        {"anonymous_flag", from_anonymous},
        {"message", Message(final_msg).process_inward()},
        {"font", font}
    };

    return post_event(payload, [&](const Params &params) {
        const auto reply = params.get_message("reply");
        if (!reply.empty()) {
            auto prefix = params.get_bool("at_sender", true) ? "[CQ:at,qq=" + to_string(from_qq) + "] " : "";
            sdk->send_group_msg(from_group, prefix + reply);
        }

        if (params.get_bool("delete", false)) {
            sdk->delete_msg(msg_id);
        }

        if (params.get_bool("kick", false) && !is_anonymous) {
            sdk->set_group_kick(from_group, from_qq, false);
        }

        if (params.get_bool("ban", false)) {
            const auto ban_duration = 30 * 60; // 30 minutes by default
            if (is_anonymous) {
                sdk->set_group_anonymous_ban(from_group, from_anonymous, ban_duration);
            } else {
                sdk->set_group_ban(from_group, from_qq, ban_duration);
            }
        }
    });
}

int32_t event_discuss_msg(int32_t sub_type, int32_t msg_id, int64_t from_discuss, int64_t from_qq, const string &msg,
                          int32_t font) {
    ENSURE_POST_NEEDED;

    const json payload = {
        {"post_type", "message"},
        {"message_type", "discuss"},
        {"time", time(nullptr)},
        {"message_id", msg_id},
        {"discuss_id", from_discuss},
        {"user_id", from_qq},
        {"message", Message(msg).process_inward()},
        {"font", font}
    };

    return post_event(payload, [&](const Params &params) {
        const auto reply = params.get_message("reply");
        if (!reply.empty()) {
            auto prefix = params.get_bool("at_sender", true) ? "[CQ:at,qq=" + to_string(from_qq) + "] " : "";
            sdk->send_discuss_msg(from_discuss, prefix + reply);
        }
    });
}

int32_t event_group_upload(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq,
                           const string &file) {
    ENSURE_POST_NEEDED;

    const auto file_bin = base64_decode(file);
    const json payload = {
        {"post_type", "event"},
        {"event", "group_upload"},
        {"time", send_time},
        {"group_id", from_group},
        {"user_id", from_qq},
        {"file", file_bin.size() >= GroupFile::MIN_SIZE ? GroupFile::from_bytes(file_bin).json() : nullptr}
    };

    return post_event(payload);
}

int32_t event_group_admin(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t being_operate_qq) {
    ENSURE_POST_NEEDED;

    const auto sub_type_str = [&sub_type]() {
        switch (sub_type) {
        case 1:
            return "unset";
        case 2:
            return "set";
        default:
            return "unknown";
        }
    }();
    const json payload = {
        {"post_type", "event"},
        {"event", "group_admin"},
        {"sub_type", sub_type_str},
        {"time", send_time},
        {"group_id", from_group},
        {"user_id", being_operate_qq}
    };

    return post_event(payload);
}

int32_t event_group_member_decrease(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq,
                                    int64_t being_operate_qq) {
    ENSURE_POST_NEEDED;

    const auto sub_type_str = [&sub_type, &being_operate_qq]() {
        switch (sub_type) {
        case 1:
            return "leave";
        case 2:
            if (being_operate_qq != sdk->get_login_qq()) {
                // the one been kicked out is not me
                return "kick";
            }
        case 3:
            return "kick_me";
        default:
            return "unknown";
        }
    }();
    const json payload = {
        {"post_type", "event"},
        {"event", "group_decrease"},
        {"sub_type", sub_type_str},
        {"time", send_time},
        {"group_id", from_group},
        {"operator_id", sub_type == 1 ? being_operate_qq /* leave by him/herself */ : from_qq},
        {"user_id", being_operate_qq}
    };

    return post_event(payload);
}

int32_t event_group_member_increase(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq,
                                    int64_t being_operate_qq) {
    ENSURE_POST_NEEDED;

    const auto sub_type_str = [&sub_type]() {
        switch (sub_type) {
        case 1:
            return "approve";
        case 2:
            return "invite";
        default:
            return "unknown";
        }
    }();
    const json payload = {
        {"post_type", "event"},
        {"event", "group_increase"},
        {"sub_type", sub_type_str},
        {"time", send_time},
        {"group_id", from_group},
        {"operator_id", from_qq},
        {"user_id", being_operate_qq}
    };

    return post_event(payload);
}

int32_t event_friend_add(int32_t sub_type, int32_t send_time, int64_t from_qq) {
    ENSURE_POST_NEEDED;

    const json payload = {
        {"post_type", "event"},
        {"event", "friend_add"},
        {"time", send_time},
        {"user_id", from_qq}
    };

    return post_event(payload);
}

int32_t event_add_friend_request(int32_t sub_type, int32_t send_time, int64_t from_qq, const string &msg,
                                 const string &response_flag) {
    ENSURE_POST_NEEDED;

    const json payload = {
        {"post_type", "request"},
        {"request_type", "friend"},
        {"time", send_time},
        {"user_id", from_qq},
        {"message", msg},
        {"flag", response_flag}
    };

    return post_event(payload, [&](const Params &params) {
        if (auto approve_opt = params.get<bool>("approve"); approve_opt) {
            auto approve = approve_opt.value();
            sdk->set_friend_add_request(response_flag, approve ? CQREQUEST_ALLOW : CQREQUEST_DENY,
                                        params.get_string("remark"));
        }
    });
}

int32_t event_add_group_request(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq,
                                const string &msg, const string &response_flag) {
    ENSURE_POST_NEEDED;

    const auto sub_type_str = [&sub_type]() {
        switch (sub_type) {
        case 1:
            return "add";
        case 2:
            return "invite";
        default:
            return "unknown";
        }
    }();
    const json payload = {
        {"post_type", "request"},
        {"request_type", "group"},
        {"sub_type", sub_type_str},
        {"time", send_time},
        {"group_id", from_group},
        {"user_id", from_qq},
        {"message", msg},
        {"flag", response_flag}
    };

    return post_event(payload, [&](const Params &params) {
        if (auto approve_opt = params.get<bool>("approve"); approve_opt) {
            auto approve = approve_opt.value();
            sdk->set_group_add_request(response_flag, sub_type, approve ? CQREQUEST_ALLOW : CQREQUEST_DENY,
                                       params.get_string("reason"));
        }
    });
}
