#ifndef METHOD_TABLE_H
#define METHOD_TABLE_H

#include <string>
#include <vector>

namespace lang {

class Function;

class MethodTable {

private:
    std::vector<Function *> functions;

public:
    std::vector<Function *> &get_functions() { return functions; }
    void add_function(Function *function) { functions.push_back(function); }

    Function *get_function(const std::string &name);
    std::vector<Function *> get_functions(const std::string &name);
};

} // namespace lang

#endif
