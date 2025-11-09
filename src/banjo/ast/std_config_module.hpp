#ifndef BANJO_AST_STD_CONFIG_MODULE_H
#define BANJO_AST_STD_CONFIG_MODULE_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/config/config.hpp"
#include "banjo/source/source_file.hpp"

#include <initializer_list>
#include <string_view>

namespace banjo {

namespace lang {

class StdConfigModule : public ASTModule {

private:
    const Config &config;
    ASTNode *block;
    utils::StringArena<128> string_arena;

public:
    StdConfigModule(SourceFile &file);
    unsigned get_arch();
    unsigned get_os();
    void add_const_u32(std::string_view name, unsigned value);

private:
    void add_const_enum(
        std::string_view const_name,
        std::string_view enum_name,
        std::initializer_list<std::string_view> values
    );

    ASTNode *create_enum_variant(std::string_view name);
};

} // namespace lang

} // namespace banjo

#endif
