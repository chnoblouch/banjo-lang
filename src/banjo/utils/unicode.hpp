#ifndef BANJO_UTILS_UNICODE_H
#define BANJO_UTILS_UNICODE_H

#include <optional>
#include <string_view>

namespace banjo::unicode {

std::optional<unsigned> decode_utf8(std::string_view array);
std::optional<unsigned> validate_utf8(std::string_view string);
bool is_utf8_boundary(char byte);

} // namespace banjo::unicode

#endif
