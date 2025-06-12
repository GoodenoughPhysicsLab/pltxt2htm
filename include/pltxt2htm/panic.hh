#pragma once

#ifndef PLTXT2HTM_ENABLE_PRINT
    #error "Print msg before crashing does not be enabled"
#endif

// libc++ do not support <stacktrace>
#if __has_include(<stacktrace>)
    #include <stacktrace>
#endif
#include <fast_io/fast_io.h>

namespace pltxt2htm::details {

constexpr void panic_print() {
}

} // namespace pltxt2htm::details
