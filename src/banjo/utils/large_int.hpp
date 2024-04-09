#ifndef LARGE_INT_H
#define LARGE_INT_H

#include <compare>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

class LargeInt {

private:
    std::uint64_t magnitude;
    bool negative;

public:
    LargeInt(std::uint64_t magnitude, bool negative);
    LargeInt(int value);
    LargeInt(unsigned value);
    LargeInt(const std::string &string);
    LargeInt(const char *string);
    LargeInt(std::string_view string);

    std::uint64_t get_magnitude() { return magnitude; }
    bool is_negative() { return negative; }
    bool is_positive() { return !negative; }

    std::string to_string() const;
    std::int64_t to_s64() const;
    std::int64_t to_u64() const;
    std::int32_t to_s32() const;
    std::uint64_t to_bits() const;

    friend LargeInt operator-(const LargeInt &large_int);
    friend LargeInt operator+(const LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt operator-(const LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt operator*(const LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt operator/(const LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt operator%(const LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt operator&(const LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt operator|(const LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt operator^(const LargeInt &lhs, const LargeInt &rhs);

    friend bool operator==(const LargeInt &lhs, const LargeInt &rhs);
    friend std::strong_ordering operator<=>(const LargeInt &lhs, const LargeInt &rhs);

    friend LargeInt &operator+=(LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt &operator/=(LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt &operator*=(LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt &operator/=(LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt &operator%=(LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt &operator&=(LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt &operator|=(LargeInt &lhs, const LargeInt &rhs);
    friend LargeInt &operator^=(LargeInt &lhs, const LargeInt &rhs);

    friend std::ostream &operator<<(std::ostream &stream, const LargeInt &large_int);
};

#endif
