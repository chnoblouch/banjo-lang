#include "module_path.hpp"

#include <utility>

namespace banjo {

namespace lang {

ModulePath::ModulePath() {}

ModulePath::ModulePath(std::initializer_list<std::string> elements) : path(elements) {}

void ModulePath::append(std::string element) {
    path.push_back(std::move(element));
}

unsigned ModulePath::get_size() const {
    return path.size();
}

bool ModulePath::is_empty() const {
    return path.empty();
}

std::string ModulePath::to_string(const std::string &delimiter /* = "." */) const {
    std::string string;
    
    for (unsigned i = 0; i < path.size(); i++) {
        if (i != 0) {
            string += delimiter;
        }

        string += path[i];
    }

    return string;
}

} // namespace lang

} // namespace banjo
