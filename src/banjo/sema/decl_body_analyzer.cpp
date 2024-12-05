#include "decl_body_analyzer.hpp"

#include "banjo/sema/const_evaluator.hpp"
#include "banjo/sema/expr_analyzer.hpp"
#include "banjo/sema/stmt_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

DeclBodyAnalyzer::DeclBodyAnalyzer(SemanticAnalyzer &analyzer) : DeclVisitor(analyzer) {}

Result DeclBodyAnalyzer::analyze_func_def(sir::FuncDef &func_def) {
    if (analyzer.get_scope().decl.is<sir::ProtoDef>()) {
        return Result::SUCCESS;
    }

    analyzer.push_scope().decl = &func_def;
    StmtAnalyzer(analyzer).analyze_block(func_def.block);
    analyzer.pop_scope();

    return Result::SUCCESS;
}

Result DeclBodyAnalyzer::analyze_const_def(sir::ConstDef &const_def) {
    if (!const_def.type) {
        return Result::ERROR;
    }

    return ExprAnalyzer(analyzer).analyze(const_def.value, ExprConstraints::expect_type(const_def.type));
}

Result DeclBodyAnalyzer::analyze_struct_def(sir::StructDef &struct_def) {
    for (sir::Expr &impl : struct_def.impls) {
        if (auto proto_def = impl.match_symbol<sir::ProtoDef>()) {
            analyze_proto_impl(struct_def, *proto_def);
        }
    }

    return Result::SUCCESS;
}

void DeclBodyAnalyzer::analyze_proto_impl(sir::StructDef &struct_def, sir::ProtoDef &proto_def) {
    sir::SymbolTable &symbol_table = *struct_def.block.symbol_table;

    for (sir::ProtoFuncDecl func_decl : proto_def.func_decls) {
        std::string_view name = func_decl.get_ident().value;
        auto iter = symbol_table.symbols.find(name);

        if (iter == symbol_table.symbols.end() || !iter->second.is<sir::FuncDef>()) {
            continue;
        }

        sir::FuncDef &func_def = iter->second.as<sir::FuncDef>();

        sir::FuncType &def_type = func_def.type;
        sir::FuncType &decl_type = func_decl.get_type();
        bool types_equal = true;

        if (def_type.params.size() != decl_type.params.size()) {
            types_equal = false;
        }

        for (unsigned i = 1; i < def_type.params.size(); i++) {
            if (def_type.params[i] != decl_type.params[i]) {
                types_equal = false;
            }
        }

        if (def_type.return_type != decl_type.return_type) {
            types_equal = false;
        }

        if (!types_equal) {
            analyzer.report_generator.report_err_impl_type_mismatch(func_def, func_decl);
        }
    }
}

Result DeclBodyAnalyzer::analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl) {
    Result partial_result;

    if (!var_decl.type) {
        return Result::ERROR;
    }

    if (var_decl.value) {
        partial_result = ExprAnalyzer(analyzer).analyze(var_decl.value, ExprConstraints::expect_type(var_decl.type));

        if (partial_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        var_decl.value = ConstEvaluator(analyzer, false).evaluate(var_decl.value);
    }

    return Result::SUCCESS;
}

Result DeclBodyAnalyzer::analyze_enum_def(sir::EnumDef &enum_def) {
    LargeInt next_value = 0;

    for (sir::EnumVariant *variant : enum_def.variants) {
        if (variant->value) {
            variant->value = ConstEvaluator(analyzer, false).evaluate(variant->value);
            next_value = variant->value.as<sir::IntLiteral>().value + 1;
            ExprAnalyzer(analyzer).analyze(variant->value);
        } else {
            variant->value = analyzer.create_expr(sir::IntLiteral{
                .ast_node = nullptr,
                .type = nullptr,
                .value = next_value,
            });

            ExprAnalyzer(analyzer).analyze(variant->value);

            next_value += 1;
        }
    }

    return Result::SUCCESS;
}

} // namespace sema

} // namespace lang

} // namespace banjo