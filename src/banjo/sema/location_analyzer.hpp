#ifndef LOCATION_ANALYZER_H
#define LOCATION_ANALYZER_H

#include "ast/ast_node.hpp"
#include "ast/expr.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "symbol/data_type.hpp"
#include "symbol/location.hpp"
#include "symbol/method_table.hpp"
#include "symbol/symbol_ref.hpp"
#include "symbol/symbol_table.hpp"

#include <optional>
#include <vector>

namespace banjo {

namespace lang {

struct SymbolUsage {
    ASTNode *arg_list;

    SymbolUsage() : arg_list(nullptr) {}
    SymbolUsage(ASTNode *arg_list) : arg_list(arg_list) {}
};

class LocationAnalyzer {

private:
    ASTNode *node;
    SemanticAnalyzerContext &context;
    SymbolUsage usage;
    DataType *expected_type = nullptr;

    SymbolTable *cur_symbol_table;
    Location location;

    bool is_moving;
    DeinitInfo *deinit_info = nullptr;

public:
    LocationAnalyzer(ASTNode *node, SemanticAnalyzerContext &context, SymbolUsage usage = {});
    void set_expected_type(DataType *expected_type) { this->expected_type = expected_type; }
    SemaResult check();
    const Location &get_location() const { return location; }

private:
    bool check(ASTNode *node);

    bool check_identifier(ASTNode *node);
    bool check_struct_member_access(ASTNode *node);
    bool check_union_member_access(ASTNode *node);
    bool check_union_case_member_access(ASTNode *node);
    bool check_ptr_member_access(ASTNode *node);
    bool check_dot_operator(ASTNode *node);
    bool check_self(ASTNode *node);
    bool check_tuple_index(ASTNode *node);
    bool check_generic_instantiation(ASTNode *node);
    bool check_bracket_expr(BracketExpr *node);
    bool check_expr(Expr *node);

    bool check_top_level(ASTNode *node);
    bool check_top_level(ASTNode *node, SymbolRef symbol);

    void set_root_func(Function *func);
    void report_root_not_found(ASTNode *node);
    void report_root_not_found_with_candidates(ASTNode *node, SymbolRef symbol);
    void analyze_args_if_required(SymbolRef symbol, DataType *type);

    bool check_meta_field_access(ASTNode *node);
    bool check_meta_method_call(ASTNode *node);

    bool is_moving_value();
    void add_move(DeinitInfo *deinit_info);

    SymbolRef resolve_method(MethodTable &method_table, const std::string &name);
};

} // namespace lang

} // namespace banjo

#endif
