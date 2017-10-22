// 
// handler.h : Provide basic macros and functions an API handler may need.
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

#pragma once

#include "app.h"

#include "./types.h"
#include "utils/params_class.h"

using RetCodes = ApiResult::RetCodes;

extern ApiHandlerMap api_handlers;

static bool __add_api_handler(const std::string &name, ApiHandler handler) {
    api_handlers[name] = handler;
    return true;
}

#define HANDLER(handler_name) \
    static void __##handler_name(const Params &, ApiResult &); \
    static bool __dummy_##handler_name = __add_api_handler(#handler_name, __##handler_name); \
    static void __##handler_name(const Params &params, ApiResult &result)

static void handle_async(const Params &params, ApiResult &result, ApiHandler handler) {
    static const auto TAG = u8"API�첽";
    if (pool) {
        pool->push([params, result, handler](int) {
            // copy "params" and "result" in the async task
            auto async_params = params;
            auto async_result = result;
            handler(async_params, async_result);
            Log::d(TAG, u8"�ɹ�ִ��һ�� API �����첽��������");
        });
        Log::d(TAG, u8"API �����첽���������ѽ����̳߳صȴ�ִ��");
        result.retcode = RetCodes::ASYNC;
    } else {
        Log::d(TAG, u8"�����̳߳�δ��ȷ��ʼ�����޷������첽�����볢���������");
        result.retcode = RetCodes::BAD_THREAD_POOL;
    }
}
