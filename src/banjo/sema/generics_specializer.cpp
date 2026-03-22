#include "generics_specializer.hpp"

#include "banjo/sema/decl_body_analyzer.hpp"
#include "banjo/sema/decl_interface_analyzer.hpp"
#include "banjo/sema/meta_expansion.hpp"
#include "banjo/sema/resource_analyzer.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sema/symbol_collector.hpp"
#include "banjo/sema/type_alias_resolver.hpp"
#include "banjo/sema/use_resolver.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_cloner.hpp"
#include "banjo/utils/macros.hpp"

namespace banjo {

namespace lang {

namespace sema {

GenericsSpecializer::GenericsSpecializer(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

sir::FuncDef *GenericsSpecializer::specialize(sir::FuncDef &generic_func_def, std::span<sir::Expr> args) {
    ASSERT(args.size() == generic_func_def.generic_params.size());

    if (sir::FuncDef *existing_specialization = find_existing_specialization(generic_func_def, args)) {
        return existing_specialization;
    } else {
        return create_specialized_clone(generic_func_def, args);
    }
}

sir::StructDef *GenericsSpecializer::specialize(sir::StructDef &generic_struct_def, std::span<sir::Expr> args) {
    ASSERT(args.size() == generic_struct_def.generic_params.size());

    if (sir::StructDef *existing_specialization = find_existing_specialization(generic_struct_def, args)) {
        return existing_specialization;
    } else {
        return create_specialized_clone(generic_struct_def, args);
    }
}

sir::FuncDef *GenericsSpecializer::create_specialized_clone(sir::FuncDef &generic_func_def, std::span<sir::Expr> args) {
    Context ctx{
        .params = generic_func_def.generic_params,
        .args = args,
    };

    sir::FuncDef *clone = generic_func_def.find_mod().create(
        sir::FuncDef{
            .ast_node = generic_func_def.ast_node,
            .ident = generic_func_def.ident,
            .parent = generic_func_def.parent,
            .type = specialize_func_type_directly(ctx, generic_func_def.type),
            .block = generic_func_def.block,
            .attrs = generic_func_def.attrs,
            .stage = generic_func_def.stage,
        }
    );

    generic_func_def.specializations.push_back(
        sir::Specialization<sir::FuncDef>{
            .generic_def = &generic_func_def,
            .args = args,
            .def = clone,
            .symbol_table = nullptr,
        }
    );

    clone->parent_specialization = &generic_func_def.specializations.back();

    if (analyzer.mode == Mode::COMPLETION) {
        analyzer.get_completion_infection().func_specializations[&generic_func_def] += 1;
    }

    return clone;
}

sir::StructDef *GenericsSpecializer::create_specialized_clone(
    sir::StructDef &generic_struct_def,
    std::span<sir::Expr> args
) {
    Context ctx{
        .params = generic_struct_def.generic_params,
        .args = args,
    };

    std::vector<sir::StructField *> fields{generic_struct_def.fields.size()};

    for (unsigned i = 0; i < generic_struct_def.fields.size(); i++) {
        sir::StructField &field = *generic_struct_def.fields[i];

        fields[i] = analyzer.create(
            sir::StructField{
                .ast_node = field.ast_node,
                .ident = field.ident,
                .type = specialize_expr(ctx, field.type),
                .attrs = field.attrs,
                .index = field.index,
            }
        );
    }

    sir::StructDef *clone = generic_struct_def.find_mod().create(
        sir::StructDef{
            .ast_node = generic_struct_def.ast_node,
            .ident = generic_struct_def.ident,
            .parent = generic_struct_def.parent,
            .block = generic_struct_def.block,
            .fields = fields,
            .impls = generic_struct_def.impls,
            .attrs = generic_struct_def.attrs,
            .stage = generic_struct_def.stage,
        }
    );

    clone->block.symbol_table = generic_struct_def.find_mod().create(
        sir::SymbolTable{
            .parent = clone->block.symbol_table->parent,
            .symbols = clone->block.symbol_table->symbols,
        }
    );

    for (auto &[name, symbol] : clone->block.symbol_table->symbols) {
        if (auto func_def = symbol.match<sir::FuncDef>()) {
            symbol = specialize(*func_def, args);
        }
    }

    generic_struct_def.specializations.push_back(
        sir::Specialization<sir::StructDef>{
            .generic_def = &generic_struct_def,
            .args = args,
            .def = clone,
            .symbol_table = nullptr,
        }
    );

    clone->parent_specialization = &generic_struct_def.specializations.back();

    if (analyzer.mode == Mode::COMPLETION) {
        analyzer.get_completion_infection().struct_specializations[&generic_struct_def] += 1;
    }

    return clone;
}

sir::FuncType GenericsSpecializer::specialize_func_type_directly(Context &ctx, sir::FuncType &func_type) {
    std::span<sir::Param> params = analyzer.allocate_array<sir::Param>(func_type.params.size());

    for (unsigned i = 0; i < func_type.params.size(); i++) {
        const sir::Param &param = func_type.params[i];

        params[i] = sir::Param{
            .ast_node = param.ast_node,
            .name = param.name,
            .type = specialize_expr(ctx, param.type),
            .attrs = param.attrs,
        };
    }

    return sir::FuncType{
        .ast_node = func_type.ast_node,
        .params = params,
        .return_type = specialize_expr(ctx, func_type.return_type),
    };
}

sir::Expr GenericsSpecializer::specialize_expr(Context &ctx, sir::Expr expr) {
    if (auto symbol_expr = expr.match<sir::SymbolExpr>()) {
        return specialize_symbol_expr(ctx, *symbol_expr);
    } else {
        return expr;
    }
}

sir::Expr GenericsSpecializer::specialize_symbol_expr(Context &ctx, sir::SymbolExpr &symbol_expr) {
    if (auto generic_param = symbol_expr.symbol.match<sir::GenericParam>()) {
        for (unsigned i = 0; i < ctx.params.size(); i++) {
            if (&ctx.params[i] == generic_param) {
                return ctx.args[i];
            }
        }

        ASSERT_UNREACHABLE;
    } else {
        return &symbol_expr;
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
