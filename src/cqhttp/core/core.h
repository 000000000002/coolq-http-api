#pragma once

#include "cqhttp/core/common.h"

#include "cqhttp/core/context.h"
#include "cqhttp/core/plugin.h"

namespace cqhttp {

    class Application {
    public:
        void use(const std::shared_ptr<Plugin> plugin) { plugins_.push_back(plugin); }

        void __hook_initialize() { iterate_hooks(&Plugin::hook_initialize, Context()); }
        void __hook_enable() { iterate_hooks(&Plugin::hook_enable, Context()); }
        void __hook_disable() { iterate_hooks(&Plugin::hook_disable, Context()); }
        void __hook_coolq_start() { iterate_hooks(&Plugin::hook_coolq_start, Context()); }
        void __hook_coolq_exit() { iterate_hooks(&Plugin::hook_coolq_exit, Context()); }

        void __hook_message_event(const cq::MessageEvent &event, json &data) {
            iterate_hooks(&Plugin::hook_message_event, EventContext<cq::MessageEvent>(event, data));
        }

        void __hook_notice_event(const cq::NoticeEvent &event, json &data) {
            iterate_hooks(&Plugin::hook_notice_event, EventContext<cq::NoticeEvent>(event, data));
        }

        void __hook_request_event(const cq::RequestEvent &event, json &data) {
            iterate_hooks(&Plugin::hook_request_event, EventContext<cq::RequestEvent>(event, data));
        }

        void __hook_before_action(const std::string &action, utils::JsonEx &params, json &result) {
            iterate_hooks(&Plugin::hook_before_action, ActionContext(action, params, result));
        }

        void __hook_missed_action(const std::string &action, utils::JsonEx &params, json &result) {
            iterate_hooks(&Plugin::hook_missed_action, ActionContext(action, params, result));
        }

        void __hook_after_action(const std::string &action, utils::JsonEx &params, json &result) {
            iterate_hooks(&Plugin::hook_after_action, ActionContext(action, params, result));
        }

    private:
        std::vector<std::shared_ptr<Plugin>> plugins_;

        template <typename HookFunc, typename Ctx>
        void iterate_hooks(const HookFunc hook_func, Ctx &&ctx) {
            ctx.config = nullptr;

            auto it = plugins_.begin();
            Context::Next next = [&]() {
                if (it == plugins_.end()) {
                    return;
                }

                ctx.next = next;
                (**it++.*hook_func)(std::forward<Ctx>(ctx));
            };
            next();
        }
    };

    /**
     * Initialize the module.
     * This will assign the event handlers of "cqhttp" module to "cqsdk" module.
     */
    void init();

    /**
     * Apply the given application. This will override any previous applied applications.
     */
    void apply(const std::shared_ptr<Application> app);
} // namespace cqhttp
