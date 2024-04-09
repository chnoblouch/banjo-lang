#ifndef SOFT_INT_H
#define SOFT_INT_H

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

template <int Bits, typename Word = std::uint64_t>
class SoftInt {

public:
    struct DivResult {
        SoftInt quotient;
        SoftInt remainder;
    };

private:
    static constexpr int NUM_WORDS = Bits / std::numeric_limits<Word>::digits;
    Word words[NUM_WORDS];

public:
    SoftInt() {
        for (int i = 0; i < NUM_WORDS; i++) {
            words[i] = 0;
        }
    }

    SoftInt(std::int64_t value);
    SoftInt(std::string value);

    Word *get_words() { return words; }

    SoftInt negated() const;
    SoftInt add(const SoftInt &other) const;
    SoftInt sub(const SoftInt &other) const;
    SoftInt mul(const SoftInt &other) const;
    DivResult div(const SoftInt &other) const;

    bool equal(const SoftInt &rhs);
    bool gt(const SoftInt &rhs);
    bool lt(const SoftInt &rhs);
    bool ge(const SoftInt &rhs);
    bool le(const SoftInt &rhs);

    std::string str();

    SoftInt operator-() const;

    friend SoftInt operator+(const SoftInt &lhs, const SoftInt &rhs);
    friend SoftInt operator-(const SoftInt &lhs, const SoftInt &rhs);
    friend SoftInt operator*(const SoftInt &lhs, const SoftInt &rhs);
    SoftInt operator/(const SoftInt &rhs) { return div(rhs).quotient; }
    SoftInt operator%(const SoftInt &rhs) { return div(rhs).remainder; }

    bool operator==(const SoftInt &rhs) { return equal(rhs); }
    bool operator!=(const SoftInt &rhs) { return !equal(rhs); }
    bool operator>(const SoftInt &rhs) { return gt(rhs); }
    bool operator<(const SoftInt &rhs) { return lt(rhs); }
    bool operator>=(const SoftInt &rhs) { return ge(rhs); }
    bool operator<=(const SoftInt &rhs) { return le(rhs); }

    bool is_zero() const;
    bool is_negative() const;
    std::int64_t trunc() const { return words[0]; }

private:
    int ucompare(const SoftInt &rhs) const;
    int scompare(const SoftInt &rhs) const;
};

template <int Bits, typename Word>
SoftInt<Bits, Word>::SoftInt(std::int64_t value) {
    words[0] = value;
    words[1] = ((value >> 63) == 1) ? 0xFFFFFFFFFFFFFFFF : 0;
}

template <int Bits, typename Word>
SoftInt<Bits, Word>::SoftInt(std::string value) {
    for (int i = 0; i < NUM_WORDS; i++) {
        words[i] = 0;
    }

    int base = 10;
    if (value.starts_with("0x")) {
        value = value.substr(2);
        base = 16;
    }

    SoftInt place_value = 1;
    for (int i = (int)value.size() - 1; i >= 0; i--) {
        char c = value[i];
        int digit_val;

        if (c >= '0' && c <= '9') {
            digit_val = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit_val = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit_val = c - 'A' + 10;
        }

        *this = add(place_value.mul(digit_val));
        place_value = place_value.mul(base);
    }
}

template <int Bits, typename Word>
SoftInt<Bits, Word> SoftInt<Bits, Word>::negated() const {
    SoftInt result;

    for (int i = 0; i < NUM_WORDS; i++) {
        result.words[i] = ~words[i];
    }

    return result.add(1);
}

template <int Bits, typename Word>
SoftInt<Bits, Word> SoftInt<Bits, Word>::add(const SoftInt &other) const {
    SoftInt<Bits, Word> result;
    Word carry = 0;

    for (int i = 0; i < SoftInt<Bits, Word>::NUM_WORDS; i++) {
        result.words[i] = words[i] + other.words[i] + carry;
        if (result.words[i] < words[i]) {
            carry = 1;
        }
    }

    return result;
}

template <int Bits, typename Word>
SoftInt<Bits, Word> SoftInt<Bits, Word>::sub(const SoftInt &other) const {
    return add(other.negated());
}

template <int Bits, typename Word>
SoftInt<Bits, Word> SoftInt<Bits, Word>::mul(const SoftInt &other) const {
    SoftInt<Bits, Word> result;

    for (int i = 0; i < NUM_WORDS; i++) {
        Word left_word = words[i];

        for (int j = 0; j < NUM_WORDS; j++) {
            Word right_word = other.words[j];
            result = result.add(left_word * right_word);
        }
    }

    return result;
}

template <int Bits, typename Word>
typename SoftInt<Bits, Word>::DivResult SoftInt<Bits, Word>::div(const SoftInt &other) const {
    SoftInt<Bits, Word> quotient;
    SoftInt<Bits, Word> remainder = *this;

    while (remainder >= other) {
        quotient = quotient.add(1);
        remainder = remainder.sub(other);
    }

    return {quotient, remainder};
}

template <int Bits, typename Word>
SoftInt<Bits, Word> SoftInt<Bits, Word>::operator-() const {
    return negated();
}

template <int Bits, typename Word>
SoftInt<Bits, Word> operator+(const SoftInt<Bits, Word> &lhs, const SoftInt<Bits, Word> &rhs) {
    return lhs.add(rhs);
}

template <int Bits, typename Word>
SoftInt<Bits, Word> operator-(const SoftInt<Bits, Word> &lhs, const SoftInt<Bits, Word> &rhs) {
    return lhs.sub(rhs);
}

template <int Bits, typename Word>
SoftInt<Bits, Word> operator*(const SoftInt<Bits, Word> &lhs, const SoftInt<Bits, Word> &rhs) {
    return lhs.add(rhs);
}

template <int Bits, typename Word>
typename SoftInt<Bits, Word>::DivResult operator/(const SoftInt<Bits, Word> &lhs, const SoftInt<Bits, Word> &rhs) {
    return lhs.div(rhs);
}

template <int Bits, typename Word>
bool SoftInt<Bits, Word>::equal(const SoftInt<Bits, Word> &rhs) {
    for (int i = 0; i < NUM_WORDS; i++) {
        if (words[i] != rhs.words[i]) {
            return false;
        }
    }

    return true;
}

template <int Bits, typename Word>
bool SoftInt<Bits, Word>::gt(const SoftInt<Bits, Word> &rhs) {
    return scompare(rhs) == 1;
}

template <int Bits, typename Word>
bool SoftInt<Bits, Word>::lt(const SoftInt<Bits, Word> &rhs) {
    return scompare(rhs) == -1;
}

template <int Bits, typename Word>
bool SoftInt<Bits, Word>::ge(const SoftInt<Bits, Word> &rhs) {
    return scompare(rhs) != -1;
}

template <int Bits, typename Word>
bool SoftInt<Bits, Word>::le(const SoftInt<Bits, Word> &rhs) {
    return scompare(rhs) != 1;
}

template <int Bits, typename Word>
int SoftInt<Bits, Word>::ucompare(const SoftInt<Bits, Word> &rhs) const {
    for (int i = 0; i < SoftInt<Bits, Word>::NUM_WORDS; i++) {
        if (words[i] > rhs.words[i]) {
            return 1;
        } else if (words[i] < rhs.words[i]) {
            return -1;
        }
    }

    return 0;
}

template <int Bits, typename Word>
int SoftInt<Bits, Word>::scompare(const SoftInt<Bits, Word> &rhs) const {
    if (is_negative() != rhs.is_negative()) {
        return is_negative() ? -1 : 1;
    }

    return ucompare(rhs);
}

template <int Bits, typename Word>
bool SoftInt<Bits, Word>::is_zero() const {
    for (int i = 0; i < NUM_WORDS; i++) {
        if (words[i] != 0) {
            return false;
        }
    }

    return true;
}

template <int Bits, typename Word>
bool SoftInt<Bits, Word>::is_negative() const {
    return (words[0] >> 63) == 1;
}

template <int Bits, typename Word>
std::string SoftInt<Bits, Word>::str() {
    std::string result;

    SoftInt value = *this;
    while (!value.is_zero()) {
        SoftInt remainder = value % 10;
        result += remainder.words[0] + '0';
        value = value / 10;
    }

    std::reverse(result.begin(), result.end());
    return result;
}

#endif
