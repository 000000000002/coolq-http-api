#pragma once

#include "common.h"

int32_t event_private_msg(int32_t sub_type, int32_t send_time, int64_t from_qq, const str &msg, int32_t font);
int32_t event_group_msg(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, const str &from_anonymous, const str &msg, int32_t font);
int32_t event_discuss_msg(int32_t sub_Type, int32_t send_time, int64_t from_discuss, int64_t from_qq, const str &msg, int32_t font);
int32_t event_group_admin(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t being_operate_qq);
int32_t event_group_member_decrease(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, int64_t being_operate_qq);
int32_t event_group_member_increase(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, int64_t being_operate_qq);
int32_t event_friend_added(int32_t sub_type, int32_t send_time, int64_t from_qq);
int32_t event_add_friend_request(int32_t sub_type, int32_t send_time, int64_t from_qq, const str &msg, const str &response_flag);
int32_t event_add_group_request(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, const str &msg, const str &response_flag);
