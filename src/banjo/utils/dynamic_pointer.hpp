#ifndef BANJO_UTILS_DYNAMIC_POINTER_H
#define BANJO_UTILS_DYNAMIC_POINTER_H

#include "banjo/utils/macros.hpp"

#include <type_traits>

namespace banjo {

template <typename... Types>
class DynamicPointer {

public:
    typedef unsigned Tag;

private:
    Tag tag;
    void *pointer;

public:
    template <typename T>
    DynamicPointer(T *pointer) {
        if (pointer) {
            tag = tag_of<T>();
            this->pointer = pointer;
        } else {
            tag = 0;
            this->pointer = nullptr;
        }
    }

    DynamicPointer() : tag(0), pointer(nullptr) {}
    DynamicPointer(std::nullptr_t) : tag(0), pointer(nullptr) {}

    Tag get_tag() const { return tag; }
    void *get_pointer() const { return pointer; }

    template <typename T>
    bool is() const {
        return tag == tag_of<T>();
    }

    template <typename T, typename... RemainingTypes>
    bool is_one_of() const {
        if constexpr (sizeof...(RemainingTypes) != 0) {
            return tag == tag_of<T>() || is_one_of<RemainingTypes...>();
        } else {
            return tag == tag_of<T>();
        }
    }

    template <typename T>
    T &as() {
        ASSERT(tag == tag_of<T>());
        return *reinterpret_cast<T *>(pointer);
    }

    template <typename T>
    const T &as() const {
        ASSERT(tag == tag_of<T>());
        return *reinterpret_cast<T *>(pointer);
    }

    template <typename T>
    T *match() {
        return tag == tag_of<T>() ? reinterpret_cast<T *>(pointer) : nullptr;
    }

    template <typename T>
    const T *match() const {
        return tag == tag_of<T>() ? reinterpret_cast<T *>(pointer) : nullptr;
    }

    operator bool() const { return tag != 0; }
    bool operator==(const DynamicPointer &other) const { return pointer == other.pointer; }
    bool operator!=(const DynamicPointer &other) const { return !(*this == other); }
    
    std::size_t compute_hash() const noexcept { return reinterpret_cast<std::size_t>(pointer); }

    template <typename T>
    static constexpr Tag tag_of() {
        return compute_tag<T, Types...>();
    }

private:
    template <typename Type, typename Current, typename... RemainingTypes>
    static constexpr Tag compute_tag() {
        if constexpr (std::is_same<Type, Current>()) {
            return 1;
        } else {
            return compute_tag<Type, RemainingTypes...>() + 1;
        }
    }

    template <typename Type>
    static constexpr Tag compute_tag() {
        return 0;
    }
};

} // namespace banjo

#endif
