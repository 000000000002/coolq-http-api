#pragma once

#include "cqhttp/core/plugin.h"

namespace cqhttp::plugins {
    struct Loggers : Plugin {
        void hook_enable(Context &ctx) override;
        void hook_disable(Context &ctx) override;
        void hook_missed_action(ActionContext &ctx) override;
    };

    static std::shared_ptr<Loggers> loggers = std::make_shared<Loggers>();
} // namespace cqhttp::plugins
