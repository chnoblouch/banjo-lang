#include "decl_interface_analyzer.hpp"

#include "banjo/sema2/expr_analyzer.hpp"

namespace banjo {

namespace lang {

namespace sema {

DeclInterfaceAnalyzer::DeclInterfaceAnalyzer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void DeclInterfaceAnalyzer::analyze() {
    for (sir::Module *mod : analyzer.sir_unit.mods) {
        analyzer.enter_mod(mod);
        analyze_decl_block(mod->block);
        analyzer.exit_mod();
    }
}

void DeclInterfaceAnalyzer::analyze_decl_block(sir::DeclBlock &decl_block) {
    for (sir::Decl &decl : decl_block.decls) {
        if (auto func_def = decl.match<sir::FuncDef>()) analyze_func_def(*func_def);
        else if (auto native_func_decl = decl.match<sir::NativeFuncDecl>()) analyze_native_func_decl(*native_func_decl);
        else if (auto const_def = decl.match<sir::ConstDef>()) analyze_const_def(*const_def);
        else if (auto struct_def = decl.match<sir::StructDef>()) analyze_struct_def(*struct_def);
        else if (auto var_decl = decl.match<sir::VarDecl>()) analyze_var_decl(*var_decl, decl);
        else if (auto enum_def = decl.match<sir::EnumDef>()) analyze_enum_def(*enum_def);
        else if (auto enum_variant = decl.match<sir::EnumVariant>()) analyze_enum_variant(*enum_variant);
    }
}

void DeclInterfaceAnalyzer::analyze_func_def(sir::FuncDef &func_def) {
    if (func_def.is_generic()) {
        return;
    }

    for (sir::Param &param : func_def.type.params) {
        func_def.block.symbol_table->symbols.insert({param.name.value, &param});

        if (param.name.value == "self") {
            param.type = analyzer.create_expr(sir::PointerType{
                .ast_node = nullptr,
                .base_type = analyzer.create_expr(sir::SymbolExpr{
                    .ast_node = nullptr,
                    .type = nullptr,
                    .symbol = analyzer.get_scope().struct_def,
                }),
            });
        } else {
            ExprAnalyzer(analyzer).analyze(param.type);
        }
    }

    ExprAnalyzer(analyzer).analyze(func_def.type.return_type);
}

void DeclInterfaceAnalyzer::analyze_native_func_decl(sir::NativeFuncDecl &native_func_decl) {
    for (sir::Param &param : native_func_decl.type.params) {
        ExprAnalyzer(analyzer).analyze(param.type);
    }

    ExprAnalyzer(analyzer).analyze(native_func_decl.type.return_type);
}

void DeclInterfaceAnalyzer::analyze_const_def(sir::ConstDef &const_def) {
    ExprAnalyzer(analyzer).analyze(const_def.type);
}

void DeclInterfaceAnalyzer::analyze_struct_def(sir::StructDef &struct_def) {
    if (struct_def.is_generic()) {
        return;
    }

    analyzer.enter_struct_def(&struct_def);
    analyze_decl_block(struct_def.block);
    analyzer.exit_struct_def();
}

void DeclInterfaceAnalyzer::analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl) {
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
}

void DeclInterfaceAnalyzer::analyze_enum_def(sir::EnumDef &enum_def) {
    analyzer.enter_enum_def(&enum_def);
    analyze_decl_block(enum_def.block);
    analyzer.exit_enum_def();
}

void DeclInterfaceAnalyzer::analyze_enum_variant(sir::EnumVariant &enum_variant) {
    if (enum_variant.value) {
        ExprAnalyzer(analyzer).analyze(enum_variant.value);
    }

    analyzer.get_scope().enum_def->variants.push_back(&enum_variant);

    enum_variant.type = analyzer.create_expr(sir::SymbolExpr{
        .ast_node = nullptr,
        .type = nullptr,
        .symbol = analyzer.get_scope().enum_def,
    });
}

} // namespace sema

} // namespace lang

} // namespace banjo
