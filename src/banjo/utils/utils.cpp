#ifdef _WINDOWS
#    define _CRT_SECURE_NO_WARNINGS
#endif

#include "banjo/utils/utils.hpp"
#include "banjo/utils/macros.hpp"

#include <climits>
#include <cstdlib>
#include <fstream>

namespace banjo {

namespace Utils {

int align(int value, int boundary) {
    if (boundary == 0) {
        return value;
    }

    int mod = value % boundary;
    return mod == 0 ? value : value + boundary - mod;
}

std::optional<std::uint64_t> parse_u64(std::string_view string) {
    std::uint64_t value = 0;
    std::uint64_t multiplier = 1;

    for (int i = static_cast<int>(string.size()) - 1; i >= 0; i--) {
        if (string[i] >= '0' && string[i] <= '9') {
            value += multiplier * static_cast<std::uint64_t>(string[i] - '0');
        } else {
            return {};
        }

        multiplier *= 10;
    }

    return value;
}

std::vector<std::string_view> split_string(std::string_view string, char delimiter) {
    std::vector<std::string_view> components;
    unsigned start = 0;
    unsigned index = 0;

    while (true) {
        if (index == string.size()) {
            if (index != start) {
                components.push_back(string.substr(start, index - start));
            }

            break;
        }

        if (string[index] == delimiter) {
            if (index != start) {
                components.push_back(string.substr(start, index - start));
            }

            start = index + 1;
        }

        index += 1;
    }

    return components;
}

std::string convert_eol_to_lf(std::string_view string) {
    std::string result;
    result.reserve(string.size());

    for (unsigned i = 0; i < string.size(); i++) {
        if (i <= string.size() - 2 && string[i] == '\r' && string[i + 1] == '\n') {
            result.push_back('\n');
        } else {
            result.push_back(string[i]);
        }
    }

    return result;
}

LEB128Buffer encode_uleb128(std::uint64_t value) {
    // For reference: https://en.wikipedia.org/wiki/LEB128#Unsigned_LEB128

    LEB128Buffer buffer;
    std::uint64_t bits = value;
    bool last = false;

    while (!last) {
        std::uint8_t byte = bits & 0x7F;
        bits >>= 7;

        if (bits == 0) {
            last = true;
        } else {
            byte |= 0x80;
        }

        buffer.append(byte);
    }

    return buffer;
}

LEB128Buffer encode_sleb128(LargeInt value) {
    // For reference: https://en.wikipedia.org/wiki/LEB128#Signed_LEB128

    LEB128Buffer buffer;
    std::uint64_t bits = value.to_bits();
    bool last = false;

    while (!last) {
        std::uint8_t byte = bits & 0x7F;
        bool sign_bit = byte & 0x40;

        bits >>= 7;

        if (value.is_negative()) {
            bits |= 0x7Full << (sizeof(std::uint64_t) * CHAR_BIT - 7);
        }

        if ((bits == 0 && !sign_bit) || (bits == static_cast<std::uint64_t>(~0ull) && sign_bit)) {
            last = true;
        } else {
            byte |= 0x80;
        }

        buffer.append(byte);
    }

    return buffer;
}

std::optional<std::string> read_string_file(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);

    if (!stream) {
        return {};
    }

    std::size_t size = stream.tellg();
    stream.seekg(0);

    std::string buffer(size, '\0');
    stream.read(buffer.data(), size);

    if (!stream) {
        return {};
    }

    return buffer;
}

bool write_string_file(std::string_view contents, const std::filesystem::path &path) {
    std::ofstream stream(path, std::ios::binary);

    if (!stream) {
        return false;
    }

    stream.write(contents.data(), contents.size());
    return static_cast<bool>(stream);
}

std::optional<std::string_view> get_env(const std::string &name) {
    const char *result = std::getenv(name.c_str());

    if (result) {
        return result;
    } else {
        return {};
    }
}

} // namespace Utils

} // namespace banjo
