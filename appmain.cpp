/**
 * CoolQ HTTP API core.
 */

#include "stdafx.h"

#include <string>
#include <sstream>
#include <fstream>
#include <curl/curl.h>
#include <jansson.h>
#include <event2/event.h>
#include <event2/http.h>
#include <WinSock2.h>

#include "encoding.h"
#include "misc_functions.h"
#include "request.h"
#include "ini.h"

using namespace std;

int ac = -1; // AuthCode�����ÿ�Q�ķ���ʱ��Ҫ�õ�
bool enabled = false;

HANDLE httpd_thread_handle = NULL;
struct event_base *httpd_event_base = NULL;
struct evhttp *httpd_event = NULL;
struct cqhttp_config
{
    string host;
    int port;
    string post_url;
} httpd_config;

/*
* ����Ӧ�õ�ApiVer��Appid������󽫲������
*/
CQEVENT(const char *, AppInfo, 0)
()
{
    return CQAPPINFO;
}

/*
* ����Ӧ��AuthCode����Q��ȡӦ����Ϣ��������ܸ�Ӧ�ã���������������������AuthCode��
* ��Ҫ�ڱ��������������κδ��룬���ⷢ���쳣���������ִ�г�ʼ����������Startup�¼���ִ�У�Type=1001����
*/
CQEVENT(int32_t, Initialize, 4)
(int32_t AuthCode)
{
    ac = AuthCode;
    return 0;
}

static int parse_conf_handler(void *user, const char *section, const char *name, const char *value)
{
    struct cqhttp_config *config = (struct cqhttp_config *)user;
    if (string(section) == "general")
    {
        string field = name;
        if (field == "host")
            config->host = value;
        else if (field == "port")
            config->port = atoi(value);
        else if (field == "post_url")
            config->post_url = value;
        else
            return 0;
    }
    else
        return 0; /* unknown section/name, error */
    return 1;
}

/**
 * Initialize plugin, called immediately when plugin is enabled.
 */
void init()
{
    LOG_D("����", "��ʼ��");

    // default config
    httpd_config.host = "0.0.0.0";
    httpd_config.port = 5700;
    httpd_config.post_url = "";

    string conf_path = string(CQ_getAppDirectory(ac)) + "config.cfg";
    FILE *conf_file = NULL;
    fopen_s(&conf_file, conf_path.c_str(), "r");
    if (!conf_file)
    {
        // first init, save default config
        LOG_D("����", "û���ҵ������ļ���д��Ĭ������");
        ofstream file(conf_path);
        file << "[general]\nhost=0.0.0.0\nport=5700\npost_url=\n";
    }
    else
    {
        // load from config file
        LOG_D("����", "��ȡ�����ļ�");
        ini_parse_file(conf_file, parse_conf_handler, &httpd_config);
        fclose(conf_file);
    }
}

/**
 * Cleanup plugin, called after all other operations when plugin is disabled.
 */
void cleanup()
{
    // do nothing currently
}

/**
 * Portal function of HTTP daemon thread.
 */
DWORD WINAPI httpd_thread_func(LPVOID lpParam)
{
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);

    httpd_event_base = event_base_new();
    httpd_event = evhttp_new(httpd_event_base);

    evhttp_set_gencb(httpd_event, cqhttp_main_handler, NULL);
    evhttp_bind_socket(httpd_event, httpd_config.host.c_str(), httpd_config.port);

    stringstream ss;
    ss << "��ʼ���� " << httpd_config.host << ":" << httpd_config.port;
    LOG_D("HTTP�߳�", ss.str());

    event_base_dispatch(httpd_event_base);
    return 0;
}

/**
 * Start HTTP daemon thread.
 */
void start_httpd()
{
    httpd_thread_handle = CreateThread(NULL,              // default security attributes
                                       0,                 // use default stack size
                                       httpd_thread_func, // thread function name
                                       NULL,              // argument to thread function
                                       0,                 // use default creation flags
                                       NULL);             // returns the thread identifier
    if (!httpd_thread_handle)
    {
        LOG_E("����", "���� HTTP �ػ��߳�ʧ��");
    }
    else
    {
        LOG_D("����", "���� HTTP �ػ��̳߳ɹ�");
    }
}

/**
 * Stop HTTP daemon thread.
 */
void stop_httpd()
{
    if (httpd_thread_handle)
    {
        if (httpd_event_base)
        {
            event_base_loopbreak(httpd_event_base);
        }
        if (httpd_event)
        {
            evhttp_free(httpd_event);
        }
        // if (httpd_event_base)
        // {
        //     event_base_free(httpd_event_base);
        // }
        WSACleanup();
        CloseHandle(httpd_thread_handle);
        httpd_thread_handle = NULL;
        httpd_event_base = NULL;
        httpd_event = NULL;
        LOG_D("ͣ��", "�ѹرպ�̨ HTTP �ػ��߳�")
    }
}

/*
* Event: plugin is enabled.
*/
CQEVENT(int32_t, __eventEnable, 0)
()
{
    enabled = true;
    init();
    start_httpd();
    return 0;
}

/*
* Event: plugin is disabled.
*/
CQEVENT(int32_t, __eventDisable, 0)
()
{
    enabled = false;
    stop_httpd();
    cleanup();
    return 0;
}

/*
* Type=21 ˽����Ϣ
* subType �����ͣ�11/���Ժ��� 1/��������״̬ 2/����Ⱥ 3/����������
*/
CQEVENT(int32_t, __eventPrivateMsg, 24)
(int32_t subType, int32_t sendTime, int64_t fromQQ, const char *msg, int32_t font)
{

    //���Ҫ�ظ���Ϣ������ÿ�Q�������ͣ��������� return EVENT_BLOCK - �ضϱ�����Ϣ�����ټ�������  ע�⣺Ӧ�����ȼ�����Ϊ"���"(10000)ʱ������ʹ�ñ�����ֵ
    //������ظ���Ϣ������֮���Ӧ��/�������������� return EVENT_IGNORE - ���Ա�����Ϣ

    string result = "";

    CURL *curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "http://news-at.zhihu.com/api/4/news/latest");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_stringstream_callback);

        stringstream resp_stream;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &resp_stream);

        CURLcode res;
        res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            string json_string = resp_stream.str();
            LOG8_D("Net>Json", string("Got json string: ") + json_string);
            json_t *data = json_loads(json_string.c_str(), 0, NULL);
            if (data)
            {
                LOG_D("Net>Json", "Succeeded to parse json data");
                stringstream ss;
                const char *date = json_string_value(json_object_get(data, "date"));
                ss << "Date: " << date << "\n\n";
                json_t *stories_jarr = json_object_get(data, "stories");
                for (size_t i = 0; i < json_array_size(stories_jarr); i++)
                {
                    const char *title = json_string_value(json_object_get(json_array_get(stories_jarr, i), "title"));
                    ss << (i == 0 ? "" : "\n") << i << ". " << title;
                }
                json_decref(data);
                result = ss.str();
            }
            else
            {
                LOG_D("Net>Json", "Failed to load json string");
            }
        }
        else
        {
            LOG_D("Net", "Failed to get response");
        }
        curl_easy_cleanup(curl);
    }
    else
    {
        LOG_D("Net", "Failed to init cURL");
    }

    if (result != "")
    {
        CQ_sendPrivateMsg(ac, fromQQ, utf8_to_gbk(result.c_str()).c_str());
        CQ_sendPrivateMsg(ac, fromQQ, msg);
    }

    return EVENT_IGNORE;
}

/*
* Type=2 Ⱥ��Ϣ
*/
CQEVENT(int32_t, __eventGroupMsg, 36)
(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t fromQQ, const char *fromAnonymous, const char *msg, int32_t font)
{

    return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}

/*
* Type=4 ��������Ϣ
*/
CQEVENT(int32_t, __eventDiscussMsg, 32)
(int32_t subType, int32_t sendTime, int64_t fromDiscuss, int64_t fromQQ, const char *msg, int32_t font)
{

    return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}

/*
* Type=101 Ⱥ�¼�-����Ա�䶯
* subType �����ͣ�1/��ȡ������Ա 2/�����ù���Ա
*/
CQEVENT(int32_t, __eventSystem_GroupAdmin, 24)
(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t beingOperateQQ)
{

    return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}

/*
* Type=102 Ⱥ�¼�-Ⱥ��Ա����
* subType �����ͣ�1/ȺԱ�뿪 2/ȺԱ���� 3/�Լ�(����¼��)����
* fromQQ ������QQ(��subTypeΪ2��3ʱ����)
* beingOperateQQ ������QQ
*/
CQEVENT(int32_t, __eventSystem_GroupMemberDecrease, 32)
(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t fromQQ, int64_t beingOperateQQ)
{

    return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}

/*
* Type=103 Ⱥ�¼�-Ⱥ��Ա����
* subType �����ͣ�1/����Ա��ͬ�� 2/����Ա����
* fromQQ ������QQ(������ԱQQ)
* beingOperateQQ ������QQ(����Ⱥ��QQ)
*/
CQEVENT(int32_t, __eventSystem_GroupMemberIncrease, 32)
(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t fromQQ, int64_t beingOperateQQ)
{

    return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}

/*
* Type=201 �����¼�-���������
*/
CQEVENT(int32_t, __eventFriend_Add, 16)
(int32_t subType, int32_t sendTime, int64_t fromQQ)
{

    return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}

/*
* Type=301 ����-�������
* msg ����
* responseFlag ������ʶ(����������)
*/
CQEVENT(int32_t, __eventRequest_AddFriend, 24)
(int32_t subType, int32_t sendTime, int64_t fromQQ, const char *msg, const char *responseFlag)
{

    //CQ_setFriendAddRequest(ac, responseFlag, REQUEST_ALLOW, "");

    return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}

/*
* Type=302 ����-Ⱥ���
* subType �����ͣ�1/����������Ⱥ 2/�Լ�(����¼��)������Ⱥ
* msg ����
* responseFlag ������ʶ(����������)
*/
CQEVENT(int32_t, __eventRequest_AddGroup, 32)
(int32_t subType, int32_t sendTime, int64_t fromGroup, int64_t fromQQ, const char *msg, const char *responseFlag)
{

    //if (subType == 1) {
    //	CQ_setGroupAddRequestV2(ac, responseFlag, REQUEST_GROUPADD, REQUEST_ALLOW, "");
    //} else if (subType == 2) {
    //	CQ_setGroupAddRequestV2(ac, responseFlag, REQUEST_GROUPINVITE, REQUEST_ALLOW, "");
    //}

    return EVENT_IGNORE; //���ڷ���ֵ˵��, ����_eventPrivateMsg������
}
