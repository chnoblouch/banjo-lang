#include "config.hpp"

namespace lang {

Config &Config::instance() {
    static Config instance;
    return instance;
}

} // namespace lang
