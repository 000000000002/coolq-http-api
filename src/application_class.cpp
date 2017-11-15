// 
// application_class.cpp : Implement Application class
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

#include "./application_class.h"

#include "app.h"

#include "conf/loader.h"
#include "service/hub_class.h"

using namespace std;

void Application::initialize(const int32_t auth_code) {
    init_sdk(auth_code);
    initialized_ = true;

    restart_worker_running_ = true;
    restart_worker_thread_ = thread([&]() {
        static const auto tag = u8"����";
        while (restart_worker_running_) {
            if (should_restart_) {
                // this is not thread-safe, but currently it's ok
                if (restart_delay_ > 0) {
                    Log::i(tag, u8"HTTP API ������� " + to_string(restart_delay_) + u8" ���������");
                }
                Sleep(restart_delay_);
                disable();
                enable();
                should_restart_ = false;
                Log::i(tag, u8"HTTP API ��������ɹ�");
            }
        }
    });
}

void Application::enable() {
    static const auto TAG = u8"����";

    if (!initialized_ || enabled_) {
        return;
    }

    Log::d(TAG, CQAPP_FULLNAME);
    Log::d(TAG, u8"��ʼ��ʼ��");

    if (const auto c = load_configuration(sdk->directories().app() + "config.cfg")) {
        config = c.value();
    }

    ServiceHub::instance().start();

    if (!pool) {
        Log::d(TAG, u8"�����̳߳ش����ɹ�");
        pool = make_shared<ctpl::thread_pool>(
            config.thread_pool_size > 0 ? config.thread_pool_size : thread::hardware_concurrency() * 2 + 1
        );
    }

    enabled_ = true;
    Log::i(TAG, u8"HTTP API ���������");
}

void Application::disable() {
    static const auto TAG = u8"ͣ��";

    if (!enabled_) {
        return;
    }

    ServiceHub::instance().stop();

    if (pool) {
        pool->stop();
        pool = nullptr;
        Log::d(TAG, u8"�����̳߳عرճɹ�");
    }

    enabled_ = false;
    Log::i(TAG, u8"HTTP API �����ͣ��");
}

void Application::restart_async(const unsigned long delay_millisecond) {
    restart_delay_ = delay_millisecond;
    should_restart_ = true; // this will let the restart worker do it
}
