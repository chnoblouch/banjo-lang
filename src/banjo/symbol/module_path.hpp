#ifndef MODULE_PATH_H
#define MODULE_PATH_H

#include <initializer_list>
#include <string>
#include <vector>

namespace banjo {

namespace lang {

class ModulePath {

private:
    std::vector<std::string> path;

public:
    ModulePath();
    ModulePath(std::vector<std::string> path);
    ModulePath(std::initializer_list<std::string> elements);

    const std::vector<std::string> &get_path() const { return path; }
    void append(std::string element);
    void append(const ModulePath &other);
    void prepend(const ModulePath &other);
    ModulePath sub(unsigned start) const;
    ModulePath parent() const;
    const std::string &get(unsigned index) const;
    unsigned get_size() const;
    bool is_empty() const;
    bool equals(const ModulePath &other) const;
    std::string to_string(const std::string &delimiter = ".") const;

    const std::string &operator[](unsigned index) const { return path[index]; }
    friend bool operator==(const ModulePath &left, const ModulePath &right) { return left.path == right.path; }
    friend bool operator!=(const ModulePath &left, const ModulePath &right) { return !(left == right); }
};

} // namespace lang

} // namespace banjo

template <>
struct std::hash<banjo::lang::ModulePath> {
    std::size_t operator()(const banjo::lang::ModulePath &path) const noexcept {
        std::size_t hash = 0;
        for (const std::string &element : path.get_path()) {
            hash ^= std::hash<std::string>()(element);
        }
        return hash;
    }
};

#endif
