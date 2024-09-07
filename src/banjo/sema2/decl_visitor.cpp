#include "decl_visitor.hpp"

#include "banjo/sir/sir_visitor.hpp"

namespace banjo {

namespace lang {

namespace sema {

DeclVisitor::DeclVisitor(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void DeclVisitor::analyze() {
    for (sir::Module *mod : analyzer.sir_unit.mods) {
        analyzer.enter_mod(mod);
        analyze_decl_block(mod->block);
        analyzer.exit_mod();
    }
}

void DeclVisitor::analyze_decl_block(sir::DeclBlock &decl_block) {
    // Declarations created by meta statement expansion are appended at the end of the declaration list
    // and not analyzed here becuse they get analyzed directly during expansion.
    // This also skips generated closure functions that we don't want to analyze.

    unsigned num_decls = decl_block.decls.size();

    for (unsigned i = 0; i < num_decls; i++) {
        analyze_decl(decl_block.decls[i]);
    }
}

void DeclVisitor::analyze_meta_block(sir::MetaBlock &meta_block) {
    for (sir::Node &node : meta_block.nodes) {
        analyze_decl(node.as<sir::Decl>());
    }
}

Result DeclVisitor::analyze_decl(sir::Decl &decl) {
    SIR_VISIT_DECL(
        decl,
        SIR_VISIT_IMPOSSIBLE,             // empty
        return process_func_def(*inner),  // func_def
        analyze_native_func_decl(*inner), // native_func_decl
        analyze_const_def(*inner),        // const_def
        process_struct_def(*inner),       // struct_def
        SIR_VISIT_IGNORE,                 // struct_field
        analyze_var_decl(*inner, decl),   // var_decl
        analyze_native_var_decl(*inner),  // native_var_decl
        process_enum_def(*inner),         // enum_def
        analyze_enum_variant(*inner),     // enum_variant
        analyze_type_alias(*inner),       // type_alias
        SIR_VISIT_IGNORE,                 // use_decl
        SIR_VISIT_IGNORE,                 // meta_if_stmt
        SIR_VISIT_IGNORE                  // expanded_meta_stmt
    );

    return Result::SUCCESS;
}

Result DeclVisitor::process_func_def(sir::FuncDef &func_def) {
    if (func_def.is_generic()) {
        return Result::SUCCESS;
    }

    return analyze_func_def(func_def);
}

void DeclVisitor::process_struct_def(sir::StructDef &struct_def) {
    if (struct_def.is_generic()) {
        return;
    }

    analyze_struct_def(struct_def);

    analyzer.enter_struct_def(&struct_def);
    analyze_decl_block(struct_def.block);
    analyzer.exit_struct_def();
}

void DeclVisitor::process_enum_def(sir::EnumDef &enum_def) {
    analyze_enum_def(enum_def);

    analyzer.enter_enum_def(&enum_def);
    analyze_decl_block(enum_def.block);
    analyzer.exit_enum_def();
}

} // namespace sema

} // namespace lang

} // namespace banjo
