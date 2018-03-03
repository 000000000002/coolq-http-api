#pragma once

#include "cqhttp/core/common.h"

#include "Windows.h"

namespace cqhttp::utils {
    int message_box(const unsigned type, const std::string &text) {
        return MessageBoxW(nullptr,
                           s2ws(text).c_str(),
                           s2ws(CQHTTP_NAME).c_str(),
                           type | MB_SETFOREGROUND | MB_TASKMODAL | MB_TOPMOST);
    }
} // namespace cqhttp::utils
