#ifndef PLATFORM_H
#define PLATFORM_H

#include <limits>

#if defined(_WIN32)
#    define OS_WINDOWS
#elif __linux__
#    define OS_LINUX
#elif __APPLE__
#    include <TargetConditionals.h>
#    if TARGET_OS_MAC
#        define OS_MACOS
#    endif
#else
#    define OS_UNKNOWN
#endif

static_assert(std::numeric_limits<double>::is_iec559, "double type does not follow the IEEE 754 standard");

#endif
