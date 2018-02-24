#include "cqsdk/cqsdk.h"

namespace app = cq::app;
namespace event = cq::event;
namespace api = cq::api;
namespace logging = cq::logging;
namespace message = cq::message;

// ��ʼ�� App ID
CQ_INITIALIZE("io.github.richardchien.coolqhttpapi");

// �����ڣ��ھ�̬��Ա��ʼ��֮��app::on_initialize �¼�����֮ǰ��ִ�У��������� SDK ��ע���¼��ص�
//CQ_MAIN {
//    cq::config.convert_unicode_emoji = true; // ���� SDK �Զ�ת�� Emoji �� Unicode��Ĭ�Ͼ��� true��
//
//    app::on_enable = []() {
//        // logging��api��dir �������ռ��µĺ���ֻ�����¼��ص������ڲ����ã�������ֱ���� CQ_MAIN �е���
//        logging::debug(u8"����", u8"���������");
//    };
//
//    event::on_private_msg = [](const cq::PrivateMessageEvent &e) {
//        logging::debug(u8"��Ϣ", u8"�յ�˽����Ϣ��" + e.message + u8"�������ߣ�" + std::to_string(e.user_id));
//
//        api::send_private_msg(e.user_id, e.message); // echo ��ȥ
//        api::send_msg(e.target, e.message); // ʹ�� e.target ָ������Ŀ��
//
//        // MessageSegment ���ṩһЩ��̬��Ա�����Կ��ٹ�����Ϣ��
//        cq::Message msg = cq::MessageSegment::contact(cq::MessageSegment::ContactType::GROUP, 201865589);
//        msg.send(e.target); // ʹ�� Message ��� send ��Ա����
//
//        e.block(); // ��ֹ�¼��������ݸ��������
//    };
//
//    event::on_group_msg = [](const auto &e /* ʹ�� C++ �� auto �ؼ��� */) {
//        const auto memlist = api::get_group_member_list(e.group_id); // ��ȡ���ݽӿ�
//        cq::Message msg = u8"��Ⱥһ���� "; // string �� Message �Զ�ת��
//        msg += std::to_string(memlist.size()) + u8" ����Ա"; // Message ����Խ��мӷ�����
//        message::send(e.target, msg); // ʹ�� message �����ռ�� send ����
//    };
//}

// ��Ӳ˵����Ҫͬʱ�� <appid>.json �ļ��� menu �ֶ������Ӧ����Ŀ��function �ֶ�Ϊ menu_demo_1
CQ_MENU(menu_demo_1) {
    api::send_private_msg(10000, "hello");
}

// ���� CQ_INITIALIZE �� CQ_MAIN��CQ_MENU ���Զ�ε�������Ӷ���˵�
CQ_MENU(menu_demo_2) {
    logging::info(u8"�˵�", u8"�����ʾ���˵�2");
}
