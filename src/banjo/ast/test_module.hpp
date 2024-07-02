#ifndef TEST_MODULE_H
#define TEST_MODULE_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/module_list.hpp"
#include "banjo/config/config.hpp"

#include <sstream>

namespace banjo {

namespace lang {

class TestModuleGenerator {

private:
    std::stringstream source;

public:
    ASTModule *generate(ModuleList &module_list, const Config &config);
};

} // namespace lang

} // namespace banjo

#endif
