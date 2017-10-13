// 
// helpers.cpp : Implement helper functions.
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

#include "helpers.h"

#include "app.h"

#include <cctype>

using namespace std;

bool isnumber(const string &s) {
    for (auto ch : s) {
        if (!isdigit(ch)) {
            return false;
        }
    }
    return !s.empty();
}

bool isfile(const string &path) {
    struct stat st;
    if (stat(ansi(path).c_str(), &st) < 0) {
        return false;
    }
    return (st.st_mode & S_IFMT) != S_IFDIR;
}

long long filesize(const string &path) {
    struct stat st;
    if (stat(ansi(path).c_str(), &st) < 0 || (st.st_mode & S_IFMT) == S_IFDIR) {
        return -1;
    }
    return st.st_size;
}

void string_replace(string &str, const string &search, const string &replace) {
    if (search.empty())
        return;
    string ws_ret;
    ws_ret.reserve(str.length());
    size_t start_pos = 0, pos;
    while ((pos = str.find(search, start_pos)) != string::npos) {
        ws_ret += str.substr(start_pos, pos - start_pos);
        ws_ret += replace;
        pos += search.length();
        start_pos = pos;
    }
    ws_ret += str.substr(start_pos);
    str.swap(ws_ret); // faster than str = wsRet;
}

/**
 * Get root directory of CoolQ, including the trailing "\".
 */
string get_coolq_root() {
    static string root;
    if (root.empty()) {
        const auto app_dir = sdk->get_app_directory();
        const auto suffix = string("app\\" CQAPP_ID "\\");
        root = app_dir.substr(0, app_dir.length() - suffix.length());
    }
    return root;
}

string ansi(const string &s) {
    return string_encode(s, Encodings::ANSI);
}

bool text_to_bool(const string &text) {
    auto t = boost::algorithm::to_lower_copy(text);
    return t == "yes" || t == "true" || t == "1";
}

namespace std {
    string to_string(const string &val) {
        return val;
    }

    string to_string(bool val) {
        return val ? "true" : "false";
    }
}
