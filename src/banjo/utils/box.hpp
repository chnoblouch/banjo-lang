#ifndef BOX_H
#define BOX_H

#include <utility>

template <typename T>
class Box {

private:
    T *value;

public:
    Box() : value(nullptr) {}
    Box(T value) : value(new T(value)) {}
    Box(const Box &other) : value(new T(*other.value)) {}
    Box(Box &&other) : value(std::exchange(other.value, nullptr)) {}
    ~Box() { delete value; }

    T &get_value() const { return *value; }

    Box &operator=(const Box &other) { return *this = Box(other); }

    Box &operator=(Box &&other) noexcept {
        std::swap(value, other.value);
        return *this;
    }

    T &operator*() const { return get_value(); }
};

#endif
