#include "utf8_encoding.hpp"

namespace banjo {

bool UTF8Encoding::is_first_byte_of_char(std::uint8_t byte) {
    return (byte & 0xC0) != 0x80;
}

} // namespace banjo
