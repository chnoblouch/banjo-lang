#include "module_path.hpp"

#include <utility>

namespace banjo {

namespace lang {

ModulePath::ModulePath() {}

ModulePath::ModulePath(std::vector<std::string> path) : path(std::move(path)) {}

ModulePath::ModulePath(std::initializer_list<std::string> elements) : path(elements) {}

void ModulePath::append(std::string element) {
    path.push_back(std::move(element));
}

void ModulePath::append(const ModulePath &other) {
    for (const std::string &element : other.get_path()) {
        path.push_back(element);
    }
}

void ModulePath::prepend(const ModulePath &other) {
    path.insert(path.begin(), other.path.begin(), other.path.end());
}

ModulePath ModulePath::sub(unsigned start) const {
    return ModulePath(std::vector<std::string>(path.begin() + start, path.end()));
}

ModulePath ModulePath::parent() const {
    std::vector<std::string> parent = path;
    parent.pop_back();
    return parent;
}

const std::string &ModulePath::get(unsigned index) const {
    return path[index];
}

unsigned ModulePath::get_size() const {
    return path.size();
}

bool ModulePath::is_empty() const {
    return path.empty();
}

bool ModulePath::equals(const ModulePath &other) const {
    if (path.size() != other.path.size()) {
        return false;
    }

    for (int i = 0; i < path.size(); i++) {
        if (path[i] != other.path[i]) {
            return false;
        }
    }

    return true;
}

std::string ModulePath::to_string(const std::string &delimiter /* = '.' */) const {
    std::string string;
    for (int i = 0; i < path.size(); i++) {
        if (i != 0) {
            string += delimiter;
        }

        string += path[i];
    }
    return string;
}

} // namespace lang

} // namespace banjo
