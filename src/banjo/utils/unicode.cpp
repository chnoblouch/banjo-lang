#include "unicode.hpp"

#include <bit>

namespace banjo::unicode {

std::optional<unsigned> decode_utf8(std::string_view array) {
    unsigned c1 = static_cast<unsigned>(array[0]);

    if ((c1 & 0x80) == 0) {
        return c1;
    } else if ((c1 & 0xE0) == 0xC0) {
        unsigned c2 = static_cast<unsigned>(array[1]);
        return ((c1 & 0x1F) << 6) | (c2 & 0x3F);
    } else if ((c1 & 0xF0) == 0xE0) {
        unsigned c2 = static_cast<unsigned>(array[1]);
        unsigned c3 = static_cast<unsigned>(array[2]);
        return ((c1 & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
    } else if ((c1 & 0xF8) == 0xF0) {
        unsigned c2 = static_cast<unsigned>(array[1]);
        unsigned c3 = static_cast<unsigned>(array[2]);
        unsigned c4 = static_cast<unsigned>(array[3]);
        return ((c1 & 0x07) << 18) | ((c2 & 0x3F) << 12) | ((c3 & 0x3F) << 6) | (c4 & 0x3F);
    } else {
        return {};
    }
}

std::optional<unsigned> validate_utf8(std::string_view string) {
    unsigned index = 0;
    unsigned length = 0;

    while (index < string.size()) {
        unsigned char c1 = std::bit_cast<unsigned char>(string[index]);

        if ((c1 & 0x80) == 0) {
            index += 1;
        } else if ((c1 & 0xE0) == 0xC0) {
            if (index + 1 >= string.size()) {
                return {};
            }

            char c2 = std::bit_cast<unsigned char>(string[index + 1]);

            if ((c2 & 0xC0) != 0x80) {
                return {};
            }

            if ((c1 == 0xC0) || (c1 == 0xC1)) {
                return {};
            }

            index += 2;
        } else if ((c1 & 0xF0) == 0xE0) {
            if (index + 2 >= string.length()) {
                return {};
            }

            unsigned char c2 = std::bit_cast<unsigned char>(string[index + 1]);
            unsigned char c3 = std::bit_cast<unsigned char>(string[index + 2]);

            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) {
                return {};
            }

            if (c1 == 0xE0 && c2 < 0xA0) {
                return {};
            }

            if (c1 == 0xED && c2 > 0x99) {
                return {};
            }

            index += 3;
        } else if ((c1 & 0xF8) == 0xF0) {
            if (index + 3 >= string.size()) {
                return {};
            }

            unsigned char c2 = std::bit_cast<unsigned char>(string[index + 1]);
            unsigned char c3 = std::bit_cast<unsigned char>(string[index + 2]);
            unsigned char c4 = std::bit_cast<unsigned char>(string[index + 3]);

            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) {
                return {};
            }

            if (c1 == 0xF0 && c2 < 0x90) {
                return {};
            }

            if ((c1 == 0xF4 && c2 > 0x8F) || c1 > 0xF4) {
                return {};
            }

            index += 4;
        } else {
            return {};
        }

        length += 1;
    }

    return length;
}

bool is_utf8_boundary(char byte) {
    return (byte & 0xC0) != 0x80;
}

} // namespace banjo::unicode
