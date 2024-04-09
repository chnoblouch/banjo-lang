#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <string>

namespace Diagnostics {

void log(const std::string &message);
void abort(const std::string &message);

} // namespace Diagnostics

#endif
