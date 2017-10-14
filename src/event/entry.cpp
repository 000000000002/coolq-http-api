//
// entry.cpp : Export event entry functions to DLL.
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

#include "app.h"

#include "conf/loader.h"
#include "event/events.h"

using namespace std;

/*
 * Return app info.
 */
CQEVENT(const char *, AppInfo, 0)
() {
    // CoolQ API version: 9
    return "9," CQAPP_ID;
}

/**
 * Get auth code.
 */
CQEVENT(int32_t, Initialize, 4)
(const int32_t auth_code) {
    sdk = Sdk(auth_code);
    return 0;
}

/**
 * Event: Plugin is enabled.
 */
CQEVENT(int32_t, __event_enable, 0)
() {
    static const auto TAG = u8"����";

    Log::d(TAG, CQAPP_FULLNAME);
    Log::d(TAG, u8"��ʼ��ʼ��");
    sdk->enabled = true;

    if (const auto config = load_configuration(sdk->get_app_directory() + "config.cfg")) {
        sdk->config = config.value();
    }

    //start_httpd();
    Log::i(TAG, u8"HTTP API ���������");

    //if (sdk->config.auto_check_update) {
    //    check_update(false);
    //}
    return 0;
}

/**
 * Event: Plugin is disabled.
 */
CQEVENT(int32_t, __event_disable, 0)
() {
    return 0;
}

/**
* Event: CoolQ is starting.
*/
CQEVENT(int32_t, __event_start, 0)
() {
    return 0;
}

/**
* Event: CoolQ is exiting.
*/
CQEVENT(int32_t, __event_exit, 0)
() {
    return 0;
}

/**
 * Type=21 ˽����Ϣ
 * sub_type �����ͣ�11/���Ժ��� 1/��������״̬ 2/����Ⱥ 3/����������
 */
CQEVENT(int32_t, __event_private_msg, 24)
(int32_t sub_type, int32_t send_time, int64_t from_qq, const char *msg, int32_t font) {
    return event_private_msg(sub_type, send_time, from_qq, string_decode(msg, Encodings::ANSI), font);
}

/**
 * Type=2 Ⱥ��Ϣ
 */
CQEVENT(int32_t, __event_group_msg, 36)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, const char *from_anonymous, const char *msg,
 int32_t font) {
    return event_group_msg(sub_type, send_time, from_group, from_qq, string_decode(from_anonymous, Encodings::ANSI),
                           string_decode(msg, Encodings::ANSI), font);
}

/**
 * Type=4 ��������Ϣ
 */
CQEVENT(int32_t, __event_discuss_msg, 32)
(int32_t sub_type, int32_t send_time, int64_t from_discuss, int64_t from_qq, const char *msg, int32_t font) {
    return event_discuss_msg(sub_type, send_time, from_discuss, from_qq, string_decode(msg, Encodings::ANSI), font);
}

/**
 * Type=11 Ⱥ�¼�-�ļ��ϴ�
 */
CQEVENT(int32_t, __event_group_upload, 28)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, const char *file) {
    return event_group_upload(sub_type, send_time, from_group, from_qq, string_decode(file, Encodings::ANSI));
}

/**
 * Type=101 Ⱥ�¼�-����Ա�䶯
 * sub_type �����ͣ�1/��ȡ������Ա 2/�����ù���Ա
 */
CQEVENT(int32_t, __event_group_admin, 24)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t being_operate_qq) {
    return event_group_admin(sub_type, send_time, from_group, being_operate_qq);
}

/**
 * Type=102 Ⱥ�¼�-Ⱥ��Ա����
 * sub_type �����ͣ�1/ȺԱ�뿪 2/ȺԱ���� 3/�Լ�(����¼��)����
 * from_qq ������QQ(��subTypeΪ2��3ʱ����)
 * being_operate_qq ������QQ
 */
CQEVENT(int32_t, __event_group_member_decrease, 32)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, int64_t being_operate_qq) {
    return event_group_member_decrease(sub_type, send_time, from_group, from_qq, being_operate_qq);
}

/**
 * Type=103 Ⱥ�¼�-Ⱥ��Ա����
 * sub_type �����ͣ�1/����Ա��ͬ�� 2/����Ա����
 * from_qq ������QQ(������ԱQQ)
 * being_operate_qq ������QQ(����Ⱥ��QQ)
 */
CQEVENT(int32_t, __event_group_member_increase, 32)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, int64_t being_operate_qq) {
    return event_group_member_increase(sub_type, send_time, from_group, from_qq, being_operate_qq);
}

/**
 * Type=201 �����¼�-���������
 */
CQEVENT(int32_t, __event_friend_add, 16)
(int32_t sub_type, int32_t send_time, int64_t from_qq) {
    return event_friend_add(sub_type, send_time, from_qq);
}

/**
 * Type=301 ����-�������
 * msg ����
 * response_flag ������ʶ(����������)
 */
CQEVENT(int32_t, __event_add_friend_request, 24)
(int32_t sub_type, int32_t send_time, int64_t from_qq, const char *msg, const char *response_flag) {
    return event_add_friend_request(sub_type, send_time, from_qq, string_decode(msg, Encodings::ANSI),
                                    string_decode(response_flag, Encodings::ANSI));
}

/**
 * Type=302 ����-Ⱥ���
 * sub_type �����ͣ�1/����������Ⱥ 2/�Լ�(����¼��)������Ⱥ
 * msg ����
 * response_flag ������ʶ(����������)
 */
CQEVENT(int32_t, __event_add_group_request, 32)
(int32_t sub_type, int32_t send_time, int64_t from_group, int64_t from_qq, const char *msg, const char *response_flag) {
    return event_add_group_request(sub_type, send_time, from_group, from_qq, string_decode(msg, Encodings::ANSI),
                                   string_decode(response_flag, Encodings::ANSI));
}

/**
 * �����²˵���
 */
CQEVENT(int32_t, __menu_check_update, 0)() {
    //check_update(true);
    return 0;
}
