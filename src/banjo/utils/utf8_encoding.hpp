#ifndef UTF8_ENCODING_H
#define UTF8_ENCODING_H

#include <cstdint>

namespace UTF8Encoding {

bool is_first_byte_of_char(std::uint8_t byte);

} // namespace UTF8Encoding

#endif
