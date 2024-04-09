#ifndef STATIC_VECTOR_H
#define STATIC_VECTOR_H

#include <cassert>
#include <cstddef>
#include <initializer_list>

template <typename T, unsigned Capacity>
class StaticVector {

public:
    typedef T *Iterator;
    typedef const T *ConstIterator;

private:
    T elements[Capacity];
    std::size_t size;

public:
    StaticVector(std::initializer_list<T> elements) : size(0) {
        for (const T &element : elements) {
            append(element);
        }
    }

    Iterator begin() { return &elements[0]; }
    ConstIterator begin() const { return &elements[0]; }
    Iterator end() { return &elements[size]; }
    ConstIterator end() const { return &elements[size]; }

    T &operator[](std::size_t index) { return elements[index]; }
    std::size_t get_size() { return size; }

    void append(const T &element) {
        assert(size < Capacity && "static vector overflow");
        elements[size++] = element;
    }
};

#endif
