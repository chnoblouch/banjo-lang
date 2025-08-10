#include "config.hpp"

#include "banjo/target/target_description.hpp"

namespace banjo {

namespace lang {

Config &Config::instance() {
    static Config instance;
    return instance;
}

bool Config::is_stdlib_enabled() {
    return !target.is_wasm() || target.get_operating_system() == target::OperatingSystem::EMSCRIPTEN;
}

} // namespace lang

} // namespace banjo
