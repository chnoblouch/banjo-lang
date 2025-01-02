#include "utils.hpp"

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

} // namespace Utils

} // namespace banjo
