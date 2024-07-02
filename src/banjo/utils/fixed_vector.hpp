#ifndef FIXED_VECTOR_H
#define FIXED_VECTOR_H

#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <utility>

namespace banjo {

template <typename T, unsigned Capacity>
class FixedVector {

public:
    typedef T *Iterator;
    typedef const T *ConstIterator;

private:
    T *elements;
    std::size_t size;

public:
    FixedVector() : elements(nullptr), size(0) {}

    FixedVector(std::initializer_list<T> elements) : elements(new T[Capacity]), size(0) {
        for (const T &element : elements) {
            append(element);
        }
    }

    FixedVector(const FixedVector &other) : elements(new T[Capacity]), size(0) {
        for (const T &element : other) {
            append(element);
        }
    }

    FixedVector(FixedVector &&other) noexcept : elements(std::exchange(other.elements, nullptr)), size(other.size) {}

    ~FixedVector() { delete[] elements; }

    FixedVector &operator=(const FixedVector &other) { return *this = FixedVector(other); }

    FixedVector &operator=(FixedVector &&other) noexcept {
        std::swap(elements, other.elements);
        std::swap(size, other.size);
        return *this;
    }

    Iterator begin() { return &elements[0]; }
    ConstIterator begin() const { return &elements[0]; }
    Iterator end() { return &elements[size]; }
    ConstIterator end() const { return &elements[size]; }

    T &operator[](std::size_t index) { return elements[index]; }
    std::size_t get_size() { return size; }

    void append(const T &element) {
        assert(size < Capacity && "fixed vector overflow");
        elements[size++] = element;
    }

    void clear() { size = 0; }
};

} // namespace banjo

#endif
