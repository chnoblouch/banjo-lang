#ifndef BANJO_AST_STD_CONFIG_MODULE_H
#define BANJO_AST_STD_CONFIG_MODULE_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/config/config.hpp"

namespace banjo {

namespace lang {

class StdConfigModule : public ASTModule {

private:
    const Config &config;
    ASTNode *block;

public:
    StdConfigModule();
    unsigned get_arch();
    unsigned get_os();
    void add_const_u32(const std::string &name, unsigned value);
};

} // namespace lang

} // namespace banjo

#endif
