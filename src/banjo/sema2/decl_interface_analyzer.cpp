#include "decl_interface_analyzer.hpp"

#include "banjo/sema2/expr_analyzer.hpp"

namespace banjo {

namespace lang {

namespace sema {

DeclInterfaceAnalyzer::DeclInterfaceAnalyzer(SemanticAnalyzer &analyzer) : DeclVisitor(analyzer) {}

Result DeclInterfaceAnalyzer::analyze_func_def(sir::FuncDef &func_def) {
    for (unsigned i = 0; i < func_def.type.params.size(); i++) {
        sir::Param &param = func_def.type.params[i];

        func_def.block.symbol_table->symbols.insert({param.name.value, &param});

        if (analyzer.get_scope().closure_ctx && i == 0) {
            continue;
        }

        if (param.name.value == "self") {
            sir::Symbol symbol;

            if (analyzer.get_scope().struct_def) {
                symbol = analyzer.get_scope().struct_def;
            } else if (analyzer.get_scope().union_def) {
                symbol = analyzer.get_scope().union_def;
            }

            param.type = analyzer.create_expr(sir::PointerType{
                .ast_node = nullptr,
                .base_type = analyzer.create_expr(sir::SymbolExpr{
                    .ast_node = nullptr,
                    .type = nullptr,
                    .symbol = symbol,
                }),
            });
        } else {
            ExprAnalyzer(analyzer).analyze(param.type);
        }
    }

    Result result = ExprAnalyzer(analyzer).analyze(func_def.type.return_type);
    return result;
}

Result DeclInterfaceAnalyzer::analyze_native_func_decl(sir::NativeFuncDecl &native_func_decl) {
    for (sir::Param &param : native_func_decl.type.params) {
        ExprAnalyzer(analyzer).analyze(param.type);
    }

    ExprAnalyzer(analyzer).analyze(native_func_decl.type.return_type);
    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_const_def(sir::ConstDef &const_def) {
    return ExprAnalyzer(analyzer).analyze(const_def.type);
}

Result DeclInterfaceAnalyzer::analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl) {
    ExprAnalyzer(analyzer).analyze(var_decl.type);

    if (analyzer.get_scope().struct_def) {
        sir::StructDef &struct_def = *analyzer.get_scope().struct_def;

        out_decl = analyzer.create_decl(sir::StructField{
            .ast_node = var_decl.ast_node,
            .ident = var_decl.ident,
            .type = var_decl.type,
            .index = static_cast<unsigned>(struct_def.fields.size()),
        });

        struct_def.fields.push_back(&out_decl.as<sir::StructField>());
    }

    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_native_var_decl(sir::NativeVarDecl &native_var_decl) {
    return ExprAnalyzer(analyzer).analyze(native_var_decl.type);
}

Result DeclInterfaceAnalyzer::analyze_enum_variant(sir::EnumVariant &enum_variant) {
    if (enum_variant.value) {
        ExprAnalyzer(analyzer).analyze(enum_variant.value);
    }

    analyzer.get_scope().enum_def->variants.push_back(&enum_variant);

    enum_variant.type = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = nullptr,
        .symbol = analyzer.get_scope().enum_def,
    });

    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_union_case(sir::UnionCase &union_case) {
    for (sir::UnionCaseField &field : union_case.fields) {
        ExprAnalyzer(analyzer).analyze(field.type);
    }

    analyzer.get_scope().union_def->cases.push_back(&union_case);

    return Result::SUCCESS;
}

} // namespace sema

} // namespace lang

} // namespace banjo
