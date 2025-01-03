#include "module_path.hpp"

namespace banjo {

namespace lang {

ModulePath::ModulePath() {}

ModulePath::ModulePath(std::initializer_list<std::string> elements) {
    for (const std::string &element : elements) {
        append(element);
    }
}

void ModulePath::append(std::string_view element) {
    if (!value.empty()) {
        value += '.';
    }

    value += element;
}

unsigned ModulePath::get_size() const {
    if (is_empty()) {
        return 0;
    }

    unsigned size = 1;

    for (char c : value) {
        if (c == '.') {
            size += 1;
        }
    }

    return size;
}

bool ModulePath::is_empty() const {
    return value.empty();
}

std::string_view ModulePath::to_string() const {
    return value;
}

std::string ModulePath::to_string(std::string_view delimiter) const {
    std::string string;

    for (char c : value) {
        if (c == '.') {
            string += delimiter;
        } else {
            string += c;
        }
    }

    return string;
}

std::string_view ModulePath::operator[](unsigned index) const {
    unsigned start = 0;

    for (unsigned i = 0; i < index; i++) {
        while (value[start] != '.') {
            start += 1;
        }

        start += 1;
    }

    unsigned end = start;

    while (end < value.size() && value[end] != '.') {
        end += 1;
    }

    return std::string_view(value).substr(start, end - start);
}

ModulePath::Iterator::Iterator(std::string_view value, unsigned index) : value(value), index(index) {}

std::string_view ModulePath::Iterator::operator*() const {
    unsigned end = index;

    while (end < value.size() && value[end] != '.') {
        end += 1;
    }

    return value.substr(index, end - index);
}

ModulePath::Iterator &ModulePath::Iterator::operator++() {
    while (true) {
        if (index == value.size()) {
            break;
        } else if (value[index] == '.') {
            index += 1;
            break;
        } else {
            index += 1;
        }
    }

    return *this;
}

bool ModulePath::Iterator::operator==(const Iterator &other) const {
    return index == other.index;
}

bool ModulePath::Iterator::operator!=(const Iterator &other) const {
    return !(*this == other);
}

} // namespace lang

} // namespace banjo
