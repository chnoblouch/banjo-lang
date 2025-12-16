#include "decl_visitor.hpp"

#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_visitor.hpp"
#include <variant>

namespace banjo {

namespace lang {

namespace sema {

DeclVisitor::DeclVisitor(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void DeclVisitor::analyze(const std::vector<sir::Module *> &mods) {
    for (sir::Module *mod : mods) {
        analyzer.enter_decl(mod);
        analyze_decl_block(mod->block);
        analyzer.exit_decl();
    }
}

void DeclVisitor::analyze_decl_block(sir::DeclBlock &decl_block) {
    // Declarations created by meta statement expansion are appended at the end of the declaration list
    // and not analyzed here becuse they get analyzed directly during expansion.
    // This also skips generated closure functions that we don't want to analyze.

    unsigned num_decls = decl_block.decls.size();

    for (unsigned i = 0; i < num_decls; i++) {
        // Stop here if a completion context has been filled it so we don't override it.
        if (!std::holds_alternative<std::monostate>(analyzer.get_completion_context())) {
            return;
        }

        analyze_decl(decl_block.decls[i]);
    }
}

void DeclVisitor::visit_func_def(sir::FuncDef &func_def) {
    if (func_def.is_generic()) {
        for (sir::Specialization<sir::FuncDef> &specialization : func_def.specializations) {
            visit_func_def(*specialization.def);
        }
    } else if (func_def.parent_specialization) {
        analyzer.enter_decl(&func_def);
        analyzer.scope_stack.top().symbol_table = func_def.parent_specialization->symbol_table;
        analyze_func_def(func_def);
        analyzer.exit_decl();
    } else {
        analyzer.enter_decl(&func_def);
        analyze_func_def(func_def);
        analyzer.exit_decl();
    }
}

void DeclVisitor::visit_func_decl(sir::FuncDecl &func_decl) {
    analyzer.enter_decl(&func_decl);
    analyze_func_decl(func_decl);
    analyzer.exit_decl();
}

void DeclVisitor::visit_native_func_decl(sir::NativeFuncDecl &native_func_decl) {
    analyzer.enter_decl(&native_func_decl);
    analyze_native_func_decl(native_func_decl);
    analyzer.exit_decl();
}

Result DeclVisitor::visit_const_def(sir::ConstDef &const_def) {
    analyzer.enter_decl(&const_def);
    Result result = analyze_const_def(const_def);
    analyzer.exit_decl();

    return result;
}

void DeclVisitor::visit_struct_def(sir::StructDef &struct_def) {
    if (struct_def.is_generic()) {
        for (sir::Specialization<sir::StructDef> &specialization : struct_def.specializations) {
            visit_struct_def(*specialization.def);
        }
    } else {
        analyzer.enter_decl(&struct_def);
        analyze_struct_def(struct_def);
        analyze_decl_block(struct_def.block);
        analyzer.exit_decl();
    }
}

void DeclVisitor::visit_var_decl(sir::VarDecl &var_decl, sir::Decl &out_decl) {
    analyzer.enter_decl(&var_decl);
    analyze_var_decl(var_decl, out_decl);
    analyzer.exit_decl();
}

void DeclVisitor::visit_native_var_decl(sir::NativeVarDecl &native_var_decl) {
    analyzer.enter_decl(&native_var_decl);
    analyze_native_var_decl(native_var_decl);
    analyzer.exit_decl();
}

void DeclVisitor::visit_enum_def(sir::EnumDef &enum_def) {
    analyzer.enter_decl(&enum_def);
    analyze_enum_def(enum_def);
    analyze_decl_block(enum_def.block);
    analyzer.exit_decl();
}

void DeclVisitor::visit_enum_variant(sir::EnumVariant &enum_variant) {
    analyzer.enter_decl(&enum_variant);
    analyze_enum_variant(enum_variant);
    analyzer.exit_decl();
}

void DeclVisitor::visit_union_def(sir::UnionDef &union_def) {
    analyzer.enter_decl(&union_def);
    analyze_union_def(union_def);
    analyze_decl_block(union_def.block);
    analyzer.exit_decl();
}

void DeclVisitor::visit_union_case(sir::UnionCase &union_case) {
    analyzer.enter_decl(&union_case);
    analyze_union_case(union_case);
    analyzer.exit_decl();
}

void DeclVisitor::visit_proto_def(sir::ProtoDef &proto_def) {
    analyzer.enter_decl(&proto_def);
    analyze_proto_def(proto_def);
    analyze_decl_block(proto_def.block);
    analyzer.exit_decl();
}

void DeclVisitor::visit_type_alias(sir::TypeAlias &type_alias) {
    analyzer.enter_decl(&type_alias);
    analyze_type_alias(type_alias);
    analyzer.exit_decl();
}

void DeclVisitor::visit_closure_def(sir::FuncDef &func_def, ClosureContext &closure_ctx) {
    analyzer.enter_decl(&func_def);
    analyzer.enter_block(func_def.block);
    analyzer.enter_closure_ctx(closure_ctx);
    analyze_func_def(func_def);
    analyzer.exit_closure_ctx();
    analyzer.exit_block();
    analyzer.exit_decl();
}

Result DeclVisitor::analyze_decl(sir::Decl &decl) {
    SIR_VISIT_DECL(
        decl,
        SIR_VISIT_IMPOSSIBLE,           // empty
        visit_func_def(*inner),         // func_def
        visit_func_decl(*inner),        // func_decl
        visit_native_func_decl(*inner), // native_func_decl
        visit_const_def(*inner),        // const_def
        visit_struct_def(*inner),       // struct_def
        SIR_VISIT_IGNORE,               // struct_field
        visit_var_decl(*inner, decl),   // var_decl
        visit_native_var_decl(*inner),  // native_var_decl
        visit_enum_def(*inner),         // enum_def
        visit_enum_variant(*inner),     // enum_variant
        visit_union_def(*inner),        // union_def
        visit_union_case(*inner),       // union_case
        visit_proto_def(*inner),        // proto_def
        visit_type_alias(*inner),       // type_alias
        SIR_VISIT_IGNORE,               // use_decl
        SIR_VISIT_IGNORE,               // meta_if_stmt
        SIR_VISIT_IGNORE,               // expanded_meta_stmt
        SIR_VISIT_IGNORE                // error
    );

    return Result::SUCCESS;
}

} // namespace sema

} // namespace lang

} // namespace banjo
