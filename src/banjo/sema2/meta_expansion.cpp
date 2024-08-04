#include "meta_expansion.hpp"

#include "banjo/sema2/const_evaluator.hpp"
#include "banjo/sema2/decl_body_analyzer.hpp"
#include "banjo/sema2/decl_interface_analyzer.hpp"
#include "banjo/sema2/decl_value_analyzer.hpp"
#include "banjo/sema2/symbol_collector.hpp"
#include "banjo/sir/sir.hpp"

namespace banjo {

namespace lang {

namespace sema {

MetaExpansion::MetaExpansion(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void MetaExpansion::run() {
    for (sir::Module *mod : analyzer.sir_unit.mods) {
        analyzer.enter_mod(mod);
        run_on_decl_block(mod->block);
        analyzer.exit_mod();
    }
}

void MetaExpansion::run_on_decl_block(sir::DeclBlock &decl_block) {
    for (unsigned i = 0; i < decl_block.decls.size(); i++) {
        const sir::Decl &decl = decl_block.decls[i];

        if (analyzer.blocked_decls.contains(&decl)) {
            continue;
        }

        analyzer.blocked_decls.insert(&decl);

        if (auto meta_if_stmt = decl.match<sir::MetaIfStmt>()) {
            evaluate_meta_if_stmt(decl_block, i);
        }

        analyzer.blocked_decls.erase(&decl);
    }

    analyzer.check_for_completeness(decl_block);
}

void MetaExpansion::evaluate_meta_if_stmt(sir::DeclBlock &decl_block, unsigned &index) {
    sir::MetaIfStmt &meta_if_stmt = decl_block.decls[index].as<sir::MetaIfStmt>();

    for (sir::MetaIfCondBranch &cond_branch : meta_if_stmt.cond_branches) {
        if (ConstEvaluator(analyzer).evaluate_to_bool(cond_branch.condition)) {
            expand(decl_block, index, cond_branch.block);
            return;
        }
    }

    if (meta_if_stmt.else_branch) {
        expand(decl_block, index, meta_if_stmt.else_branch->block);
    }
}

void MetaExpansion::expand(sir::DeclBlock &decl_block, unsigned &index, sir::MetaBlock &meta_block) {
    analyzer.blocked_decls.clear();

    decl_block.decls[index] = analyzer.create_stmt(sir::ExpandedMetaStmt{
        .ast_node = decl_block.ast_node,
    });

    SymbolCollector(analyzer).collect_in_meta_block(meta_block);
    DeclInterfaceAnalyzer(analyzer).analyze_meta_block(meta_block);
    DeclValueAnalyzer(analyzer).analyze_meta_block(meta_block);
    DeclBodyAnalyzer(analyzer).analyze_meta_block(meta_block);

    // TODO: nested meta expansion

    for (sir::Node &node : meta_block.nodes) {
        decl_block.decls.push_back(node.as<sir::Decl>());
    }

    index--;
}

} // namespace sema

} // namespace lang

} // namespace banjo
