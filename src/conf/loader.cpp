// 
// loader.cpp : Provide function to load configuration.
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

#include "loader.h"

#include "app.h"

#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>

#include "helpers.h"
#include "config_struct.h"

using namespace std;

optional<Config> load_configuration(const string &filepath) {
    static const auto TAG = u8"����";

    Config config;

    Log::d(TAG, u8"���Լ��������ļ�");

    const auto login_qq_str = to_string(sdk->get_login_qq());

    const auto ansi_filepath = ansi(filepath);
    if (!boost::filesystem::is_regular_file(ansi_filepath)) {
        // create default config file
        Log::i(TAG, u8"û���ҵ������ļ���д��Ĭ������");
        if (ofstream file(ansi_filepath); file.is_open()) {
            file << "[" << login_qq_str << "]" << endl
                    << "host=0.0.0.0" << endl
                    << "port=5700" << endl
                    << "post_url=" << endl
                    << "access_token=" << endl
                    << "secret=" << endl
                    << "post_message_format=string" << endl
                    << "serve_data_files=no" << endl
                    << "update_source=https://raw.githubusercontent.com/richardchien/coolq-http-api-release/master/" << endl
                    << "update_channel=stable" << endl
                    << "auto_check_update=no" << endl
                    << "thread_pool_size=4" << endl;
            file.close();
        } else {
            Log::e(TAG, u8"д��Ĭ������ʧ�ܣ������ļ�ϵͳȨ��");
        }
    }

    // load from config file
    try {
        boost::property_tree::ptree pt;
        read_ini(ansi_filepath, pt);

        struct string_to_bool_translator {
            boost::optional<bool> get_value(const string &s) const {
                auto b_opt = to_bool(s);
                return b_opt ? boost::make_optional<bool>(b_opt.value()) : boost::none;
            }
        };

        #define GET_CONFIG(key, type) \
            config.key = pt.get<type>(login_qq_str + "." #key, config.key); \
            Log::d(TAG, #key "=" + to_string(config.key))
        #define GET_BOOL_CONFIG(key) \
            config.key = pt.get<bool>(login_qq_str + "." #key, config.key, string_to_bool_translator()); \
            Log::d(TAG, #key "=" + to_string(config.key))
        GET_CONFIG(host, string);
        GET_CONFIG(port, unsigned short);
        GET_CONFIG(post_url, string);
        GET_CONFIG(access_token, string);
        GET_CONFIG(secret, string);
        GET_CONFIG(post_message_format, string);
        GET_BOOL_CONFIG(serve_data_files);
        GET_CONFIG(update_source, string);
        GET_CONFIG(update_channel, string);
        GET_BOOL_CONFIG(auto_check_update);
        GET_CONFIG(thread_pool_size, int);
        #undef GET_CONFIG

        Log::i(TAG, u8"���������ļ��ɹ�");
    } catch (...) {
        // failed to load configurations
        Log::e(TAG, u8"���������ļ�ʧ�ܣ����������ļ���ʽ�ͷ���Ȩ��");
        return nullopt;
    }

    return config;
}
