#include "meta_expansion.hpp"

#include "banjo/sema/const_evaluator.hpp"
#include "banjo/sema/expr_analyzer.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sema/stmt_analyzer.hpp"
#include "banjo/sema/symbol_collector.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_cloner.hpp"

namespace banjo {

namespace lang {

namespace sema {

MetaExpansion::MetaExpansion(SemanticAnalyzer &analyzer) : analyzer(analyzer) {}

void MetaExpansion::run(const std::vector<sir::Module *> &mods) {
    for (sir::Module *mod : mods) {
        analyzer.enter_decl(mod);
        run_on_decl_block(mod->block);
        analyzer.exit_decl();
    }
}

void MetaExpansion::run_on_decl_block(sir::DeclBlock &decl_block) {
    for (unsigned i = 0; i < decl_block.decls.size(); i++) {
        sir::Decl &decl = decl_block.decls[i];

        if (analyzer.blocked_decls.contains(&decl)) {
            continue;
        }

        analyzer.blocked_decls.insert(&decl);

        // TODO: This code is duplicated and brittle.

        if (auto struct_def = decl.match<sir::StructDef>()) {
            if (!struct_def->is_generic()) {
                analyzer.enter_decl(struct_def);
                run_on_decl_block(struct_def->block);
                analyzer.exit_decl();
            }
        } else if (auto union_def = decl.match<sir::UnionDef>()) {
            analyzer.enter_decl(union_def);
            run_on_decl_block(union_def->block);
            analyzer.exit_decl();
        } else if (auto proto_def = decl.match<sir::ProtoDef>()) {
            analyzer.enter_decl(proto_def);
            run_on_decl_block(proto_def->block);
            analyzer.exit_decl();
        } else if (decl.is<sir::MetaIfStmt>()) {
            evaluate_meta_if_stmt(decl_block, i);
        }

        analyzer.blocked_decls.erase(&decl);
    }
}

void MetaExpansion::evaluate_meta_if_stmt(sir::DeclBlock &decl_block, unsigned &index) {
    sir::Decl decl = decl_block.decls[index];

    if (decl.is<sir::ExpandedMetaStmt>()) {
        return;
    }

    sir::MetaIfStmt &meta_if_stmt = decl.as<sir::MetaIfStmt>();

    // bool is_guard = analyzer.get_mod().path == ModulePath{"main"};
    // bool is_guard = false;

    // if (is_guard) {
    //     analyzer.symbol_ctx.push_meta_condition();
    // }

    for (sir::MetaIfCondBranch &cond_branch : meta_if_stmt.cond_branches) {
        ExprAnalyzer expr_analyzer{
            analyzer,
            ExprAnalyzer::DONT_EVAL_META_EXPRS | ExprAnalyzer::ANALYZE_SYMBOL_INTERFACES,
        };

        if (expr_analyzer.analyze_uncoerced(cond_branch.condition) != Result::SUCCESS) {
            return;
        }

        // Evaluating the expression may have already expanded this `meta if` statement.
        if (!decl_block.decls[index].is<sir::MetaIfStmt>() /* && !is_guard */) {
            return;
        }

        if (ConstEvaluator{analyzer}.evaluate_to_bool(cond_branch.condition) /* || is_guard */) {
            // if (is_guard) {
            //     analyzer.symbol_ctx.add_next_meta_condition(cond_branch.condition);
            // }

            expand(decl_block, index, *std::get<sir::DeclBlock *>(cond_branch.block));
            return;

            // if (!is_guard) {
            //     return;
            // }
        }
    }

    if (meta_if_stmt.else_branch) {
        // if (is_guard) {
        //     analyzer.symbol_ctx.add_else_meta_condition();
        // }

        expand(decl_block, index, *std::get<sir::DeclBlock *>(meta_if_stmt.else_branch->block));
    }

    // if (is_guard) {
    //     analyzer.symbol_ctx.pop_meta_condition();
    // }
}

void MetaExpansion::expand(sir::DeclBlock &decl_block, unsigned &index, sir::DeclBlock &body) {
    analyzer.blocked_decls.clear();

    decl_block.decls[index] = analyzer.create(sir::ExpandedMetaStmt{});

    SymbolCollector(analyzer).collect_in_block(body);

    for (sir::Decl decl : body.decls) {
        decl_block.decls.push_back(decl);
    }
}

void MetaExpansion::evaluate_meta_if_stmt(sir::Block &block, unsigned &index) {
    sir::MetaIfStmt &meta_if_stmt = block.stmts[index].as<sir::MetaIfStmt>();

    // bool is_guard = analyzer.get_mod().path == ModulePath{"main"};
    bool is_guard = false;

    if (is_guard) {
        analyzer.symbol_ctx.push_meta_condition();
    }

    for (sir::MetaIfCondBranch &cond_branch : meta_if_stmt.cond_branches) {
        ExprAnalyzer expr_analyzer{
            analyzer,
            ExprAnalyzer::DONT_EVAL_META_EXPRS | ExprAnalyzer::ANALYZE_SYMBOL_INTERFACES,
        };

        if (expr_analyzer.analyze_uncoerced(cond_branch.condition) != Result::SUCCESS) {
            return;
        }

        if (ConstEvaluator{analyzer}.evaluate_to_bool(cond_branch.condition) || is_guard) {
            if (is_guard) {
                analyzer.symbol_ctx.add_next_meta_condition(cond_branch.condition);
            }

            expand(block, index, *std::get<sir::Block *>(cond_branch.block));

            if (!is_guard) {
                return;
            }
        }
    }

    if (meta_if_stmt.else_branch) {
        if (is_guard) {
            analyzer.symbol_ctx.add_else_meta_condition();
        }

        expand(block, index, *std::get<sir::Block *>(meta_if_stmt.else_branch->block));
    }

    if (is_guard) {
        analyzer.symbol_ctx.pop_meta_condition();
    }
}

void MetaExpansion::expand(sir::Block &block, unsigned &index, sir::Block &body) {
    block.stmts[index] = analyzer.create(sir::ExpandedMetaStmt{});

    for (sir::Stmt stmt : body.stmts) {
        index += 1;
        block.stmts.insert(block.stmts.begin() + index, stmt);
        StmtAnalyzer(analyzer).analyze(block, index);
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
