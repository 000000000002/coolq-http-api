#include "./experimental_actions.h"

#include "cqhttp/utils/http.h"

using namespace std;
namespace api = cq::api;

namespace cqhttp::plugins {
    using Codes = ActionResult::Codes;

    static void action_get_friend_list(ActionContext &ctx) {
        auto &result = ctx.result;

        const auto flat = ctx.params.get_bool("flat", false);

        try {
            const auto cookies = api::get_cookies();
            const auto g_tk = to_string(api::get_csrf_token());
            const auto login_qq = to_string(api::get_login_user_id());

            {
                // try mobile QZone API
                const auto url = "http://m.qzone.com/friend/mfriend_list?g_tk=" + g_tk + "&res_uin=" + login_qq
                                 + "&res_type=normal&format=json";
                const auto res = utils::http::get_json(url, true, cookies).value_or(nullptr);
                try {
                    if (res.at("code").get<int>() == 0) {
                        // succeeded
                        auto resp_data = res.at("data");

                        if (flat) {
                            result.data = {
                                {"friend_groups", json::array()},
                                {"friends", json::array()},
                            };

                            for (auto gp : resp_data.at("gpnames")) {
                                result.data["friend_groups"].push_back({
                                    {"friend_group_id", gp.at("gpid")},
                                    {"friend_group_name", gp.at("gpname")},
                                });
                            }

                            for (auto frnd : resp_data.at("list")) {
                                result.data["friends"].push_back({
                                    {"user_id", frnd.at("uin")},
                                    {"nickname", frnd.at("nick")},
                                    {"remark", frnd.at("remark")},
                                    {"friend_group_id", frnd.at("groupid")},
                                });
                            }
                        } else {
                            result.data = json::array();

                            map<int64_t, int> gpid_idx_map;
                            for (auto gp : resp_data.at("gpnames")) {
                                auto res_gp = json::object();
                                const auto gpid = gp.at("gpid");
                                res_gp.emplace("friend_group_id", gpid);
                                res_gp.emplace("friend_group_name", gp.at("gpname"));
                                res_gp.emplace("friends", json::array());
                                gpid_idx_map[gpid] = result.data.size();
                                result.data.push_back(res_gp);
                            }

                            for (auto frnd : resp_data.at("list")) {
                                const auto gpid = frnd.at("groupid");
                                auto res_frnd = json::object();
                                res_frnd.emplace("user_id", frnd.at("uin"));
                                res_frnd.emplace("nickname", frnd.at("nick"));
                                res_frnd.emplace("remark", frnd.at("remark"));
                                result.data[gpid_idx_map[gpid]]["friends"].push_back(res_frnd);
                            }
                        }

                        result.code = Codes::OK;
                        return;
                    }
                } catch (exception &) {
                }
            }

            {
                // try desktop web QZone API
                const auto url =
                    "https://h5.qzone.qq.com/proxy/domain/r.qzone.qq.com/cgi-bin/tfriend/"
                    "friend_show_qqfriends.cgi?g_tk="
                    + g_tk + "&uin=" + login_qq;
                const auto res = utils::http::get_json(url, true, cookies).value_or(nullptr);
                try {
                    auto resp_data = res;
                    result.data = json::array();

                    map<int64_t, int> gpid_idx_map;
                    for (auto gp : resp_data.at("gpnames")) {
                        auto res_gp = json::object();
                        const auto gpid = gp.at("gpid");
                        res_gp.emplace("friend_group_id", gpid);
                        res_gp.emplace("friend_group_name", gp.at("gpname"));
                        res_gp["friends"] = json::array();
                        gpid_idx_map[gpid] = result.data.size();
                        result.data.push_back(res_gp);
                    }

                    for (auto frnd : resp_data.at("items")) {
                        const auto gpid = frnd.at("groupid");
                        auto res_frnd = json::object();
                        res_frnd.emplace("user_id", frnd.at("uin"));
                        res_frnd.emplace("nickname", frnd.at("name"));
                        res_frnd.emplace("remark", frnd.at("remark"));
                        result.data[gpid_idx_map[gpid]]["friends"].push_back(res_frnd);
                    }

                    result.code = Codes::OK;
                    return;
                } catch (exception &) {
                }
            }
        } catch (cq::exception::ApiError &) {
        }

        // failed
        result.code = Codes::CREDENTIAL_INVALID;
        result.data = nullptr;
    }

    static void action_get_group_info(ActionContext &ctx) {
        auto &result = ctx.result;

        const auto group_id = ctx.params.get_integer("group_id");
        if (group_id <= 0) {
            result.code = Codes::DEFAULT_ERROR;
            result.data = nullptr;
            return;
        }

        string login_id_str;
        string cookies;
        string csrf_token;
        try {
            login_id_str = to_string(api::get_login_user_id());
            cookies = "pt2gguin=o" + login_id_str + ";ptisp=os;p_uin=o" + login_id_str + ";" + api::get_cookies();
            csrf_token = to_string(api::get_csrf_token());
        } catch (cq::exception::ApiError &) {
            goto FAILED;
        }

        result.data = json::object();

        {
            // get basic info
            const auto url = "http://qun.qzone.qq.com/cgi-bin/get_group_member?g_tk=" + csrf_token
                             + "&uin=" + login_id_str + "&neednum=1&groupid=" + to_string(group_id);
            const auto res = utils::http::get_json(url, true, cookies).value_or(nullptr);

            try {
                const auto data = res.at("data");
                result.data.emplace("group_id", group_id);
                result.data.emplace("group_name", data.at("group_name"));
                result.data.emplace("create_time", data.at("create_time"));
                result.data.emplace("category", data.at("class"));
                result.data.emplace("member_count", data.at("total"));
                result.data.emplace("introduction", data.at("finger_memo"));
                result.data["admins"] = json::array();
                for (const auto &admin : data.at("item")) {
                    if (admin.at("iscreator") == 0 && admin.at("ismanager") == 0) {
                        // skip non-admin (may be bot itself)
                        continue;
                    }

                    json j = {
                        {"user_id", admin.at("uin")},
                        {"nickname", admin.at("nick")},
                    };
                    if (admin.at("iscreator") == 1) {
                        j["role"] = "owner";
                    } else {
                        j["role"] = "admin";
                    }
                    result.data["admins"].push_back(j);
                }
                result.data.emplace("admin_count", result.data["admins"].size());
            } catch (exception &) {
            }
        }

        {
            // get some extra info, like group size (max member count)
            const auto url = "http://qinfo.clt.qq.com/cgi-bin/qun_info/get_members_info_v1?friends=1&name=1&gc="
                             + to_string(group_id) + "&bkn=" + csrf_token + "&src=qinfo_v3";
            const auto data = utils::http::get_json(url, true, cookies).value_or(nullptr);

            try {
                result.data.emplace("owner_id", data.at("owner"));
                result.data.emplace("max_admin_count", data.at("max_admin"));
                result.data.emplace("max_member_count", data.at("max_num"));
                if (result.data.count("member_count") == 0) {
                    // it's actually impossible to reach here, but we check it in case
                    result.data.emplace("member_count", data.at("mem_num"));
                }
            } catch (exception &) {
            }
        }

        if (!result.data.empty()) {
            // we got some information at least
            result.code = Codes::OK;
            return;
        }

    FAILED:
        result.code = Codes::CREDENTIAL_INVALID;
        result.data = nullptr;
    }

    void ExperimentalActions::hook_missed_action(ActionContext &ctx) {
        if (ctx.action == "_get_friend_list") {
            action_get_friend_list(ctx);
        } else if (ctx.action == "_get_group_info") {
            action_get_group_info(ctx);
        } else {
            ctx.next();
        }
    }
} // namespace cqhttp::plugins
