#include "escaped_string.hpp"

#include <optional>

namespace banjo::escaped_string {

static std::optional<unsigned> decode_hex(char c) {
    if (c >= '0' && c <= '9') {
        return static_cast<unsigned>(c - '0');
    } else if (c >= 'a' && c <= 'f') {
        return 10 + static_cast<unsigned>(c - 'a');
    } else if (c >= 'A' && c <= 'F') {
        return 10 + static_cast<unsigned>(c - 'A');
    } else {
        return {};
    }
}

static std::optional<char> decode_hex_escape(std::string_view encoded, unsigned index) {
    if (index + 3 >= encoded.size()) {
        return {};
    }

    std::optional<unsigned> h = decode_hex(encoded[index + 2]);
    std::optional<unsigned> l = decode_hex(encoded[index + 3]);

    if (!h || !l) {
        return {};
    }

    return static_cast<char>(*h << 4 | *l);
}

Expected<std::string, unsigned> decode(std::string_view encoded) {
    std::string result;
    unsigned index = 0;

    while (index < encoded.size()) {
        char c = encoded[index];

        if (c != '\\') {
            result.push_back(c);
            index += 1;
            continue;
        }

        if (index == encoded.size() - 1) {
            return index;
        }

        c = encoded[index + 1];

        switch (c) {
            case 'n':
                result.push_back(0x0A);
                index += 2;
                break;
            case 'r':
                result.push_back(0x0D);
                index += 2;
                break;
            case 't':
                result.push_back(0x09);
                index += 2;
                break;
            case '0':
                result.push_back(0x00);
                index += 2;
                break;
            case '\\':
                result.push_back('\\');
                index += 2;
                break;
            case '\'':
                result.push_back('\'');
                index += 2;
                break;
            case '\"':
                result.push_back('\"');
                index += 2;
                break;
            case 'x':
                if (auto decoded = decode_hex_escape(encoded, index)) {
                    result.push_back(*decoded);
                    index += 4;
                    break;
                } else {
                    return index;
                }
            default: return index;
        }
    }

    return result;
}

} // namespace banjo::escaped_string
