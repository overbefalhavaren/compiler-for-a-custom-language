#pragma once

#include <cstdint>

namespace c {

namespace log {

enum Level : uint8_t {
    Debug,
    Info,
    StyleWarn,  
    Warning,
    Error
};

} // namespace log
} // namespace c
