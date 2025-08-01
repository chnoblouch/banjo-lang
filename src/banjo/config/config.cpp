#include "config.hpp"

namespace banjo {

namespace lang {

Config &Config::instance() {
    static Config instance;
    return instance;
}

bool Config::is_stdlib_enabled() {
    return !target.is_wasm();
}

} // namespace lang

} // namespace banjo
