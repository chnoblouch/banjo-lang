#ifndef BANJO_UTILS_PLATFORM_H
#define BANJO_UTILS_PLATFORM_H

#include <limits>

#if __x86_64__ || _M_AMD64
#    define ARCH_X86_64 1
#elif __aarch64__ || _M_ARM64
#    define ARCH_AARCH64 1
#else
#    define ARCH_UNKNOWN 1
#endif

#if defined(_WIN32)
#    define OS_WINDOWS 1
#elif __linux__
#    define OS_LINUX 1
#elif __APPLE__
#    include <TargetConditionals.h>
#    if TARGET_OS_MAC
#        define OS_MACOS 1
#    endif
#else
#    define OS_UNKNOWN 1
#endif

static_assert(std::numeric_limits<double>::is_iec559, "double type does not follow the IEEE 754 standard");

#endif
