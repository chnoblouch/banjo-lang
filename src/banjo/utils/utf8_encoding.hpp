#ifndef BANJO_UTILS_UTF8_ENCODING_H
#define BANJO_UTILS_UTF8_ENCODING_H

#include <cstdint>

namespace banjo {

namespace UTF8Encoding {

bool is_first_byte_of_char(std::uint8_t byte);

} // namespace UTF8Encoding

} // namespace banjo

#endif
