#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include "banjo/sir/sir.hpp"

#include <string>

namespace banjo {

namespace hot_reloader {

namespace Diagnostics {

void log(const std::string &message);
[[noreturn]] void abort(const std::string &message);

std::string symbol_to_string(lang::sir::Symbol symbol);

} // namespace Diagnostics

} // namespace hot_reloader

} // namespace banjo

#endif
