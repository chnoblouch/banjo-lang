#include "decl_body_analyzer.hpp"

#include "banjo/sema/const_evaluator.hpp"
#include "banjo/sema/expr_analyzer.hpp"
#include "banjo/sema/expr_finalizer.hpp"
#include "banjo/sema/return_checker.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sema/stmt_analyzer.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

DeclBodyAnalyzer::DeclBodyAnalyzer(SemanticAnalyzer &analyzer) : DeclVisitor(analyzer) {}

Result DeclBodyAnalyzer::analyze_func_def(sir::FuncDef &func_def) {
    if (func_def.stage < sir::SemaStage::BODY) {
        func_def.stage = sir::SemaStage::BODY;
    } else {
        return Result::SUCCESS;
    }

    if (func_def.parent.is<sir::ProtoDef>() && func_def.is_method()) {
        return Result::SUCCESS;
    }

    StmtAnalyzer(analyzer).analyze_block(func_def.block);

    ReturnChecker::Result return_checker_result = ReturnChecker(analyzer).check(func_def.block);
    bool has_return_value = !func_def.type.return_type.is_primitive_type(sir::Primitive::VOID);

    if (!has_return_value || return_checker_result == ReturnChecker::Result::RETURNS_ALWAYS) {
        return Result::SUCCESS;
    } else {
        if (return_checker_result == ReturnChecker::Result::RETURNS_SOMETIMES) {
            analyzer.report_generator.report_err_does_not_always_return(func_def.ident);
        } else if (return_checker_result == ReturnChecker::Result::RETURNS_NEVER) {
            analyzer.report_generator.report_err_does_not_return(func_def.ident);
        }

        return Result::ERROR;
    }
}

Result DeclBodyAnalyzer::analyze_const_def(sir::ConstDef &const_def) {
    if (const_def.stage >= sir::SemaStage::BODY) {
        return Result::SUCCESS;
    }

    if (!const_def.type) {
        return Result::ERROR;
    }

    if (std::ranges::count(analyzer.decl_stack, sir::Decl{&const_def})) {
        analyzer.report_generator.report_err_cyclical_definition(const_def.value);
        const_def.stage = sir::SemaStage::BODY;
        return Result::DEF_CYCLE;
    }

    Result result = ExprAnalyzer{analyzer}.analyze_value(const_def.value, const_def.type);

    if (result != Result::SUCCESS) {
        const_def.stage = sir::SemaStage::BODY;
        return result;
    }

    analyzer.decl_stack.push_back(&const_def);
    ConstEvaluator::Output evaluated = ConstEvaluator{analyzer}.evaluate(const_def.value);
    analyzer.decl_stack.pop_back();

    if (evaluated.result != Result::SUCCESS) {
        const_def.stage = sir::SemaStage::BODY;
        return evaluated.result;
    }

    const_def.value = evaluated.expr;
    const_def.stage = sir::SemaStage::BODY;

    return result;
}

Result DeclBodyAnalyzer::analyze_struct_def(sir::StructDef &struct_def) {
    if (struct_def.stage < sir::SemaStage::BODY) {
        struct_def.stage = sir::SemaStage::BODY;
    } else {
        return Result::SUCCESS;
    }

    for (sir::Expr &impl : struct_def.impls) {
        if (auto proto_def = impl.match_symbol<sir::ProtoDef>()) {
            analyze_proto_impl(struct_def, *proto_def);
        }
    }

    // TODO: This should probably be checked during interface analysis.
    if (struct_def.get_layout() == sir::Attributes::Layout::OVERLAPPING && struct_def.fields.empty()) {
        analyzer.report_generator.report_err_struct_overlapping_no_fields(struct_def);
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

Result DeclBodyAnalyzer::analyze_var_decl(sir::VarDecl &var_decl, sir::Decl & /*out_decl*/) {
    if (var_decl.stage < sir::SemaStage::BODY) {
        var_decl.stage = sir::SemaStage::BODY;
    } else {
        return Result::SUCCESS;
    }

    Result partial_result;

    if (!var_decl.type) {
        return Result::ERROR;
    }

    if (var_decl.value) {
        partial_result = ExprAnalyzer(analyzer).analyze_uncoerced(var_decl.value);
        if (partial_result != Result::SUCCESS) {
            return Result::ERROR;
        }

        ConstEvaluator::Output evaluated = ConstEvaluator{analyzer}.evaluate(var_decl.value);
        if (evaluated.result != Result::SUCCESS) {
            return Result::ERROR;
        }

        var_decl.value = evaluated.expr;

        partial_result = ExprFinalizer(analyzer).finalize_by_coercion(var_decl.value, var_decl.type);
        if (partial_result != Result::SUCCESS) {
            return Result::SUCCESS;
        }
    }

    return Result::SUCCESS;
}

Result DeclBodyAnalyzer::analyze_enum_def(sir::EnumDef &enum_def) {
    if (enum_def.stage < sir::SemaStage::BODY) {
        enum_def.stage = sir::SemaStage::BODY;
    } else {
        return Result::SUCCESS;
    }

    Result partial_result;
    LargeInt next_value = 0;

    for (sir::EnumVariant *variant : enum_def.variants) {
        if (variant->value) {
            partial_result = ExprAnalyzer(analyzer).analyze_uncoerced(variant->value);
            if (partial_result != Result::SUCCESS) {
                continue;
            }

            ConstEvaluator::Output evaluated = ConstEvaluator{analyzer}.evaluate(variant->value);
            if (evaluated.result != Result::SUCCESS) {
                continue;
            }

            variant->value = evaluated.expr;
            next_value = variant->value.as<sir::IntLiteral>().value + 1;
            ExprFinalizer(analyzer).finalize(variant->value);
        } else {
            variant->value = analyzer.create(
                sir::IntLiteral{
                    .ast_node = nullptr,
                    .type = nullptr,
                    .value = next_value,
                }
            );

            ExprAnalyzer(analyzer).analyze_value(variant->value);

            next_value += 1;
        }
    }

    return Result::SUCCESS;
}

} // namespace sema

} // namespace lang

} // namespace banjo
