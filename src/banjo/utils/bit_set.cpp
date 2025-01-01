#include "bit_set.hpp"

namespace banjo {

bool BitSet::get(unsigned position) const {
    unsigned word_index = get_word_index(position);
    if (word_index >= words.size()) {
        return false;
    }

    return words[word_index] & get_mask(position);
}

unsigned BitSet::size() const {
    unsigned size = 0;

    for (const Word &word : words) {
        size += std::popcount(word);
    }

    return size;
}

void BitSet::set(unsigned position) {
    unsigned word_index = get_word_index(position);
    if (word_index >= words.size()) {
        words.resize(word_index + 1);
    }

    words[word_index] |= get_mask(position);
}

void BitSet::clear(unsigned position) {
    unsigned word_index = get_word_index(position);
    if (word_index >= words.size()) {
        return;
    }

    words[word_index] &= ~get_mask(position);
}

void BitSet::union_with(const BitSet &other) {
    if (words.size() < other.words.size()) {
        words.resize(other.words.size());
    }

    for (unsigned i = 0; i < other.words.size(); i++) {
        words[i] |= other.words[i];
    }
}

BitSet::Iterator::Iterator(const std::vector<Word> &words, unsigned position) : words(words), position(position) {
    advance();
}

unsigned BitSet::Iterator::operator*() const {
    return position;
}

BitSet::Iterator &BitSet::Iterator::operator++() {
    position += 1;
    advance();
    return *this;
}

bool BitSet::Iterator::operator==(const Iterator &other) const {
    return position == other.position;
}

bool BitSet::Iterator::operator!=(const Iterator &other) const {
    return !(*this == other);
}

void BitSet::Iterator::advance() {
    unsigned word_index = get_word_index(position);
    Word mask = get_mask(position);

    while (word_index < words.size() && !(words[word_index] & mask)) {
        position += 1;
        word_index = get_word_index(position);
        mask = get_mask(position);
    }
}

} // namespace banjo
