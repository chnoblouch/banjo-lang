#ifndef PLATFORM_H
#define PLATFORM_H

#include <limits>

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
