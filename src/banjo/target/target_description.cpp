#include "target_description.hpp"

namespace banjo {

namespace target {

TargetDescription::TargetDescription()
  : architecture(Architecture::INVALID),
    operating_system(OperatingSystem::INVALID) {}

TargetDescription::TargetDescription(
    Architecture architecture,
    OperatingSystem operating_system,
    Environment environment /* = Environment::NONE */
)
  : architecture(architecture),
    operating_system(operating_system),
    environment(environment) {}

std::string TargetDescription::to_string() const {
    std::string arch_name;
    switch (architecture) {
        case Architecture::INVALID: arch_name = "invalid"; break;
        case Architecture::X86_64: arch_name = "x86_64"; break;
        case Architecture::AARCH64: arch_name = "aarch64"; break;
    }

    std::string os_name;
    switch (operating_system) {
        case OperatingSystem::INVALID: os_name = "invalid"; break;
        case OperatingSystem::WINDOWS: os_name = "windows"; break;
        case OperatingSystem::LINUX: os_name = "linux"; break;
        case OperatingSystem::MACOS: os_name = "macos"; break;
        case OperatingSystem::ANDROID: os_name = "android"; break;
        case OperatingSystem::IOS: os_name = "ios"; break;
    }

    if (environment == Environment::NONE) {
        return arch_name + "-" + os_name;
    }

    std::string env_name;
    switch (environment) {
        case Environment::NONE: break;
        case Environment::MSVC: env_name = "msvc"; break;
        case Environment::GNU: env_name = "gnu"; break;
    }

    return arch_name + "-" + os_name + "-" + env_name;
}

bool TargetDescription::is_windows() const {
    return operating_system == OperatingSystem::WINDOWS;
}

bool TargetDescription::is_unix() const {
    return operating_system == OperatingSystem::LINUX;
}

bool TargetDescription::is_darwin() const {
    return operating_system == OperatingSystem::MACOS || operating_system == OperatingSystem::IOS;
}

} // namespace target

} // namespace banjo
