#include "request_handlers.h"

#include <jansson.h>
#include <cstdlib>
#include <algorithm>

#include "request.h"
#include "encoding.h"
#include "base64.h"

using namespace std;

extern int ac; // global AuthCode in appmain.cpp

CQHTTP_REQUEST_HANDLER(get_login_info)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t id = CQ_getLoginQQ(ac);
    const char *nickname = CQ_getLoginNick(ac);
    result.data = json_pack("{s:I,s:s?}", "id", id, "nickname", gbk_to_utf8(nickname).c_str());
    return result;
}

CQHTTP_REQUEST_HANDLER(send_private_msg)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t user_id = cqhttp_get_integer_param(request, "user_id", 0);
    char *msg = cqhttp_get_param(request, "message");
    if (user_id && msg)
        CQ_sendPrivateMsg(ac, user_id, utf8_to_gbk(msg).c_str());
    else
        result.status = CQHTTP_STATUS_FAILED;
    if (msg)
        free(msg);
    return result;
}

CQHTTP_REQUEST_HANDLER(send_group_msg)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    char *msg = cqhttp_get_param(request, "message");
    if (group_id && msg)
        CQ_sendGroupMsg(ac, group_id, utf8_to_gbk(msg).c_str());
    else
        result.status = CQHTTP_STATUS_FAILED;
    if (msg)
        free(msg);
    return result;
}

CQHTTP_REQUEST_HANDLER(send_discuss_msg)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t discuss_id = cqhttp_get_integer_param(request, "discuss_id", 0);
    char *msg = cqhttp_get_param(request, "message");
    if (discuss_id && msg)
        CQ_sendDiscussMsg(ac, discuss_id, utf8_to_gbk(msg).c_str());
    else
        result.status = CQHTTP_STATUS_FAILED;
    if (msg)
        free(msg);
    return result;
}

CQHTTP_REQUEST_HANDLER(send_like) // CoolQ Pro only
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t user_id = cqhttp_get_integer_param(request, "user_id", 0);
    if (user_id)
        CQ_sendLike(ac, user_id);
    else
        result.status = CQHTTP_STATUS_FAILED;
    return result;
}

CQHTTP_REQUEST_HANDLER(set_group_kick)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    int64_t user_id = cqhttp_get_integer_param(request, "user_id", 0);
    bool reject_add_request = cqhttp_get_bool_param(request, "reject_add_request", false);
    if (group_id && user_id)
        CQ_setGroupKick(ac, group_id, user_id, reject_add_request);
    else
        result.status = CQHTTP_STATUS_FAILED;
    return result;
}

CQHTTP_REQUEST_HANDLER(set_group_ban)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    int64_t user_id = cqhttp_get_integer_param(request, "user_id", 0);
    int64_t duration = cqhttp_get_integer_param(request, "duration", 30 * 60 /* 30 minutes */);
    if (group_id && user_id && duration >= 0)
        CQ_setGroupBan(ac, group_id, user_id, duration);
    else
        result.status = CQHTTP_STATUS_FAILED;
    return result;
}

CQHTTP_REQUEST_HANDLER(set_group_anonymous_ban)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    char *anonymous_flag = cqhttp_get_param(request, "flag");
    int64_t duration = cqhttp_get_integer_param(request, "duration", 30 * 60 /* 30 minutes */);
    if (group_id && anonymous_flag && duration >= 0)
        CQ_setGroupAnonymousBan(ac, group_id, utf8_to_gbk(anonymous_flag).c_str(), duration);
    else
        result.status = CQHTTP_STATUS_FAILED;
    if (anonymous_flag)
        free(anonymous_flag);
    return result;
}

CQHTTP_REQUEST_HANDLER(set_group_whole_ban)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    bool enable = cqhttp_get_bool_param(request, "enable", true);
    if (group_id)
        CQ_setGroupWholeBan(ac, group_id, enable);
    else
        result.status = CQHTTP_STATUS_FAILED;
    return result;
}

CQHTTP_REQUEST_HANDLER(set_group_admin)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    int64_t user_id = cqhttp_get_integer_param(request, "user_id", 0);
    bool enable = cqhttp_get_bool_param(request, "enable", true);
    if (group_id && user_id)
        CQ_setGroupAdmin(ac, group_id, user_id, enable);
    else
        result.status = CQHTTP_STATUS_FAILED;
    return result;
}

CQHTTP_REQUEST_HANDLER(set_group_anonymous) // CoolQ Pro only
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    bool enable = cqhttp_get_bool_param(request, "enable", true);
    if (group_id)
        CQ_setGroupAnonymous(ac, group_id, enable);
    else
        result.status = CQHTTP_STATUS_FAILED;
    return result;
}

CQHTTP_REQUEST_HANDLER(set_group_card)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    int64_t user_id = cqhttp_get_integer_param(request, "user_id", 0);
    char *card = cqhttp_get_param(request, "card");
    if (group_id && user_id)
        CQ_setGroupCard(ac, group_id, user_id, card ? utf8_to_gbk(card).c_str() : NULL);
    else
        result.status = CQHTTP_STATUS_FAILED;
    if (card)
        free(card);
    return result;
}

CQHTTP_REQUEST_HANDLER(set_group_leave)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    bool is_dismiss = cqhttp_get_bool_param(request, "is_dismiss", false);
    if (group_id)
        CQ_setGroupLeave(ac, group_id, is_dismiss);
    else
        result.status = CQHTTP_STATUS_FAILED;
    return result;
}

CQHTTP_REQUEST_HANDLER(set_group_special_title)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    int64_t user_id = cqhttp_get_integer_param(request, "user_id", 0);
    char *special_title = cqhttp_get_param(request, "special_title");
    int64_t duration = cqhttp_get_integer_param(request, "duration", -1 /* permanent */); // seems to be no effect
    if (group_id && user_id)
        CQ_setGroupSpecialTitle(ac, group_id, user_id, special_title ? utf8_to_gbk(special_title).c_str() : NULL, duration);
    else
        result.status = CQHTTP_STATUS_FAILED;
    if (special_title)
        free(special_title);
    return result;
}

CQHTTP_REQUEST_HANDLER(set_discuss_leave)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t discuss_id = cqhttp_get_integer_param(request, "discuss_id", 0);
    if (discuss_id)
        CQ_setDiscussLeave(ac, discuss_id);
    else
        result.status = CQHTTP_STATUS_FAILED;
    return result;
}

CQHTTP_REQUEST_HANDLER(set_friend_add_request)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    char *flag = cqhttp_get_param(request, "flag");
    bool approve = cqhttp_get_bool_param(request, "approve", true);
    char *remark = cqhttp_get_param(request, "remark");
    if (flag)
        CQ_setFriendAddRequest(ac,
                               utf8_to_gbk(flag).c_str(),
                               approve ? REQUEST_ALLOW : REQUEST_DENY,
                               remark ? utf8_to_gbk(remark).c_str() : NULL);
    else
        result.status = CQHTTP_STATUS_FAILED;
    if (flag)
        free(flag);
    if (remark)
        free(remark);
    return result;
}

CQHTTP_REQUEST_HANDLER(set_group_add_request)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    char *flag = cqhttp_get_param(request, "flag");
    char *type = cqhttp_get_param(request, "type");
    bool approve = cqhttp_get_bool_param(request, "approve", true);
    char *remark = cqhttp_get_param(request, "remark");
    int request_type = -1;
    if (strcmp(type, "add") == 0)
        request_type = REQUEST_GROUPADD;
    else if (strcmp(type, "invite") == 0)
        request_type = REQUEST_GROUPINVITE;
    if (flag && type && request_type != -1)
        CQ_setGroupAddRequestV2(ac,
                                utf8_to_gbk(flag).c_str(),
                                request_type,
                                approve ? REQUEST_ALLOW : REQUEST_DENY,
                                remark ? utf8_to_gbk(remark).c_str() : NULL);
    else
        result.status = CQHTTP_STATUS_FAILED;
    if (flag)
        free(flag);
    if (type)
        free(type);
    if (remark)
        free(remark);
    return result;
}

void get_integer_from_decoded_bytes(const string &bytes, size_t start, size_t size, void *dst)
{
    string sub = bytes.substr(start, size);
    reverse(sub.begin(), sub.end());
    memcpy(dst, sub.data(), size);
}

void get_string_from_decoded_bytes(const string &bytes, size_t start, size_t *size, string *dst)
{
    int16_t str_length = 0;
    get_integer_from_decoded_bytes(bytes, start, 2, &str_length);
    if (str_length == 0)
        *dst = "";
    else
        *dst = gbk_to_utf8(bytes.substr(start + 2, str_length).c_str());
    *size = 2 + str_length;
}

#define INIT(bytes_str)       \
    size_t _start = 0, _size; \
    string &_bytes = bytes_str;
#define INTEGER(field)                                             \
    _size = sizeof(field);                                         \
    get_integer_from_decoded_bytes(_bytes, _start, _size, &field); \
    _start += _size;
#define STRING(field)                                              \
    get_string_from_decoded_bytes(_bytes, _start, &_size, &field); \
    _start += _size;

struct raw_group_member_info
{
    int64_t group_id;
    int64_t user_id;
    string nickname; // all strings are converted to utf8
    string card;
    int32_t sex;
    int32_t age;
    string area;
    int32_t join_time;
    int32_t last_sent_time;
    string level;
    int32_t role;
    int32_t unfriendly; // unsure
    string title;
    int32_t title_expire_time;
    int32_t card_changeable;

    json_t *json()
    {
        json_t *data = json_object();
        json_object_set_new(data, "group_id", json_integer(group_id));
        json_object_set_new(data, "user_id", json_integer(user_id));
        json_object_set_new(data, "nickname", json_string(nickname.c_str()));
        json_object_set_new(data, "card", json_string(card.c_str()));
        json_object_set_new(data, "sex", json_string(sex == 0 ? "male" : (sex == 1 ? "female" : "unknown")));
        json_object_set_new(data, "age", json_integer(age));
        json_object_set_new(data, "area", json_string(area.c_str()));
        json_object_set_new(data, "join_time", json_integer(join_time));
        json_object_set_new(data, "last_sent_time", json_integer(last_sent_time));
        json_object_set_new(data, "level", json_string(level.c_str()));
        json_object_set_new(data, "role", json_string(role == 3 ? "owner" : (role == 2 ? "admin" : (role == 1 ? "member" : "unknown"))));
        json_object_set_new(data, "unfriendly", json_boolean(unfriendly));
        json_object_set_new(data, "title", json_string(title.c_str()));
        json_object_set_new(data, "title_expire_time", json_integer(title_expire_time));
        json_object_set_new(data, "card_changeable", json_boolean(card_changeable));
        return data;
    }
};

CQHTTP_REQUEST_HANDLER(get_group_member_info)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t group_id = cqhttp_get_integer_param(request, "group_id", 0);
    int64_t user_id = cqhttp_get_integer_param(request, "user_id", 0);
    bool no_cache = cqhttp_get_bool_param(request, "no_cache", false);
    if (group_id && user_id)
    {
        string bytes = base64_decode(gbk_to_utf8(CQ_getGroupMemberInfoV2(ac, group_id, user_id, no_cache)));
        if (bytes.size() >= 58 /* minimum valid bytes size */)
        {
            struct raw_group_member_info member_info;
            INIT(bytes);
            INTEGER(member_info.group_id);
            INTEGER(member_info.user_id);
            STRING(member_info.nickname);
            STRING(member_info.card);
            INTEGER(member_info.sex);
            INTEGER(member_info.age);
            STRING(member_info.area);
            INTEGER(member_info.join_time);
            INTEGER(member_info.last_sent_time);
            STRING(member_info.level);
            INTEGER(member_info.role);
            INTEGER(member_info.unfriendly);
            STRING(member_info.title);
            INTEGER(member_info.title_expire_time);
            INTEGER(member_info.card_changeable);
            result.data = member_info.json();
        }
        else
            result.status = CQHTTP_STATUS_FAILED;
    }
    else
        result.status = CQHTTP_STATUS_FAILED;
    return result;
}

struct raw_stranger_info
{
    int64_t user_id;
    string nickname; // utf8
    int32_t sex;
    int32_t age;

    json_t *json()
    {
        json_t *data = json_object();
        json_object_set_new(data, "user_id", json_integer(user_id));
        json_object_set_new(data, "nickname", json_string(nickname.c_str()));
        json_object_set_new(data, "sex", json_string(sex == 0 ? "male" : (sex == 1 ? "female" : "unknown")));
        json_object_set_new(data, "age", json_integer(age));
        return data;
    }
};

CQHTTP_REQUEST_HANDLER(get_stranger_info)
(const struct cqhttp_request &request)
{
    struct cqhttp_result result;
    int64_t user_id = cqhttp_get_integer_param(request, "user_id", 0);
    bool no_cache = cqhttp_get_bool_param(request, "no_cache", false);
    if (user_id)
    {
        string bytes = base64_decode(gbk_to_utf8(CQ_getStrangerInfo(ac, user_id, no_cache)));
        if (bytes.size() >= 18 /* minimum valid bytes size */)
        {
            struct raw_stranger_info stranger_info;
            INIT(bytes);
            INTEGER(stranger_info.user_id);
            STRING(stranger_info.nickname);
            INTEGER(stranger_info.sex);
            INTEGER(stranger_info.age);
            result.data = stranger_info.json();
        }
        else
            result.status = CQHTTP_STATUS_FAILED;
    }
    else
        result.status = CQHTTP_STATUS_FAILED;
    return result;
}

#undef INIT
#undef INTEGER
#undef STRING
