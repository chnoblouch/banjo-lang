#include "decl_interface_analyzer.hpp"

#include "banjo/sema/expr_analyzer.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sema/symbol_collector.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_cloner.hpp"

namespace banjo {

namespace lang {

namespace sema {

DeclInterfaceAnalyzer::DeclInterfaceAnalyzer(SemanticAnalyzer &analyzer) : DeclVisitor(analyzer) {}

Result DeclInterfaceAnalyzer::analyze_func_def(sir::FuncDef &func_def) {
    DeclState &state = analyzer.decl_states[*func_def.sema_index];

    if (state.stage < DeclStage::INTERFACE) {
        state.stage = DeclStage::INTERFACE;
    } else {
        return Result::SUCCESS;
    }

    for (unsigned i = 0; i < func_def.type.params.size(); i++) {
        sir::Param &param = func_def.type.params[i];
        analyzer.add_symbol_def(&param);

        if (!(state.scope->decl_parent.is<sir::ProtoDef>() && func_def.is_method())) {
            func_def.block.symbol_table->insert_local(param.name.value, &param);
        }

        if (state.scope->closure_ctx && i == 0) {
            continue;
        }

        analyze_param(i, param);
    }

    ExprAnalyzer(analyzer).analyze_type(func_def.type.return_type);

    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_func_decl(sir::FuncDecl &func_decl) {
    DeclState &state = analyzer.decl_states[*func_decl.sema_index];

    if (state.stage < DeclStage::INTERFACE) {
        state.stage = DeclStage::INTERFACE;
    } else {
        return Result::SUCCESS;
    }

    for (unsigned i = 0; i < func_decl.type.params.size(); i++) {
        analyze_param(i, func_decl.type.params[i]);
    }

    ExprAnalyzer(analyzer).analyze_type(func_decl.type.return_type);

    if (!(state.scope->decl_parent.is<sir::ProtoDef>() && func_decl.is_method())) {
        analyzer.report_generator.report_err_func_decl_outside_proto(func_decl);
    }

    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_native_func_decl(sir::NativeFuncDecl &native_func_decl) {
    DeclState &state = analyzer.decl_states[*native_func_decl.sema_index];

    if (state.stage < DeclStage::INTERFACE) {
        state.stage = DeclStage::INTERFACE;
    } else {
        return Result::SUCCESS;
    }

    for (sir::Param &param : native_func_decl.type.params) {
        ExprAnalyzer(analyzer).analyze_type(param.type);
    }

    ExprAnalyzer(analyzer).analyze_type(native_func_decl.type.return_type);

    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_const_def(sir::ConstDef &const_def) {
    DeclState &state = analyzer.decl_states[*const_def.sema_index];

    if (state.stage < DeclStage::INTERFACE) {
        state.stage = DeclStage::INTERFACE;
    } else {
        return Result::SUCCESS;
    }

    ExprAnalyzer(analyzer).analyze_type(const_def.type);

    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_struct_def(sir::StructDef &struct_def) {
    DeclState &state = analyzer.decl_states[*struct_def.sema_index];

    if (state.stage < DeclStage::INTERFACE) {
        state.stage = DeclStage::INTERFACE;
    } else {
        return Result::SUCCESS;
    }

    Result partial_result;

    for (sir::Expr &impl : struct_def.impls) {
        partial_result = ExprAnalyzer(analyzer).analyze_type(impl);

        if (partial_result != Result::SUCCESS) {
            continue;
        }

        if (auto proto_def = impl.match_symbol<sir::ProtoDef>()) {
            analyze_proto_impl(struct_def, *proto_def);
        } else {
            analyzer.report_generator.report_err_expected_proto(impl);
        }
    }

    return Result::SUCCESS;
}

void DeclInterfaceAnalyzer::analyze_proto_impl(sir::StructDef &struct_def, sir::ProtoDef &proto_def) {
    sir::SymbolTable &symbol_table = *struct_def.block.symbol_table;

    for (sir::ProtoFuncDecl func_decl : proto_def.func_decls) {
        std::string_view name = func_decl.get_ident().value;
        auto iter = symbol_table.symbols.find(name);

        if (iter == symbol_table.symbols.end()) {
            if (auto func_def = func_decl.decl.match<sir::FuncDef>()) {
                insert_default_impl(struct_def, *func_def);
            } else {
                analyzer.report_generator.report_err_impl_missing_func(struct_def, func_decl);
            }
        } else {
            if (!iter->second.is<sir::FuncDef>()) {
                analyzer.report_generator.report_err_impl_missing_func(struct_def, func_decl);
            }
        }
    }
}

void DeclInterfaceAnalyzer::insert_default_impl(sir::StructDef &struct_def, sir::FuncDef &func_def) {
    sir::SymbolTable &parent_symbol_table = *func_def.block.symbol_table->parent;
    sir::FuncDef *clone = sir::Cloner(analyzer.get_mod(), parent_symbol_table).clone_func_def(func_def);
    struct_def.block.decls.push_back(clone);

    SymbolCollector(analyzer).collect_func_def(*clone);
    visit_func_def(*clone);
}

Result DeclInterfaceAnalyzer::analyze_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl) {
    DeclState &state = analyzer.decl_states[*var_decl.sema_index];

    if (state.stage < DeclStage::INTERFACE) {
        state.stage = DeclStage::INTERFACE;
    } else {
        return Result::SUCCESS;
    }

    ExprAnalyzer(analyzer).analyze_type(var_decl.type);

    if (auto struct_def = state.scope->decl_parent.match<sir::StructDef>()) {
        // TODO: Error handling for missing type.

        out_decl = analyzer.create(
            sir::StructField{
                .ast_node = var_decl.ast_node,
                .ident = var_decl.ident,
                .type = var_decl.type,
                .attrs = var_decl.attrs,
                .index = static_cast<unsigned>(struct_def->fields.size()),
            }
        );

        struct_def->fields.push_back(&out_decl.as<sir::StructField>());

        analyzer.add_symbol_def(&out_decl.as<sir::StructField>());
    }

    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_native_var_decl(sir::NativeVarDecl &native_var_decl) {
    DeclState &state = analyzer.decl_states[*native_var_decl.sema_index];

    if (state.stage < DeclStage::INTERFACE) {
        state.stage = DeclStage::INTERFACE;
    } else {
        return Result::SUCCESS;
    }

    Result result = ExprAnalyzer(analyzer).analyze_type(native_var_decl.type);

    return result;
}

Result DeclInterfaceAnalyzer::analyze_enum_variant(sir::EnumVariant &enum_variant) {
    DeclState &state = analyzer.decl_states[*enum_variant.sema_index];

    if (state.stage < DeclStage::INTERFACE) {
        state.stage = DeclStage::INTERFACE;
    } else {
        return Result::SUCCESS;
    }

    if (enum_variant.value) {
        ExprAnalyzer(analyzer).analyze_value(enum_variant.value);
    }

    sir::EnumDef &enum_def = state.scope->decl_parent.as<sir::EnumDef>();
    enum_def.variants.push_back(&enum_variant);

    enum_variant.type = analyzer.create(
        sir::SymbolExpr{
            .ast_node = nullptr,
            .type = nullptr,
            .symbol = &enum_def,
        }
    );

    return Result::SUCCESS;
}

Result DeclInterfaceAnalyzer::analyze_union_case(sir::UnionCase &union_case) {
    DeclState &state = analyzer.decl_states[*union_case.sema_index];

    if (state.stage < DeclStage::INTERFACE) {
        state.stage = DeclStage::INTERFACE;
    } else {
        return Result::SUCCESS;
    }

    for (sir::UnionCaseField &field : union_case.fields) {
        ExprAnalyzer(analyzer).analyze_type(field.type);
    }

    if (auto union_def = state.scope->decl_parent.match<sir::UnionDef>()) {
        union_def->cases.push_back(&union_case);
    } else {
        analyzer.report_generator.report_err_case_outside_union(union_case);
    }

    return Result::SUCCESS;
}

void DeclInterfaceAnalyzer::analyze_param(unsigned index, sir::Param &param) {
    // TODO: Check for duplicate parameter names.

    sir::Symbol parent_decl = analyzer.get_decl_scope().decl_parent;

    if (param.is_self()) {
        if (!parent_decl.is_one_of<sir::StructDef, sir::UnionDef, sir::ProtoDef>()) {
            analyzer.report_generator.report_err_self_not_allowed(param);
            return;
        }

        if (index != 0) {
            analyzer.report_generator.report_err_self_not_first(param);
            return;
        }

        sir::Expr base_type = analyzer.create(
            sir::SymbolExpr{
                .ast_node = nullptr,
                .type = nullptr,
                .symbol = parent_decl,
            }
        );

        if (param.attrs && param.attrs->byval) {
            param.type = base_type;
            return;
        }

        param.type.as<sir::ReferenceType>().base_type = base_type;
    } else {
        ExprAnalyzer(analyzer).analyze_type(param.type);
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
