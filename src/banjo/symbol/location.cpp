#include "location.hpp"

namespace lang {

void Location::add_element(LocationElement element) {
    elements.push_back(element);
}

void Location::remove_last_element() {
    elements.pop_back();
}

void Location::replace_last_element(LocationElement element) {
    elements.back() = element;
}

DataType *Location::get_type() const {
    if (!has_root()) {
        return nullptr;
    }

    return elements.back().get_type();
}

Function *Location::get_func() const {
    if (!has_root() || !elements.back().is_func()) {
        return nullptr;
    }

    return elements.back().get_func();
}

bool Location::has_root() const {
    return !elements.empty();
}

const LocationElement &Location::get_last_element() const {
    return elements.back();
}

} // namespace lang
