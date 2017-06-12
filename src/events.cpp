#include "events.h"

#include "app.h"

#include <jansson/jansson.h>

#include "helpers.h"
#include "cqcode.h"
#include "post_json.h"

using namespace std;

#define ENSURE_POST_NEEDED() if (CQ->config.post_url.length() == 0) { return EVENT_IGNORE; }
#define MATCH_PATTERN(msg) regex_search(msg, CQ->config.pattern)

int32_t event_private_msg(int32_t sub_type, int32_t send_time, int64_t from_qq, const str &msg, int32_t font) {
    ENSURE_POST_NEEDED();

    if (MATCH_PATTERN(msg.to_bytes())) {
        auto json = json_pack("{s:s, s:s, s:s, s:i, s:I, s:s}",
                              "post_type", "message",
                              "message_type", "private",
                              "sub_type", [&]() {
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
                              }(),
                              "time", send_time,
                              "user_id", from_qq,
                              "message", enhance_cq_code(msg, CQCODE_ENHANCE_INCOMING).c_str());
        auto response = post_json(json);
        json_decref(json);

        if (response.json) {
            auto reply_str_json = json_object_get(response.json, "reply");
            if (reply_str_json && !json_is_null(reply_str_json)) {
                CQ->sendPrivateMsg(from_qq, json_string_value(reply_str_json));
            }
            return release_response(response);
        }
    }
    return EVENT_IGNORE;
}

int32_t event_group_msg(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, const str &from_anonymous, const str &msg, int32_t font) {
    ENSURE_POST_NEEDED();

    if (MATCH_PATTERN(msg.to_bytes())) {
        str anonymous = "";
        auto is_anonymous = from_anonymous.length() > 0;
        auto json = json_pack("{s:s, s:s, s:i, s:I, s:I, s:s, s:s, s:s}",
                              "post_type", "message",
                              "message_type", "group",
                              "time", send_time,
                              "group_id", from_group,
                              "user_id", from_qq,
                              "anonymous", anonymous.c_str(),
                              "anonymous_flag", from_anonymous.c_str(),
                              "message", enhance_cq_code(msg, CQCODE_ENHANCE_INCOMING).c_str());
        auto response = post_json(json);
        json_decref(json);

        if (response.json) {
            auto reply_str_json = json_object_get(response.json, "reply");
            if (reply_str_json && !json_is_null(reply_str_json)) {
                str reply = json_string_value(reply_str_json);

                // check if should at sender
                auto at_sender_json = json_object_get(response.json, "at_sender");
                auto at_sender = true;
                if (json_is_boolean(at_sender_json) && json_is_false(at_sender_json)) {
                    at_sender = false;
                }
                if (at_sender && !is_anonymous) {
                    reply = "[CQ:at,qq=" + itos(from_qq) + "] " + reply;
                }

                // send reply
                CQ->sendGroupMsg(from_group, reply);
            }

            // kick sender if needed
            auto kick = json_is_true(json_object_get(response.json, "kick"));
            if (kick && !is_anonymous) {
                CQ->setGroupKick(from_group, from_qq, false);
            }

            // ban sender if needed
            auto ban = json_is_true(json_object_get(response.json, "ban"));
            if (ban) {
                if (is_anonymous) {
                    CQ->setGroupAnonymousBan(from_group, from_anonymous, 30 * 60);
                } else {
                    CQ->setGroupBan(from_group, from_qq, 30 * 60);
                }
            }

            return release_response(response);
        }
    }
    return EVENT_IGNORE;
}

int32_t event_discuss_msg(int32_t sub_Type, int32_t send_time, int64_t from_discuss, int64_t from_qq, const str &msg, int32_t font) {
    ENSURE_POST_NEEDED();

    if (MATCH_PATTERN(msg.to_bytes())) {
        auto json = json_pack("{s:s, s:s, s:i, s:I, s:I, s:s}",
                              "post_type", "message",
                              "message_type", "discuss",
                              "time", send_time,
                              "discuss_id", from_discuss,
                              "user_id", from_qq,
                              "message", enhance_cq_code(msg, CQCODE_ENHANCE_INCOMING).c_str());
        auto response = post_json(json);
        json_decref(json);

        if (response.json) {
            auto reply_str_json = json_object_get(response.json, "reply");
            if (reply_str_json && !json_is_null(reply_str_json)) {
                str reply = json_string_value(reply_str_json);

                // check if should at sender
                auto at_sender_json = json_object_get(response.json, "at_sender");
                auto at_sender = true;
                if (json_is_boolean(at_sender_json) && json_is_false(at_sender_json)) {
                    at_sender = false;
                }
                if (at_sender) {
                    reply = "[CQ:at,qq=" + itos(from_qq) + "] " + reply;
                }

                // send reply
                CQ->sendDiscussMsg(from_discuss, reply);
            }

            return release_response(response);
        }
    }
    return EVENT_IGNORE;
}

int32_t event_group_admin(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t being_operate_qq) {
    ENSURE_POST_NEEDED();

    auto json = json_pack("{s:s, s:s, s:s, s:i, s:I, s:I}",
                          "post_type", "event",
                          "event", "group_admin",
                          "sub_type", [&]() {
                              switch (sub_type) {
                              case 1:
                                  return "unset";
                              case 2:
                                  return "set";
                              default:
                                  return "unknown";
                              }
                          }(),
                          "time", send_time,
                          "group_id", from_group,
                          "user_id", being_operate_qq);
    auto response = post_json(json);
    json_decref(json);

    if (response.json) {
        return release_response(response);
    }

    return EVENT_IGNORE;
}

int32_t event_group_member_decrease(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, int64_t being_operate_qq) {
    ENSURE_POST_NEEDED();

    auto json = json_pack("{s:s, s:s, s:s, s:i, s:I, s:I, s:I}",
                          "post_type", "event",
                          "event", "group_decrease",
                          "sub_type", [&]() {
                              switch (sub_type) {
                              case 1:
                                  return "leave";
                              case 2:
                                  if (being_operate_qq != CQ->getLoginQQ()) {
                                      // the one been kicked out is not me
                                      return "kick";
                                  }
                              case 3:
                                  return "kick_me";
                              default:
                                  return "unknown";
                              }
                          }(),
                          "time", send_time,
                          "group_id", from_group,
                          "operator_id", sub_type == 1 ? being_operate_qq /* leave by him/herself */ : from_qq,
                          "user_id", being_operate_qq);
    auto response = post_json(json);
    json_decref(json);

    if (response.json) {
        return release_response(response);
    }

    return EVENT_IGNORE;
}

int32_t event_group_member_increase(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, int64_t being_operate_qq) {
    ENSURE_POST_NEEDED();

    auto json = json_pack("{s:s, s:s, s:s, s:i, s:I, s:I, s:I}",
                          "post_type", "event",
                          "event", "group_increase",
                          "sub_type", [&]() {
                              switch (sub_type) {
                              case 1:
                                  return "approve";
                              case 2:
                                  return "invite";
                              default:
                                  return "unknown";
                              }
                          }(),
                          "time", send_time,
                          "group_id", from_group,
                          "operator_id", from_qq,
                          "user_id", being_operate_qq);
    auto response = post_json(json);
    json_decref(json);

    if (response.json) {
        return release_response(response);
    }

    return EVENT_IGNORE;
}

int32_t event_friend_added(int32_t sub_type, int32_t send_time, int64_t from_qq) {
    ENSURE_POST_NEEDED();

    auto json = json_pack("{s:s, s:s, s:i, s:I}",
                          "post_type", "event",
                          "event", "friend_added",
                          "time", send_time,
                          "user_id", from_qq);
    auto response = post_json(json);
    json_decref(json);

    if (response.json) {
        return release_response(response);
    }

    return EVENT_IGNORE;
}

int32_t event_add_friend_request(int32_t sub_type, int32_t send_time, int64_t from_qq, const str &msg, const str &response_flag) {
    ENSURE_POST_NEEDED();

    auto json = json_pack("{s:s, s:s, s:i, s:I, s:s, s:s}",
                          "post_type", "request",
                          "request_type", "friend",
                          "time", send_time,
                          "user_id", from_qq,
                          "message", msg.c_str(),
                          "flag", response_flag.c_str());
    auto response = post_json(json);
    json_decref(json);

    if (response.json) {
        // approve or reject request if needed
        auto approve_json = json_object_get(response.json, "approve");
        if (json_is_boolean(approve_json)) {
            // the action is specified
            auto approve = json_boolean_value(approve_json);
            auto remark_str_json = json_object_get(response.json, "remark");
            if (remark_str_json && !json_is_null(remark_str_json)) {
                CQ->setFriendAddRequest(response_flag, approve ? REQUEST_ALLOW : REQUEST_DENY, json_string_value(remark_str_json));
            }
        }

        return release_response(response);
    }

    return EVENT_IGNORE;
}

int32_t event_add_group_request(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, const str &msg, const str &response_flag) {
    ENSURE_POST_NEEDED();

    auto json = json_pack("{s:s, s:s, s:s, s:i, s:I, s:I, s:s, s:s}",
                          "post_type", "request",
                          "request_type", "group",
                          "sub_type", [&]() {
                              switch (sub_type) {
                              case 1:
                                  return "add";
                              case 2:
                                  return "invite";
                              default:
                                  return "unknown";
                              }
                          }(),
                          "time", send_time,
                          "group_id", from_group,
                          "user_id", from_qq,
                          "message", msg.c_str(),
                          "flag", response_flag.c_str());
    auto response = post_json(json);
    json_decref(json);

    if (response.json) {
        // approve or reject request if needed
        auto approve_json = json_object_get(response.json, "approve");
        if (json_is_boolean(approve_json)) {
            // the action is specified
            auto approve = json_boolean_value(approve_json);
            auto reason_str_json = json_object_get(response.json, "reason");
            if (!reason_str_json || json_is_null(reason_str_json)) {
                reason_str_json = json_object_get(response.json, "remark"); // for compatibility with v1.1.3 and before
            }
            if (reason_str_json && !json_is_null(reason_str_json)) {
                CQ->setGroupAddRequestV2(response_flag, sub_type, approve ? REQUEST_ALLOW : REQUEST_DENY, json_string_value(reason_str_json));
            }
        }

        return release_response(response);
    }

    return EVENT_IGNORE;
}
