#include "target.hpp"

#include "banjo/utils/platform.hpp"

#include <utility>

namespace banjo {
namespace cli {

Target::Target() : arch("unknown"), os("unknown") {}

Target::Target(std::string arch, std::string os) : arch(std::move(arch)), os(std::move(os)), env() {}

Target::Target(std::string arch, std::string os, std::optional<std::string> env)
  : arch(std::move(arch)),
    os(std::move(os)),
    env(std::move(env)) {}

Target Target::host() {
    std::string arch;

#define ARCH_X86_64 1

#if ARCH_X86_64
    arch = "x86_64";
#elif ARCH_AARCH64
    arch = "aarch64"
#else
    arch = "unknown";
#endif

    std::string os;

#if OS_WINDOWS
    os = "windows";
#elif OS_LINUX
    os = "linux";
#elif OS_MACOS
    os = "macos";
#else
    os = "unknown";
#endif

    std::optional<std::string> env = get_default_env(os);

    return Target(arch, os, env);
}

std::string Target::to_string() const {
    if (env) {
        return arch + "-" + os + "-" + *env;
    } else {
        return arch + "-" + os;
    }
}

std::vector<Target> Target::list_available() {
    return {
        Target("x86_64", "windows", "msvc"),
        Target("x86_64", "windows", "gnu"),
        Target("x86_64", "linux", "gnu"),
        Target("aarch64", "linux", "gnu"),
        Target("aarch64", "macos"),
    };
}

std::optional<std::string> Target::get_default_env(std::string os) {
    if (os == "windows") {
        return "msvc";
    } else if (os == "linux") {
        return "gnu";
    } else {
        return {};
    }
}

} // namespace cli
} // namespace banjo
