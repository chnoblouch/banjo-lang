#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <string>

namespace banjo {

namespace hot_reloader {

namespace Diagnostics {

void log(const std::string &message);
void abort(const std::string &message);

} // namespace Diagnostics

} // namespace hot_reloader

} // namespace banjo

#endif
