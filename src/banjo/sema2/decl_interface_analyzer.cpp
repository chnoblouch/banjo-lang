#include "decl_interface_analyzer.hpp"

#include "banjo/sema2/expr_analyzer.hpp"
#include "banjo/sir/sir.hpp"

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
            sir::Symbol symbol = analyzer.get_scope().decl;

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

Result DeclInterfaceAnalyzer::analyze_func_decl(sir::FuncDecl &func_decl) {
    func_decl.type.params[0].type = analyzer.create_expr(sir::PrimitiveType{
        .ast_node = nullptr,
        .primitive = sir::Primitive::ADDR,
    });

    for (unsigned i = 1; i < func_decl.type.params.size(); i++) {
        sir::Param &param = func_decl.type.params[i];
        ExprAnalyzer(analyzer).analyze(param.type);
    }

    ExprAnalyzer(analyzer).analyze(func_decl.type.return_type);

    if (auto proto_def = analyzer.get_scope().decl.match<sir::ProtoDef>()) {
        proto_def->func_decls.push_back(&func_decl);
    }

    return Result::SUCCESS;
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

Result DeclInterfaceAnalyzer::analyze_struct_def(sir::StructDef &struct_def) {
    Result partial_result;

    for (sir::Expr &impl : struct_def.impls) {
        partial_result = ExprAnalyzer(analyzer).analyze(impl);

        if (partial_result != Result::SUCCESS) {
            continue;
        }

        if (!impl.match_symbol<sir::ProtoDef>()) {
            analyzer.report_generator.report_err_expected_proto(impl);
        }
    }

    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl) {
    ExprAnalyzer(analyzer).analyze(var_decl.type);

    if (auto struct_def = analyzer.get_scope().decl.match<sir::StructDef>()) {
        out_decl = analyzer.create_decl(sir::StructField{
            .ast_node = var_decl.ast_node,
            .ident = var_decl.ident,
            .type = var_decl.type,
            .attrs = var_decl.attrs,
            .index = static_cast<unsigned>(struct_def->fields.size()),
        });

        struct_def->fields.push_back(&out_decl.as<sir::StructField>());

        analyzer.add_symbol_def(&out_decl.as<sir::StructField>());
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

    sir::EnumDef &enum_def = analyzer.get_scope().decl.as<sir::EnumDef>();
    enum_def.variants.push_back(&enum_variant);

    enum_variant.type = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = nullptr,
        .symbol = &enum_def,
    });

    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_union_case(sir::UnionCase &union_case) {
    for (sir::UnionCaseField &field : union_case.fields) {
        ExprAnalyzer(analyzer).analyze(field.type);
    }

    sir::UnionDef &union_def = analyzer.get_scope().decl.as<sir::UnionDef>();
    union_def.cases.push_back(&union_case);

    return Result::SUCCESS;
}

} // namespace sema

} // namespace lang

} // namespace banjo
