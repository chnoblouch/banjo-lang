#include "utils.hpp"

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
        if (string[index] == delimiter) {
            if (index != start) {
                components.push_back(string.substr(start, index - start));
            }

            start = index + 1;
        }

        index += 1;

        if (index == string.size()) {
            if (index += start) {
                components.push_back(string.substr(start, index - start));
            }

            break;
        }
    }

    return components;
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
    return buffer;
}

} // namespace Utils

} // namespace banjo
