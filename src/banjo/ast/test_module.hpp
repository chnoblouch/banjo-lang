#ifndef TEST_MODULE_H
#define TEST_MODULE_H

#include "ast/ast_module.hpp"
#include "ast/module_list.hpp"
#include "config/config.hpp"

#include <sstream>

namespace lang {

class TestModuleGenerator {

private:
    std::stringstream source;

public:
    ASTModule *generate(ModuleList &module_list, const Config &config);
};

} // namespace lang

#endif
