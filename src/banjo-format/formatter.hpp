#ifndef BANJO_FORMAT_FORMATTER_H
#define BANJO_FORMAT_FORMATTER_H

#include "banjo/ast/ast_node.hpp"

#include <filesystem>
#include <fstream>
#include <string_view>

namespace banjo {
namespace format {

class Formatter {

private:
    unsigned indent = 0;
    bool insert_empty_line = false;
    std::ofstream out_stream;

public:
    void format(const std::filesystem::path &file_path);

private:
    void print_node(lang::ASTNode *node);

    void print_mod(lang::ASTNode *node);
    void print_func_def(lang::ASTNode *node);
    void print_generic_func_def(lang::ASTNode *node);
    void print_func_decl(lang::ASTNode *node);
    void print_native_func_decl(lang::ASTNode *node);
    void print_const(lang::ASTNode *node);
    void print_struct_def(lang::ASTNode *node);
    void print_generic_struct_def(lang::ASTNode *node);
    void print_var(lang::ASTNode *node);
    void print_typeless_var(lang::ASTNode *node);
    void print_native_var(lang::ASTNode *node);
    void print_enum_def(lang::ASTNode *node);
    void print_enum_variant(lang::ASTNode *node);
    void print_union_def(lang::ASTNode *node);
    void print_union_case(lang::ASTNode *node);
    void print_proto_def(lang::ASTNode *node);
    void print_type_alias(lang::ASTNode *node);
    void print_use_decl(lang::ASTNode *node);
    void print_use_rebind(lang::ASTNode *node);
    void print_use_list(lang::ASTNode *node);

    void print_block(lang::ASTNode *node);
    void print_block_of_decl(lang::ASTNode *node);
    void print_assign_stmt(lang::ASTNode *node);
    void print_comp_assign_stmt(lang::ASTNode *node, std::string_view op_text);
    void print_return_stmt(lang::ASTNode *node);
    void print_if_stmt(lang::ASTNode *node);
    void print_if_condition(lang::ASTNode *node, bool first);
    void print_else(lang::ASTNode *node);
    void print_switch_stmt(lang::ASTNode *node);
    void print_switch_case(lang::ASTNode *node);
    void print_try_stmt(lang::ASTNode *node);
    void print_try_success_case(lang::ASTNode *node);
    void print_try_error_case(lang::ASTNode *node);
    void print_try_else_case(lang::ASTNode *node);
    void print_while_stmt(lang::ASTNode *node);
    void print_for_stmt(lang::ASTNode *node);

    void print_char_literal(lang::ASTNode *node);
    void print_array_expr(lang::ASTNode *node);
    void print_string_literal(lang::ASTNode *node);
    void print_struct_literal(lang::ASTNode *node);
    void print_anon_struct_literal(lang::ASTNode *node);
    void print_struct_literal_entries(lang::ASTNode *node);
    void print_struct_literal_entry(lang::ASTNode *node);
    void print_map_expr(lang::ASTNode *node);
    void print_map_literal_entry(lang::ASTNode *node);
    void print_closure_literal(lang::ASTNode *node);
    void print_paren_expr(lang::ASTNode *node);
    void print_tuple_expr(lang::ASTNode *node);
    void print_binary_expr(lang::ASTNode *node, std::string_view op_text);
    void print_unary_expr(lang::ASTNode *node, std::string_view op_text);
    void print_cast_expr(lang::ASTNode *node);
    void print_call_expr(lang::ASTNode *node);
    void print_dot_expr(lang::ASTNode *node);
    void print_range_expr(lang::ASTNode *node);
    void print_bracket_expr(lang::ASTNode *node);
    void print_static_array_type(lang::ASTNode *node);
    void print_func_type(lang::ASTNode *node);
    void print_optional_type(lang::ASTNode *node);
    void print_result_type(lang::ASTNode *node);
    void print_closure_type(lang::ASTNode *node);
    void print_explicit_type(lang::ASTNode *node);
    void print_meta_access(lang::ASTNode *node);
    
    void print_meta_if_stmt(lang::ASTNode *node);
    void print_meta_for_stmt(lang::ASTNode *node);

    void print_func_type(lang::ASTNode *params_node, lang::ASTNode *return_type_node);
    void print_closure_type(lang::ASTNode *params_node, lang::ASTNode *return_type_node);
    void print_param(lang::ASTNode *node);
    void print_qualifier_list(lang::ASTNode *node);
    void print_generic_param_list(lang::ASTNode *node);
    void print_generic_param(lang::ASTNode *node);

    void print_list(lang::ASTNode *node);
    void print_block_children(lang::ASTNode *node);
    void indent_line();
    void try_insert_empty_line(lang::ASTNode *child);

    void emit(std::string_view text);
    void emit(char c);
};

} // namespace format
} // namespace banjo

#endif
