#ifndef STD_CONFIG_MODULE_H
#define STD_CONFIG_MODULE_H

#include "ast/ast_block.hpp"
#include "ast/ast_module.hpp"
#include "config/config.hpp"

namespace banjo {

namespace lang {

class StdConfigModule : public ASTModule {

private:
    const Config &config;
    ASTBlock *block;

public:
    StdConfigModule();
    unsigned get_arch();
    unsigned get_os();
    void add_const_u32(const std::string &name, unsigned value);
};

} // namespace lang

} // namespace banjo

#endif
