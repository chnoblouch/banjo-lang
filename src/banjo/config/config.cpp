#include "config.hpp"

namespace banjo {

namespace lang {

Config &Config::instance() {
    static Config instance;
    return instance;
}

} // namespace lang

} // namespace banjo
