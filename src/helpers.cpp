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

#include <codecvt>
#include <regex>
#include <openssl/hmac.h>
#include <boost/filesystem.hpp>
#include <cpprest/filestream.h>

#include "utils/rest_client.h"
#include "utils/encoding.h"

using namespace std;
namespace fs = boost::filesystem;

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

string ws2s(const wstring &ws) {
    return std::wstring_convert<codecvt_utf8<wchar_t>, wchar_t>().to_bytes(ws);
}

wstring s2ws(const string &s) {
    return std::wstring_convert<codecvt_utf8<wchar_t>, wchar_t>().from_bytes(s);
}

string ansi(const string &s) {
    return string_encode(s, Encodings::ANSI);
}

bool to_bool(const string &str, const bool default_val) {
    auto result = to_bool(str);
    return result ? result.value() : default_val;
}

optional<bool> to_bool(const string &str) {
    const auto s = boost::algorithm::to_lower_copy(str);
    if (s == "yes" || s == "true" || s == "1") {
        return true;
    }
    if (s == "no" || s == "false" || s == "0") {
        return false;
    }
    return nullopt;
}

namespace std {
    string to_string(const string &val) {
        return val;
    }

    string to_string(bool val) {
        return val ? "true" : "false";
    }
}

#define FAKE_USER_AGENT "Mozilla/5.0 (Windows NT 10.0; Win64; x64) " \
    "AppleWebKit/537.36 (KHTML, like Gecko) " \
    "Chrome/56.0.2924.87 Safari/537.36"

optional<json> get_remote_json(const string &url, const bool use_fake_ua, const string &cookies) {
    http_request request(http::methods::GET);
    request.headers().add(L"User-Agent", s2ws(use_fake_ua ? FAKE_USER_AGENT : CQAPP_USER_AGENT));
    request.headers().add(L"Referer", s2ws(url));
    if (!cookies.empty()) {
        request.headers().add(L"Cookie", s2ws(cookies));
    }
    return get_remote_json(url, request);
}

optional<nlohmann::json> get_remote_json(const string &url, const http_request &request) {
    auto task = http_client(s2ws(url))
            .request(request)
            .then([](pplx::task<http_response> task) {
                auto next_task = pplx::task_from_result<string>("");
                try {
                    auto resp = task.get();
                    if (resp.status_code() == 200) {
                        next_task = resp.extract_utf8string(true);
                    }
                } catch (http_exception &) {
                    // failed to request
                }
                return next_task;
            })
            .then([](string &body) {
                if (smatch m; regex_search(body, m, regex("\\);?\\s*$"))) {
                    // is jsonp
                    if (auto start = body.find("("); start != string::npos) {
                        body = body.substr(start + 1, body.size() - (start + 1) - m.length());
                    }
                }
                if (!body.empty()) {
                    return pplx::task_from_result(make_optional<json>(json::parse(body)));
                    // may throw invalid_argument due to invalid json
                }
                return pplx::task_from_result(optional<json>());
            });

    try {
        return task.get();
    } catch (invalid_argument &) {
        return nullopt;
    }
}

bool download_remote_file(const string &url, const string &local_path, const bool use_fake_ua) {
    using concurrency::streams::container_buffer;

    auto succeeded = false;

    auto ansi_local_path = ansi(local_path);

    http_request request(http::methods::GET);
    request.headers().add(L"User-Agent", s2ws(use_fake_ua ? FAKE_USER_AGENT : CQAPP_USER_AGENT));
    request.headers().add(L"Referer", s2ws(url));

    http_client(s2ws(url)).request(request).then([&](http_response response) {
        if (ofstream f(ansi_local_path, ios::out | ios::binary); f.is_open()) {
            auto length = response.headers().content_length();
            decltype(length) read_count = 0;
            auto body_stream = response.body();

            size_t last_read_count = 0;
            do {
                container_buffer<string> buffer;
                body_stream.read(buffer, 8192).then([&](size_t count) {
                    read_count += count;
                    last_read_count = count;
                }).wait();

                f << buffer.collection().substr(0, last_read_count);
            } while (last_read_count > 0);

            if (read_count == length) {
                succeeded = true;
            }
            f.close();
        }
    }).wait();

    if (!succeeded && fs::exists(ansi_local_path)) {
        fs::remove(ansi_local_path);
    }

    return succeeded;
}

int message_box(unsigned type, const string &text) {
    return MessageBoxW(nullptr,
                       s2ws(text).c_str(),
                       s2ws(CQAPP_NAME).c_str(),
                       type | MB_SETFOREGROUND | MB_TASKMODAL | MB_TOPMOST);
}

string hmac_sha1_hex(string key, string msg) {
    unsigned digest_len = 20;
    const auto digest = new unsigned char[digest_len];
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, key.c_str(), key.size(), EVP_sha1(), nullptr);
    HMAC_Update(&ctx, reinterpret_cast<const unsigned char *>(msg.c_str()), msg.size());
    HMAC_Final(&ctx, digest, &digest_len);
    HMAC_CTX_cleanup(&ctx);

    stringstream ss;
    for (unsigned i = 0; i < digest_len; ++i) {
        ss << hex << setfill('0') << setw(2) << static_cast<unsigned int>(digest[i]);
    }
    delete[] digest;
    return ss.str();
}

static const uint32_t EMOJI_RANGES_1[][2] = {
    {0x203C, 0x203C},{0x2049, 0x2049},{0x2122, 0x2122},{0x2139, 0x2139},{0x2194, 0x2199},{0x21A9, 0x21AA},
    {0x231A, 0x231B},{0x2328, 0x2328},{0x23CF, 0x23CF},{0x23E9, 0x23F3},{0x23F8, 0x23FA},{0x24C2, 0x24C2},
    {0x25AA, 0x25AB},{0x25B6, 0x25B6},{0x25C0, 0x25C0},{0x25FB, 0x25FE},{0x2600, 0x2604},{0x260E, 0x260E},
    {0x2611, 0x2611},{0x2614, 0x2615},{0x2618, 0x2618},{0x261D, 0x261D},{0x2620, 0x2620},{0x2622, 0x2623},
    {0x2626, 0x2626},{0x262A, 0x262A},{0x262E, 0x262F},{0x2638, 0x263A},{0x2640, 0x2640},{0x2642, 0x2642},
    {0x2648, 0x2653},{0x2660, 0x2660},{0x2663, 0x2663},{0x2665, 0x2666},{0x2668, 0x2668},{0x267B, 0x267B},
    {0x267F, 0x267F},{0x2692, 0x2697},{0x2699, 0x2699},{0x269B, 0x269C},{0x26A0, 0x26A1},{0x26AA, 0x26AB},
    {0x26B0, 0x26B1},{0x26BD, 0x26BE},{0x26C4, 0x26C5},{0x26C8, 0x26C8},{0x26CE, 0x26CE},{0x26CF, 0x26CF},
    {0x26D1, 0x26D1},{0x26D3, 0x26D4},{0x26E9, 0x26EA},{0x26F0, 0x26F5},{0x26F7, 0x26FA},{0x26FD, 0x26FD},
    {0x2702, 0x2702},{0x2705, 0x2705},{0x2708, 0x2709},{0x270A, 0x270B},{0x270C, 0x270D},{0x270F, 0x270F},
    {0x2712, 0x2712},{0x2714, 0x2714},{0x2716, 0x2716},{0x271D, 0x271D},{0x2721, 0x2721},{0x2728, 0x2728},
    {0x2733, 0x2734},{0x2744, 0x2744},{0x2747, 0x2747},{0x274C, 0x274C},{0x274E, 0x274E},{0x2753, 0x2755},
    {0x2757, 0x2757},{0x2763, 0x2764},{0x2795, 0x2797},{0x27A1, 0x27A1},{0x27B0, 0x27B0},{0x27BF, 0x27BF},
    {0x2934, 0x2935},{0x2B05, 0x2B07},{0x2B1B, 0x2B1C},{0x2B50, 0x2B50},{0x2B55, 0x2B55},{0x3030, 0x3030},
    {0x303D, 0x303D},{0x3297, 0x3297},{0x3299, 0x3299}
};
static const uint32_t EMOJI_RANGES_2[][2] = {
    {0x1F004, 0x1F004},{0x1F0CF, 0x1F0CF},{0x1F170, 0x1F171},{0x1F17E, 0x1F17E},{0x1F17F, 0x1F17F},{0x1F18E, 0x1F18E},
    {0x1F191, 0x1F19A},{0x1F1E6, 0x1F1FF},{0x1F201, 0x1F202},{0x1F21A, 0x1F21A},{0x1F22F, 0x1F22F},{0x1F232, 0x1F23A},
    {0x1F250, 0x1F251},{0x1F300, 0x1F320},{0x1F321, 0x1F321},{0x1F324, 0x1F32C},{0x1F32D, 0x1F32F},{0x1F330, 0x1F335},
    {0x1F336, 0x1F336},{0x1F337, 0x1F37C},{0x1F37D, 0x1F37D},{0x1F37E, 0x1F37F},{0x1F380, 0x1F393},{0x1F396, 0x1F397},
    {0x1F399, 0x1F39B},{0x1F39E, 0x1F39F},{0x1F3A0, 0x1F3C4},{0x1F3C5, 0x1F3C5},{0x1F3C6, 0x1F3CA},{0x1F3CB, 0x1F3CE},
    {0x1F3CF, 0x1F3D3},{0x1F3D4, 0x1F3DF},{0x1F3E0, 0x1F3F0},{0x1F3F3, 0x1F3F5},{0x1F3F7, 0x1F3F7},{0x1F3F8, 0x1F3FF},
    {0x1F400, 0x1F43E},{0x1F43F, 0x1F43F},{0x1F440, 0x1F440},{0x1F441, 0x1F441},{0x1F442, 0x1F4F7},{0x1F4F8, 0x1F4F8},
    {0x1F4F9, 0x1F4FC},{0x1F4FD, 0x1F4FD},{0x1F4FF, 0x1F4FF},{0x1F500, 0x1F53D},{0x1F549, 0x1F54A},{0x1F54B, 0x1F54E},
    {0x1F550, 0x1F567},{0x1F56F, 0x1F570},{0x1F573, 0x1F579},{0x1F57A, 0x1F57A},{0x1F587, 0x1F587},{0x1F58A, 0x1F58D},
    {0x1F590, 0x1F590},{0x1F595, 0x1F596},{0x1F5A4, 0x1F5A4},{0x1F5A5, 0x1F5A5},{0x1F5A8, 0x1F5A8},{0x1F5B1, 0x1F5B2},
    {0x1F5BC, 0x1F5BC},{0x1F5C2, 0x1F5C4},{0x1F5D1, 0x1F5D3},{0x1F5DC, 0x1F5DE},{0x1F5E1, 0x1F5E1},{0x1F5E3, 0x1F5E3},
    {0x1F5E8, 0x1F5E8},{0x1F5EF, 0x1F5EF},{0x1F5F3, 0x1F5F3},{0x1F5FA, 0x1F5FA},{0x1F5FB, 0x1F5FF},{0x1F600, 0x1F600},
    {0x1F601, 0x1F610},{0x1F611, 0x1F611},{0x1F612, 0x1F614},{0x1F615, 0x1F615},{0x1F616, 0x1F616},{0x1F617, 0x1F617},
    {0x1F618, 0x1F618},{0x1F619, 0x1F619},{0x1F61A, 0x1F61A},{0x1F61B, 0x1F61B},{0x1F61C, 0x1F61E},{0x1F61F, 0x1F61F},
    {0x1F620, 0x1F625},{0x1F626, 0x1F627},{0x1F628, 0x1F62B},{0x1F62C, 0x1F62C},{0x1F62D, 0x1F62D},{0x1F62E, 0x1F62F},
    {0x1F630, 0x1F633},{0x1F634, 0x1F634},{0x1F635, 0x1F640},{0x1F641, 0x1F642},{0x1F643, 0x1F644},{0x1F645, 0x1F64F},
    {0x1F680, 0x1F6C5},{0x1F6CB, 0x1F6CF},{0x1F6D0, 0x1F6D0},{0x1F6D1, 0x1F6D2},{0x1F6E0, 0x1F6E5},{0x1F6E9, 0x1F6E9},
    {0x1F6EB, 0x1F6EC},{0x1F6F0, 0x1F6F0},{0x1F6F3, 0x1F6F3},{0x1F6F4, 0x1F6F6},{0x1F6F7, 0x1F6F8},{0x1F910, 0x1F918},
    {0x1F919, 0x1F91E},{0x1F91F, 0x1F91F},{0x1F920, 0x1F927},{0x1F928, 0x1F92F},{0x1F930, 0x1F930},{0x1F931, 0x1F932},
    {0x1F933, 0x1F93A},{0x1F93C, 0x1F93E},{0x1F940, 0x1F945},{0x1F947, 0x1F94B},{0x1F94C, 0x1F94C},{0x1F950, 0x1F95E},
    {0x1F95F, 0x1F96B},{0x1F980, 0x1F984},{0x1F985, 0x1F991},{0x1F992, 0x1F997},{0x1F9C0, 0x1F9C0},{0x1F9D0, 0x1F9E6}
};

bool is_emoji(const uint32_t codepoint) {
    if (codepoint >= 0x203C && codepoint <= 0x3299) {
        for (auto pair : EMOJI_RANGES_1) {
            if (codepoint < pair[0])
                return false;
            if (codepoint <= pair[1])
                return true;
        }
        return false;
    }
    if (codepoint >= 0x1F004 && codepoint <= 0x1F9E6) {
        for (auto pair : EMOJI_RANGES_2) {
            if (codepoint < pair[0])
                return false;
            if (codepoint <= pair[1])
                return true;
        }
        return false;
    }
    return false;
}

string string_to_coolq(const string &str) {
    // call CoolQ API
    string processed_str;

    wstring_convert<codecvt_utf8<uint32_t>, uint32_t> uint32_conv;
    auto uint32_str = uint32_conv.from_bytes(str);

    auto append_text = [&](decltype(uint32_str.cbegin()) begin, decltype(uint32_str.cbegin()) end) {
        decltype(uint32_str) uint32_part_str(begin, end);
        auto utf8_part_str = uint32_conv.to_bytes(uint32_part_str);
        processed_str += utf8_part_str;
    };

    auto last_it = uint32_str.cbegin();
    for (auto it = uint32_str.cbegin(); it != uint32_str.cend(); ++it) {
        const auto codepoint = *it;
        if (is_emoji(codepoint)) {
            // is emoji
            append_text(last_it, it);
            processed_str += "[CQ:emoji,id=" + to_string(codepoint) + "]";
            last_it = it + 1;
        }
    }
    append_text(last_it, uint32_str.cend());

    return string_encode(processed_str, Encodings::ANSI);
}

string string_from_coolq(const string &str) {
    // handle CoolQ event and data
    auto utf8_str = string_decode(str, Encodings::ANSI);
    string processed_str;

    const regex r(R"(\[CQ:emoji,\s*id=(\d+)\])");
    smatch m;
    auto it = utf8_str.cbegin();
    while (regex_search(it, utf8_str.cend(), m, r)) {
        processed_str += string(it, it + m.position());

        const auto codepoint = static_cast<char32_t>(stoul(m.str(1)));
        const u32string u32_str = {codepoint};
        const auto p = reinterpret_cast<const uint32_t *>(u32_str.data());
        wstring_convert<codecvt_utf8<uint32_t>, uint32_t> conv;
        const auto emoji_utf8_str = conv.to_bytes(p, p + u32_str.size());
        processed_str += emoji_utf8_str;

        it += m.position() + m.length();
    }
    processed_str += string(it, utf8_str.cend());

    return processed_str;
}
