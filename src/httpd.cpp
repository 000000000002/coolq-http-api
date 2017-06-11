#include "httpd.h"

#include <event2/event.h>
#include <event2/http.h>
#include <WinSock2.h>
#include <thread>
#include <atomic>

#include "request.h"
#include "helpers.h"
#include "conf/Config.h"
#include "CQApp.h"

using namespace std;

extern CQApp *CQ;

static thread httpd_thread;
static atomic<bool> httpd_thread_running = false;
static struct event_base *httpd_event_base = nullptr;
static struct evhttp *httpd_event = nullptr;

/**
 * Start HTTP daemon thread.
 */
void start_httpd() {
    // try to stop httpd first, in case of error
    stop_httpd();

    httpd_thread = thread([]() { // TODO: bug when enable after disable
            httpd_thread_running = true;

            auto &config = CQ->config;

            // WSADATA wsa_data;
            // WSAStartup(MAKEWORD(2, 2), &wsa_data);

            httpd_event_base = event_base_new();
            httpd_event = evhttp_new(httpd_event_base);

            evhttp_set_gencb(httpd_event, cqhttp_main_handler, nullptr);
            evhttp_bind_socket(httpd_event, config.host.c_str(), config.port);

            LOG_D("����", string("��ʼ���� http://") + config.host + ":" + itos(config.port));

            event_base_dispatch(httpd_event_base); // infinite event loop

            httpd_thread_running = false;
        });
    LOG_D("����", "���� HTTP �ػ��̳߳ɹ�");
}

/**
 * Stop HTTP daemon thread.
 */
void stop_httpd() {
    if (httpd_thread_running) {
        if (httpd_event_base) {
            event_base_loopbreak(httpd_event_base);
        }
        if (httpd_event) {
            evhttp_free(httpd_event);
        }
        // it seems that the following cannot be done properly, don't know why
        // if (httpd_event_base) {
        //     event_base_free(httpd_event_base);
        // }
        // WSACleanup();
        httpd_event_base = nullptr;
        httpd_event = nullptr;
        httpd_thread_running = false;
        LOG_D("�ر�", "�ѹرպ�̨ HTTP �ػ��߳�");
    }
}
