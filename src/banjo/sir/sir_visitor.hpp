#ifndef SIR_VISITOR_H
#define SIR_VISITOR_H

#include "banjo/sir/sir.hpp" // IWYU pragma: export
#include "banjo/utils/macros.hpp"

#define SIR_VISIT_IMPOSSIBLE ASSERT_UNREACHABLE
#define SIR_VISIT_IGNORE ;

#define SIR_VISIT_EXPR(                                                                                                \
    expr,                                                                                                              \
    empty_visitor,                                                                                                     \
    int_literal_visitor,                                                                                               \
    fp_literal_visitor,                                                                                                \
    bool_literal_visitor,                                                                                              \
    char_literal_visitor,                                                                                              \
    null_literal_visitor,                                                                                              \
    none_literal_visitor,                                                                                              \
    undefined_literal_visitor,                                                                                         \
    array_literal_visitor,                                                                                             \
    string_literal_visitor,                                                                                            \
    struct_literal_visitor,                                                                                            \
    union_case_literal_visitor,                                                                                        \
    map_literal_visitor,                                                                                               \
    closure_literal_visitor,                                                                                           \
    symbol_expr_visitor,                                                                                               \
    binary_expr_visitor,                                                                                               \
    unary_expr_visitor,                                                                                                \
    cast_expr_visitor,                                                                                                 \
    index_expr_visitor,                                                                                                \
    call_expr_visitor,                                                                                                 \
    field_expr_visitor,                                                                                                \
    range_expr_visitor,                                                                                                \
    tuple_expr_visitor,                                                                                                \
    coercion_expr_visitor,                                                                                             \
    primitive_type_visitor,                                                                                            \
    pointer_type_visitor,                                                                                              \
    static_array_type_visitor,                                                                                         \
    func_type_visitor,                                                                                                 \
    optional_type_visitor,                                                                                             \
    result_type_visitor,                                                                                               \
    array_type_visitor,                                                                                                \
    map_type_visitor,                                                                                                  \
    closure_type_visitor,                                                                                              \
    ident_expr_visitor,                                                                                                \
    star_expr_visitor,                                                                                                 \
    bracket_expr_visitor,                                                                                              \
    dot_expr_visitor,                                                                                                  \
    pseudo_type_visitor,                                                                                               \
    meta_access_visitor,                                                                                               \
    meta_field_expr_visitor,                                                                                           \
    meta_call_expr_visitor,                                                                                            \
    move_expr_visitor,                                                                                                 \
    deinit_expr_visitor,                                                                                               \
    completion_token_visitor                                                                                           \
)                                                                                                                      \
    if (!(expr)) {                                                                                                     \
        empty_visitor;                                                                                                 \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::IntLiteral>()) {                           \
        int_literal_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::FPLiteral>()) {                            \
        fp_literal_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::BoolLiteral>()) {                          \
        bool_literal_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::CharLiteral>()) {                          \
        char_literal_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::NullLiteral>()) {                          \
        null_literal_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::NoneLiteral>()) {                          \
        none_literal_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::UndefinedLiteral>()) {                     \
        undefined_literal_visitor;                                                                                     \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::ArrayLiteral>()) {                         \
        array_literal_visitor;                                                                                         \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::StringLiteral>()) {                        \
        string_literal_visitor;                                                                                        \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::StructLiteral>()) {                        \
        struct_literal_visitor;                                                                                        \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::UnionCaseLiteral>()) {                     \
        union_case_literal_visitor;                                                                                    \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::MapLiteral>()) {                           \
        map_literal_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::ClosureLiteral>()) {                       \
        closure_literal_visitor;                                                                                       \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::SymbolExpr>()) {                           \
        symbol_expr_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::BinaryExpr>()) {                           \
        binary_expr_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::UnaryExpr>()) {                            \
        unary_expr_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::CastExpr>()) {                             \
        cast_expr_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::IndexExpr>()) {                            \
        index_expr_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::CallExpr>()) {                             \
        call_expr_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::FieldExpr>()) {                            \
        field_expr_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::RangeExpr>()) {                            \
        range_expr_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::TupleExpr>()) {                            \
        tuple_expr_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::CoercionExpr>()) {                         \
        coercion_expr_visitor;                                                                                         \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::PrimitiveType>()) {                        \
        primitive_type_visitor;                                                                                        \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::PointerType>()) {                          \
        pointer_type_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::StaticArrayType>()) {                      \
        static_array_type_visitor;                                                                                     \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::FuncType>()) {                             \
        func_type_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::OptionalType>()) {                         \
        optional_type_visitor;                                                                                         \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::ResultType>()) {                           \
        result_type_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::ArrayType>()) {                            \
        array_type_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::MapType>()) {                              \
        map_type_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::ClosureType>()) {                          \
        closure_type_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::IdentExpr>()) {                            \
        ident_expr_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::StarExpr>()) {                             \
        star_expr_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::BracketExpr>()) {                          \
        bracket_expr_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::DotExpr>()) {                              \
        dot_expr_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::PseudoType>()) {                           \
        pseudo_type_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::MetaAccess>()) {                           \
        meta_access_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::MetaFieldExpr>()) {                        \
        meta_field_expr_visitor;                                                                                       \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::MetaCallExpr>()) {                         \
        meta_call_expr_visitor;                                                                                        \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::MoveExpr>()) {                             \
        move_expr_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::DeinitExpr>()) {                           \
        deinit_expr_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::CompletionToken>()) {                      \
        completion_token_visitor;                                                                                      \
    } else {                                                                                                           \
        ASSERT_UNREACHABLE;                                                                                            \
    }

#define SIR_VISIT_TYPE(                                                                                                \
    expr,                                                                                                              \
    empty_visitor,                                                                                                     \
    symbol_expr_visitor,                                                                                               \
    tuple_expr_visitor,                                                                                                \
    primitive_type_visitor,                                                                                            \
    pointer_type_visitor,                                                                                              \
    static_array_type_visitor,                                                                                         \
    func_type_visitor,                                                                                                 \
    closure_type_visitor,                                                                                              \
    other_visitor                                                                                                      \
)                                                                                                                      \
    if (!(expr)) {                                                                                                     \
        empty_visitor;                                                                                                 \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::SymbolExpr>()) {                           \
        symbol_expr_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::TupleExpr>()) {                            \
        tuple_expr_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::PrimitiveType>()) {                        \
        primitive_type_visitor;                                                                                        \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::PointerType>()) {                          \
        pointer_type_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::StaticArrayType>()) {                      \
        static_array_type_visitor;                                                                                     \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::FuncType>()) {                             \
        func_type_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (expr).match<banjo::lang::sir::ClosureType>()) {                          \
        closure_type_visitor;                                                                                          \
    } else {                                                                                                           \
        other_visitor;                                                                                                 \
    }

#define SIR_VISIT_STMT(                                                                                                \
    stmt,                                                                                                              \
    empty_visitor,                                                                                                     \
    var_stmt_visitor,                                                                                                  \
    assign_stmt_visitor,                                                                                               \
    comp_assign_stmt_visitor,                                                                                          \
    return_stmt_visitor,                                                                                               \
    if_stmt_visitor,                                                                                                   \
    switch_stmt_visitor,                                                                                               \
    try_stmt_visitor,                                                                                                  \
    while_stmt_visitor,                                                                                                \
    for_stmt_visitor,                                                                                                  \
    loop_stmt_visitor,                                                                                                 \
    continue_stmt_visitor,                                                                                             \
    break_stmt_visitor,                                                                                                \
    meta_if_stmt_visitor,                                                                                              \
    meta_for_stmt_visitor,                                                                                             \
    expanded_meta_stmt_visitor,                                                                                        \
    expr_stmt_visitor,                                                                                                 \
    block_stmt_visitor,                                                                                                \
    error_visitor                                                                                                      \
)                                                                                                                      \
    if (!(stmt)) {                                                                                                     \
        empty_visitor;                                                                                                 \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::VarStmt>()) {                              \
        var_stmt_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::AssignStmt>()) {                           \
        assign_stmt_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::CompAssignStmt>()) {                       \
        comp_assign_stmt_visitor;                                                                                      \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::ReturnStmt>()) {                           \
        return_stmt_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::IfStmt>()) {                               \
        if_stmt_visitor;                                                                                               \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::SwitchStmt>()) {                           \
        switch_stmt_visitor;                                                                                           \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::TryStmt>()) {                              \
        try_stmt_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::WhileStmt>()) {                            \
        while_stmt_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::ForStmt>()) {                              \
        for_stmt_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::LoopStmt>()) {                             \
        loop_stmt_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::ContinueStmt>()) {                         \
        continue_stmt_visitor;                                                                                         \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::BreakStmt>()) {                            \
        break_stmt_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::MetaIfStmt>()) {                           \
        meta_if_stmt_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::MetaForStmt>()) {                          \
        meta_for_stmt_visitor;                                                                                         \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::ExpandedMetaStmt>()) {                     \
        expanded_meta_stmt_visitor;                                                                                    \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::Expr>()) {                                 \
        expr_stmt_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::Block>()) {                                \
        block_stmt_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (stmt).match<banjo::lang::sir::Error>()) {                                \
        error_visitor;                                                                                                 \
    } else {                                                                                                           \
        ASSERT_UNREACHABLE;                                                                                            \
    }

#define SIR_VISIT_DECL(                                                                                                \
    decl,                                                                                                              \
    empty_visitor,                                                                                                     \
    func_def_visitor,                                                                                                  \
    func_decl_visitor,                                                                                                 \
    native_func_decl_visitor,                                                                                          \
    const_def_visitor,                                                                                                 \
    struct_def_visitor,                                                                                                \
    struct_field_visitor,                                                                                              \
    var_decl_visitor,                                                                                                  \
    native_var_decl_visitor,                                                                                           \
    enum_def_visitor,                                                                                                  \
    enum_variant_visitor,                                                                                              \
    union_def_visitor,                                                                                                 \
    union_case_visitor,                                                                                                \
    proto_def_visitor,                                                                                                 \
    type_alias_visitor,                                                                                                \
    use_decl_visitor,                                                                                                  \
    meta_if_stmt_visitor,                                                                                              \
    expanded_meta_stmt_visitor,                                                                                        \
    error_visitor,                                                                                                     \
    completion_token_visitor                                                                                           \
)                                                                                                                      \
    if (!(decl)) {                                                                                                     \
        empty_visitor;                                                                                                 \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::FuncDef>()) {                              \
        func_def_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::FuncDecl>()) {                             \
        func_decl_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::NativeFuncDecl>()) {                       \
        native_func_decl_visitor;                                                                                      \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::ConstDef>()) {                             \
        const_def_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::StructDef>()) {                            \
        struct_def_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::StructField>()) {                          \
        struct_field_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::VarDecl>()) {                              \
        var_decl_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::NativeVarDecl>()) {                        \
        native_var_decl_visitor;                                                                                       \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::EnumDef>()) {                              \
        enum_def_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::EnumVariant>()) {                          \
        enum_variant_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::UnionDef>()) {                             \
        union_def_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::UnionCase>()) {                            \
        union_case_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::ProtoDef>()) {                             \
        proto_def_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::TypeAlias>()) {                            \
        type_alias_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::UseDecl>()) {                              \
        use_decl_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::MetaIfStmt>()) {                           \
        meta_if_stmt_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::ExpandedMetaStmt>()) {                     \
        expanded_meta_stmt_visitor;                                                                                    \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::Error>()) {                                \
        error_visitor;                                                                                                 \
    } else if ([[maybe_unused]] auto inner = (decl).match<banjo::lang::sir::CompletionToken>()) {                      \
        completion_token_visitor;                                                                                      \
    } else {                                                                                                           \
        ASSERT_UNREACHABLE;                                                                                            \
    }

#define SIR_VISIT_SYMBOL(                                                                                              \
    symbol,                                                                                                            \
    empty_visitor,                                                                                                     \
    module_visitor,                                                                                                    \
    func_def_visitor,                                                                                                  \
    func_decl_visitor,                                                                                                 \
    native_func_decl_visitor,                                                                                          \
    const_def_visitor,                                                                                                 \
    struct_def_visitor,                                                                                                \
    struct_field_visitor,                                                                                              \
    var_decl_visitor,                                                                                                  \
    native_var_decl_visitor,                                                                                           \
    enum_def_visitor,                                                                                                  \
    enum_variant_visitor,                                                                                              \
    union_def_visitor,                                                                                                 \
    union_case_visitor,                                                                                                \
    proto_def_visitor,                                                                                                 \
    type_alias_visitor,                                                                                                \
    use_ident_visitor,                                                                                                 \
    use_rebind_visitor,                                                                                                \
    local_visitor,                                                                                                     \
    param_visitor,                                                                                                     \
    overload_set_visitor                                                                                               \
)                                                                                                                      \
    if (!(symbol)) {                                                                                                   \
        empty_visitor;                                                                                                 \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::Module>()) {                             \
        module_visitor;                                                                                                \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::FuncDef>()) {                            \
        func_def_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::FuncDecl>()) {                           \
        func_decl_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::NativeFuncDecl>()) {                     \
        native_func_decl_visitor;                                                                                      \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::ConstDef>()) {                           \
        const_def_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::StructDef>()) {                          \
        struct_def_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::StructField>()) {                        \
        struct_field_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::VarDecl>()) {                            \
        var_decl_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::NativeVarDecl>()) {                      \
        native_var_decl_visitor;                                                                                       \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::EnumDef>()) {                            \
        enum_def_visitor;                                                                                              \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::EnumVariant>()) {                        \
        enum_variant_visitor;                                                                                          \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::UnionDef>()) {                           \
        union_def_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::UnionCase>()) {                          \
        union_case_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::ProtoDef>()) {                           \
        proto_def_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::TypeAlias>()) {                          \
        type_alias_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::UseIdent>()) {                           \
        use_ident_visitor;                                                                                             \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::UseRebind>()) {                          \
        use_rebind_visitor;                                                                                            \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::Local>()) {                              \
        local_visitor;                                                                                                 \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::Param>()) {                              \
        param_visitor;                                                                                                 \
    } else if ([[maybe_unused]] auto inner = (symbol).match<banjo::lang::sir::OverloadSet>()) {                        \
        overload_set_visitor;                                                                                          \
    } else {                                                                                                           \
        ASSERT_UNREACHABLE;                                                                                            \
    }

#endif
