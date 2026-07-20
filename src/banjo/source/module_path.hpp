#ifndef BANJO_SOURCE_MODULE_PATH_H
#define BANJO_SOURCE_MODULE_PATH_H

#include <initializer_list>
#include <string>
#include <string_view>
#include <optional>

namespace banjo::lang {

class ModulePath {

public:
    class Iterator {

    private:
        std::string_view value;
        unsigned index;

    public:
        Iterator(std::string_view value, unsigned index);
        std::string_view operator*() const;
        Iterator &operator++();
        bool operator==(const Iterator &other) const;
        bool operator!=(const Iterator &other) const;
    };

private:
    std::string value;

public:
    ModulePath();
    ModulePath(std::initializer_list<std::string_view> elements);
    ModulePath(std::initializer_list<std::string> elements);
    ModulePath(std::initializer_list<const char *> elements);

    void append(std::string_view element);
    unsigned get_size() const;
    bool is_empty() const;
    std::string_view to_string() const;
    std::string to_string(std::string_view delimiter) const;
    std::string_view name() const;
    std::optional<ModulePath> parent() const;
    unsigned num_common_ancestors(const ModulePath &other) const;
    ModulePath strip(unsigned count) const;

    Iterator begin() const { return Iterator(value, 0); }
    Iterator end() const { return Iterator(value, value.size()); }

    std::string_view operator[](unsigned index) const;
    friend bool operator==(const ModulePath &lhs, const ModulePath &right) { return lhs.value == right.value; }
    friend bool operator!=(const ModulePath &rhs, const ModulePath &right) { return !(rhs == right); }
};

} // namespace banjo::lang

template <>
struct std::hash<banjo::lang::ModulePath> {
    std::size_t operator()(const banjo::lang::ModulePath &path) const noexcept {
        return std::hash<std::string_view>{}(path.to_string());
    }
};

#endif
