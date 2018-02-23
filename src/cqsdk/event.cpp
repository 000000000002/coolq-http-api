#include "./event.h"

#include "./def.h"
#include "./utils/string.h"
#include "./utils/function.h"

namespace cq::event {
    std::function<Operation(const PrivateMessageEvent &)> on_private_msg;
    std::function<Operation(const GroupMessageEvent &)> on_group_msg;
    std::function<Operation(const DiscussMessageEvent &)> on_discuss_msg;
    std::function<Operation(const GroupUploadEvent &)> on_group_upload;
    std::function<Operation(const GroupAdminEvent &)> on_group_admin;
    std::function<Operation(const GroupMemberDecreaseEvent &)> on_group_member_decrease;
    std::function<Operation(const GroupMemberIncreaseEvent &)> on_group_member_increase;
    std::function<Operation(const FriendAddEvent &)> on_friend_add;
    std::function<Operation(const FriendRequestEvent &)> on_friend_request;
    std::function<Operation(const GroupRequestEvent &)> on_group_request;
}

using namespace std;
using namespace cq;
using cq::utils::call_if_valid;
using cq::utils::string_from_coolq;

/**
 * Type=21 ˽����Ϣ
 * sub_type �����ͣ�11/���Ժ��� 1/��������״̬ 2/����Ⱥ 3/����������
 */
CQEVENT(int32_t, cq_event_private_msg, 24)
(int32_t sub_type, int32_t msg_id, int64_t from_qq, const char *msg, int32_t font) {
    event::PrivateMessageEvent e;
    e.target = Target(from_qq);
    e.sub_type = static_cast<message::SubType>(sub_type);
    e.message_id = msg_id;
    e.message = string_from_coolq(msg);
    e.font = font;
    e.user_id = from_qq;
    return call_if_valid(event::on_private_msg, e);
}

/**
 * Type=2 Ⱥ��Ϣ
 */
CQEVENT(int32_t, cq_event_group_msg, 36)
(int32_t sub_type, int32_t msg_id, int64_t from_group, int64_t from_qq, const char *from_anonymous, const char *msg,
 int32_t font) {
    event::GroupMessageEvent e;
    e.target = Target(from_qq, from_group, Target::GROUP);
    e.sub_type = static_cast<message::SubType>(sub_type);
    e.message_id = msg_id;
    e.message = string_from_coolq(msg);
    e.font = font;
    e.user_id = from_qq;
    e.group_id = from_group;
    e.anonymous = string_from_coolq(from_anonymous);
    return call_if_valid(event::on_group_msg, e);
}

/**
 * Type=4 ��������Ϣ
 */
CQEVENT(int32_t, cq_event_discuss_msg, 32)
(int32_t sub_type, int32_t msg_id, int64_t from_discuss, int64_t from_qq, const char *msg, int32_t font) {
    event::DiscussMessageEvent e;
    e.target = Target(from_qq, from_discuss, Target::DISCUSS);
    e.sub_type = static_cast<message::SubType>(sub_type);
    e.message_id = msg_id;
    e.message = string_from_coolq(msg);
    e.font = font;
    e.user_id = from_qq;
    e.discuss_id = from_discuss;
    return call_if_valid(event::on_discuss_msg, e);
}

/**
 * Type=11 Ⱥ�¼�-�ļ��ϴ�
 */
CQEVENT(int32_t, cq_event_group_upload, 28)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, const char *file) {
    event::GroupUploadEvent e;
    e.target = Target(from_qq, from_group, Target::GROUP);
    e.time = send_time;
    e.sub_type = static_cast<notice::SubType>(sub_type);
    e.file = string_from_coolq(file);
    e.user_id = from_qq;
    e.group_id = from_group;
    return call_if_valid(event::on_group_upload, e);
}

/**
 * Type=101 Ⱥ�¼�-����Ա�䶯
 * sub_type �����ͣ�1/��ȡ������Ա 2/�����ù���Ա
 */
CQEVENT(int32_t, cq_event_group_admin, 24)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t being_operate_qq) {
    event::GroupAdminEvent e;
    e.target = Target(being_operate_qq, from_group, Target::GROUP);
    e.time = send_time;
    e.sub_type = static_cast<notice::SubType>(sub_type);
    e.user_id = being_operate_qq;
    e.group_id = from_group;
    return call_if_valid(event::on_group_admin, e);
}

/**
 * Type=102 Ⱥ�¼�-Ⱥ��Ա����
 * sub_type �����ͣ�1/ȺԱ�뿪 2/ȺԱ���� 3/�Լ�(����¼��)����
 * from_qq ������QQ(��subTypeΪ2��3ʱ����)
 * being_operate_qq ������QQ
 */
CQEVENT(int32_t, cq_event_group_member_decrease, 32)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, int64_t being_operate_qq) {
    event::GroupMemberDecreaseEvent e;
    e.target = Target(being_operate_qq, from_group, Target::GROUP);
    e.time = send_time;
    e.sub_type = static_cast<notice::SubType>(sub_type);
    e.user_id = being_operate_qq;
    e.group_id = from_group;
    e.operator_id = e.sub_type == notice::GROUP_MEMBER_DECREASE_LEAVE ? being_operate_qq : from_qq;
    return call_if_valid(event::on_group_member_decrease, e);
}

/**
 * Type=103 Ⱥ�¼�-Ⱥ��Ա����
 * sub_type �����ͣ�1/����Ա��ͬ�� 2/����Ա����
 * from_qq ������QQ(������ԱQQ)
 * being_operate_qq ������QQ(����Ⱥ��QQ)
 */
CQEVENT(int32_t, cq_event_group_member_increase, 32)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, int64_t being_operate_qq) {
    event::GroupMemberIncreaseEvent e;
    e.target = Target(being_operate_qq, from_group, Target::GROUP);
    e.time = send_time;
    e.sub_type = static_cast<notice::SubType>(sub_type);
    e.user_id = being_operate_qq;
    e.group_id = from_group;
    e.operator_id = from_qq;
    return call_if_valid(event::on_group_member_increase, e);
}

/**
 * Type=201 �����¼�-���������
 */
CQEVENT(int32_t, cq_event_friend_add, 16)
(int32_t sub_type, int32_t send_time, int64_t from_qq) {
    event::FriendAddEvent e;
    e.target = Target(from_qq);
    e.time = send_time;
    e.sub_type = static_cast<notice::SubType>(sub_type);
    e.user_id = from_qq;
    return call_if_valid(event::on_friend_add, e);
}

/**
 * Type=301 ����-�������
 * msg ����
 * response_flag ������ʶ(����������)
 */
CQEVENT(int32_t, cq_event_add_friend_request, 24)
(int32_t sub_type, int32_t send_time, int64_t from_qq, const char *msg, const char *response_flag) {
    event::FriendRequestEvent e;
    e.target = Target(from_qq);
    e.time = send_time;
    e.sub_type = static_cast<request::SubType>(sub_type);
    e.comment = string_from_coolq(msg);
    e.flag = string_from_coolq(response_flag);
    e.user_id = from_qq;
    return call_if_valid(event::on_friend_request, e);
}

/**
 * Type=302 ����-Ⱥ���
 * sub_type �����ͣ�1/����������Ⱥ 2/�Լ�(����¼��)������Ⱥ
 * msg ����
 * response_flag ������ʶ(����������)
 */
CQEVENT(int32_t, cq_event_add_group_request, 32)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, const char *msg, const char *response_flag) {
    event::GroupRequestEvent e;
    e.target = Target(from_qq, from_group, Target::GROUP);
    e.time = send_time;
    e.sub_type = static_cast<request::SubType>(sub_type);
    e.comment = string_from_coolq(msg);
    e.flag = string_from_coolq(response_flag);
    e.user_id = from_qq;
    e.group_id = from_group;
    return call_if_valid(event::on_group_request, e);
}
