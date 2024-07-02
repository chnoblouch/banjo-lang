#include "paths.hpp"

#include "banjo/utils/platform.hpp"

#if defined(OS_WINDOWS)
#    include <windows.h>
#elif defined(OS_LINUX)
#    include <linux/limits.h>
#    include <unistd.h>
#elif defined(OS_MACOS)
#    include <mach-o/dyld.h>
#    include <sys/syslimits.h>
#else
#    error finding paths to toolchain files is not implemented on this OS
#endif

namespace banjo {

namespace Paths {

std::filesystem::path executable() {
#if defined(OS_WINDOWS)
    const DWORD buffer_size = 512;
    CHAR buffer[buffer_size];
    GetModuleFileName(NULL, buffer, buffer_size);
    return buffer;
#elif defined(OS_LINUX)
    char buffer[PATH_MAX + 1];
    size_t length = readlink("/proc/self/exe", buffer, PATH_MAX);
    buffer[length] = '\0';
    return buffer;
#elif defined(OS_MACOS)
    char buffer[PATH_MAX + 1];
    uint32_t buffer_size = PATH_MAX;
    _NSGetExecutablePath(buffer, &buffer_size);
    buffer[buffer_size] = '\0';
    return buffer;
#endif
}

} // namespace Paths

} // namespace banjo
