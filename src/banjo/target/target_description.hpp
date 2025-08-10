#ifndef BANJO_TARGET_DESCRIPTION
#define BANJO_TARGET_DESCRIPTION

#include <string>

namespace banjo {

namespace target {

enum class Architecture {
    INVALID,
    X86_64,
    AARCH64,
    WASM,
};

enum class OperatingSystem {
    INVALID,
    WINDOWS,
    LINUX,
    MACOS,
    ANDROID,
    IOS,
    EMSCRIPTEN,
    UNKNOWN,
};

enum class Environment {
    NONE,
    MSVC,
    GNU,
};

class TargetDescription {

private:
    Architecture architecture;
    OperatingSystem operating_system;
    Environment environment;

public:
    TargetDescription();

    TargetDescription(
        Architecture architecture,
        OperatingSystem operating_system,
        Environment environment = Environment::NONE
    );

    Architecture get_architecture() const { return architecture; }
    OperatingSystem get_operating_system() const { return operating_system; }
    Environment get_environment() const { return environment; }

    std::string to_string() const;

    bool is_windows() const;
    bool is_unix() const;
    bool is_darwin() const;
    bool is_wasm() const;
};

} // namespace target

} // namespace banjo

#endif
