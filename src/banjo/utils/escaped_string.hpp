#ifndef BANJO_UTILS_ESCAPED_STRING_H
#define BANJO_UTILS_ESCAPED_STRING_H

#include "banjo/utils/expected.hpp"

#include <string>

namespace banjo::escaped_string {

Expected<std::string, unsigned> decode(std::string_view encoded);

} // namespace banjo::escaped_string

#endif
