#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string_view>

namespace banjo {

namespace Utils {

int align(int value, int boundary);

template <typename T>
bool is_one_of(T value, std::initializer_list<T> candidates) {
    for (T candidate : candidates) {
        if (value == candidate) {
            return true;
        }
    }

    return false;
}

std::optional<std::uint64_t> parse_u64(std::string_view string);

} // namespace Utils

} // namespace banjo

#endif
