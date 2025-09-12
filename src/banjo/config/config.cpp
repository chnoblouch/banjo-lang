#include "config.hpp"

#include "banjo/target/target_description.hpp"

namespace banjo {

namespace lang {

Config &Config::instance() {
    static Config instance;
    return instance;
}

bool Config::is_stdlib_enabled() {
    if (disable_std) {
        return false;
    }

    if (target.is_wasm()) {
        return target.get_operating_system() == target::OperatingSystem::EMSCRIPTEN;
    }

    return true;
}

} // namespace lang

} // namespace banjo
