#ifndef BANJO_UTILS_BIT_SET_H
#define BANJO_UTILS_BIT_SET_H

#include <bit>
#include <cstdint>
#include <limits>
#include <vector>

namespace banjo {

class BitSet {

private:
    typedef std::uint64_t Word;

    static constexpr unsigned BITS_PER_WORD = std::numeric_limits<Word>::digits;

public:
    class Iterator {

    private:
        const std::vector<Word> &words;
        unsigned position;

    public:
        Iterator(const std::vector<Word> &words, unsigned position);
        unsigned operator*() const;
        Iterator &operator++();
        bool operator==(const Iterator &other) const;
        bool operator!=(const Iterator &other) const;

    private:
        void advance();
    };

private:
    std::vector<Word> words;

public:
    bool get(unsigned position) const;
    unsigned size() const;

    void set(unsigned position);
    void clear(unsigned position);
    void union_with(const BitSet &other);

    Iterator begin() const { return Iterator(words, 0); }
    Iterator end() const { return Iterator(words, words.size() * BITS_PER_WORD); }

private:
    static unsigned get_word_index(unsigned position) { return position / BITS_PER_WORD; }
    static Word get_mask(unsigned position) { return 1ull << (position % BITS_PER_WORD); }
};

} // namespace banjo

#endif
