#ifndef MODULE_PATH_H
#define MODULE_PATH_H

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace banjo {

namespace lang {

class ModulePath {

private:
    std::vector<std::string> path;

public:
    ModulePath();
    ModulePath(std::initializer_list<std::string> elements);

    void append(std::string element);
    unsigned get_size() const;
    bool is_empty() const;
    std::string to_string(const std::string &delimiter = ".") const;

    std::vector<std::string>::const_iterator begin() const { return path.begin(); }
    std::vector<std::string>::const_iterator end() const { return path.end(); }

    std::string_view operator[](unsigned index) const { return path[index]; }
    friend bool operator==(const ModulePath &lhs, const ModulePath &right) { return lhs.path == right.path; }
    friend bool operator!=(const ModulePath &rhs, const ModulePath &right) { return !(rhs == right); }
};

} // namespace lang

} // namespace banjo

template <>
struct std::hash<banjo::lang::ModulePath> {
    std::size_t operator()(const banjo::lang::ModulePath &path) const noexcept {
        std::size_t hash = 0;
        for (const std::string &element : path) {
            hash ^= std::hash<std::string>()(element);
        }
        return hash;
    }
};

#endif
