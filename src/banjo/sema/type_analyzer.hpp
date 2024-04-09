#ifndef TYPE_ANALYZER_H
#define TYPE_ANALYZER_H

#include "ast/ast_node.hpp"
#include "sema/semantic_analyzer_context.hpp"
#include "symbol/symbol_ref.hpp"

#include <cassert>

namespace lang {

class TypeAnalyzer {

public:
    struct Result {
        SemaResult result;
        DataType *type;

        Result(SemaResult result) : result(result), type(nullptr) {}
        Result(DataType *type) : result(SemaResult::OK), type(type) { assert(type); }
    };

private:
    SemanticAnalyzerContext &context;

public:
    TypeAnalyzer(SemanticAnalyzerContext &context);
    Result analyze(ASTNode *node);
    Result analyze_param(ASTNode *node);

private:
    Result analyze_primitive_type(ASTNode *node, PrimitiveType primitive);
    Result analyze_array_type(ASTNode *node);
    Result analyze_map_type(ASTNode *node);
    Result analyze_optional_type(ASTNode *node);
    Result analyze_result_type(ASTNode *node);
    Result analyze_ident_type(ASTNode *node);
    Result analyze_dot_operator_type(ASTNode *node);
    Result analyze_ptr_type(ASTNode *node);
    Result analyze_func_type(ASTNode *node);
    Result analyze_static_array_type(ASTNode *node);
    Result analyze_tuple_type(ASTNode *node);
    Result analyze_closure_type(ASTNode *node);
    Result analyze_generic_instance_type(ASTNode *node);
    Result analyze_explicit_type(ASTNode *node);

    DataType *from_symbol(SymbolRef symbol, ASTNode *node);
    void resolve_dot_operator_element(ASTNode *node, SymbolTable *&symbol_table);

    DataType *instantiate_generic_std_type(
        ASTNode *node,
        const ModulePath &module_path,
        std::string name,
        GenericInstanceArgs args
    );
};

} // namespace lang

#endif
