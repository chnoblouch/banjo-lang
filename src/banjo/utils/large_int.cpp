#include "large_int.hpp"

#include <algorithm>
#include <cassert>
#include <limits>

namespace banjo {

LargeInt::LargeInt(std::uint64_t magnitude, bool negative) : magnitude(magnitude), negative(negative) {
    if (magnitude == 0 && negative) {
        negative = false;
    }
}

LargeInt::LargeInt(int value) {
    if (value >= 0) {
        magnitude = value;
        negative = false;
    } else {
        magnitude = -value;
        negative = true;
    }
}

LargeInt::LargeInt(unsigned value) : magnitude(value), negative(false) {}

LargeInt::LargeInt(const std::string &string) : LargeInt(std::string_view{string}) {}

LargeInt::LargeInt(const char *string) : LargeInt(std::string_view{string}) {}

LargeInt::LargeInt(std::string_view string) {
    int start_index = 0;

    if (string[0] == '-') {
        negative = true;
        start_index = 1;
    } else {
        negative = false;
    }

    unsigned base = 10;

    if (string.length() >= (start_index + 2) && string[start_index] == '0') {
        char c = string[start_index + 1];

        if (c == 'x' || c == 'X') {
            base = 16;
            start_index += 2;
        } else if (c == 'b' || c == 'B') {
            base = 2;
            start_index += 2;
        }
    }

    std::uint64_t multiplier = 1;
    magnitude = 0;

    for (int i = (int)string.length() - 1; i >= start_index; i--) {
        char c = string[i];
        unsigned digit;

        if (c >= '0' && c <= '9') {
            digit = (unsigned)string[i] - (unsigned)'0';
        } else if (c >= 'a' && c <= 'f') {
            digit = 10 + (unsigned)string[i] - (unsigned)'a';
        } else if (c >= 'A' && c <= 'F') {
            digit = 10 + (unsigned)string[i] - (unsigned)'A';
        } else {
            digit = 0;
        }

        magnitude += multiplier * digit;
        multiplier *= base;
    }

    if (magnitude == 0 && negative) {
        negative = false;
    }
}

std::string LargeInt::to_string() const {
    if (magnitude == 0) {
        return "0";
    }

    std::string result;

    for (std::uint64_t value = magnitude; value > 0; value /= 10) {
        result += (char)(value % 10 + (std::uint64_t)'0');
    }

    if (negative) {
        result += '-';
    }

    std::reverse(result.begin(), result.end());
    return result;
}

std::int64_t LargeInt::to_s64() const {
    if (negative) {
        return -magnitude;
    } else {
        return magnitude;
    }
}

std::int64_t LargeInt::to_u64() const {
    assert(!negative);
    return magnitude;
}

std::int32_t LargeInt::to_s32() const {
    return (std::int32_t)to_s64();
}

std::uint64_t LargeInt::to_bits() const {
    if (!negative) {
        return magnitude;
    } else {
        return std::numeric_limits<std::uint64_t>::max() - (magnitude - 1);
    }
}

LargeInt operator-(const LargeInt &large_int) {
    return {large_int.magnitude, !large_int.negative};
}

LargeInt operator+(const LargeInt &lhs, const LargeInt &rhs) {
    std::uint64_t magnitude;
    bool negative = lhs.negative;

    if (lhs.negative == rhs.negative) {
        magnitude = lhs.magnitude + rhs.magnitude;
    } else {
        if (rhs.magnitude <= lhs.magnitude) {
            magnitude = lhs.magnitude - rhs.magnitude;
        } else {
            magnitude = rhs.magnitude - lhs.magnitude;
            negative = !negative;
        }
    }

    return {magnitude, negative};
}

LargeInt operator-(const LargeInt &lhs, const LargeInt &rhs) {
    return lhs + (-rhs);
}

LargeInt operator*(const LargeInt &lhs, const LargeInt &rhs) {
    std::uint64_t magnitude = lhs.magnitude * rhs.magnitude;
    bool negative = lhs.negative != rhs.negative;
    return {magnitude, negative};
}

LargeInt operator/(const LargeInt &lhs, const LargeInt &rhs) {
    std::uint64_t magnitude = lhs.magnitude / rhs.magnitude;
    bool negative = lhs.negative != rhs.negative;
    return {magnitude, negative};
}

LargeInt operator%(const LargeInt &lhs, const LargeInt &rhs) {
    std::uint64_t magnitude = lhs.magnitude % rhs.magnitude;
    bool negative = lhs.negative != rhs.negative;
    return {magnitude, negative};
}

LargeInt operator&(const LargeInt &lhs, const LargeInt &rhs) {
    return {lhs.to_bits() & rhs.to_bits(), false};
}

LargeInt operator|(const LargeInt &lhs, const LargeInt &rhs) {
    return {lhs.to_bits() | rhs.to_bits(), false};
}

LargeInt operator^(const LargeInt &lhs, const LargeInt &rhs) {
    return {lhs.to_bits() ^ rhs.to_bits(), false};
}

LargeInt &operator+=(LargeInt &lhs, const LargeInt &rhs) {
    lhs = lhs + rhs;
    return lhs;
}

LargeInt &operator-=(LargeInt &lhs, const LargeInt &rhs) {
    lhs = lhs - rhs;
    return lhs;
}

LargeInt &operator*=(LargeInt &lhs, const LargeInt &rhs) {
    lhs = lhs * rhs;
    return lhs;
}

LargeInt &operator/=(LargeInt &lhs, const LargeInt &rhs) {
    lhs = lhs / rhs;
    return lhs;
}

LargeInt &operator%=(LargeInt &lhs, const LargeInt &rhs) {
    lhs = lhs % rhs;
    return lhs;
}

LargeInt &operator&=(LargeInt &lhs, const LargeInt &rhs) {
    lhs = lhs & rhs;
    return lhs;
}

LargeInt &operator|=(LargeInt &lhs, const LargeInt &rhs) {
    lhs = lhs | rhs;
    return lhs;
}

LargeInt &operator^=(LargeInt &lhs, const LargeInt &rhs) {
    lhs = lhs ^ rhs;
    return lhs;
}

bool operator==(const LargeInt &lhs, const LargeInt &rhs) {
    return lhs.magnitude == rhs.magnitude && lhs.negative == rhs.negative;
}

std::strong_ordering operator<=>(const LargeInt &lhs, const LargeInt &rhs) {
    if (lhs.negative == rhs.negative) {
        if (lhs.magnitude == rhs.magnitude) {
            return std::strong_ordering::equal;
        } else if (lhs.magnitude > rhs.magnitude) {
            return lhs.negative ? std::strong_ordering::less : std::strong_ordering::greater;
        } else {
            return lhs.negative ? std::strong_ordering::greater : std::strong_ordering::less;
        }
    } else if (lhs.negative) {
        return std::strong_ordering::less;
    } else {
        return std::strong_ordering::greater;
    }
}

std::ostream &operator<<(std::ostream &stream, const LargeInt &large_int) {
    return stream << large_int.to_string();
}

} // namespace banjo
