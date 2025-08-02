#ifndef BANJO_UTILS_H
#define BANJO_UTILS_H

#include "banjo/utils/fixed_vector.hpp"
#include "banjo/utils/large_int.hpp"

#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <vector>

namespace banjo {

namespace Utils {

typedef FixedVector<unsigned char, 16> LEB128Buffer;

template <typename T>
T align(T value, T boundary) {
    if (boundary == 0) {
        return value;
    }

    T mod = value % boundary;
    return mod == 0 ? value : value + boundary - mod;
}

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
std::vector<std::string_view> split_string(std::string_view string, char delimiter);
std::string convert_eol_to_lf(std::string_view string);

LEB128Buffer encode_uleb128(std::uint64_t value);
LEB128Buffer encode_sleb128(LargeInt value);

template <typename T>
std::vector<T> remove_duplicates(const std::vector<T> &array) {
    std::vector<T> result;

    for (const T &element : array) {
        bool is_duplicate = false;

        for (const T &other : result) {
            if (other == element) {
                is_duplicate = true;
                break;
            }
        }

        if (!is_duplicate) {
            result.push_back(element);
        }
    }

    return result;
}

std::optional<std::string> read_string_file(const std::filesystem::path &path);
bool write_string_file(std::string_view contents, const std::filesystem::path &path);
std::optional<std::string_view> get_env(const std::string &name);

} // namespace Utils

} // namespace banjo

#endif
