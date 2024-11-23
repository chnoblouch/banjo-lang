#include "meta_expansion.hpp"

#include "banjo/sema2/const_evaluator.hpp"
#include "banjo/sema2/decl_body_analyzer.hpp"
#include "banjo/sema2/decl_interface_analyzer.hpp"
#include "banjo/sema2/stmt_analyzer.hpp"
#include "banjo/sema2/symbol_collector.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_cloner.hpp"

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
    bool prev_in_meta_expansion = analyzer.in_meta_expansion;
    analyzer.in_meta_expansion = true;

    for (unsigned i = 0; i < decl_block.decls.size(); i++) {
        const sir::Decl &decl = decl_block.decls[i];

        if (analyzer.blocked_decls.contains(&decl)) {
            continue;
        }

        analyzer.blocked_decls.insert(&decl);

        if (decl.is<sir::MetaIfStmt>()) {
            evaluate_meta_if_stmt(decl_block, i);
        }

        analyzer.blocked_decls.erase(&decl);
    }

    analyzer.in_meta_expansion = prev_in_meta_expansion;
    analyzer.incomplete_decl_blocks.erase(&decl_block);
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

    decl_block.decls[index] = analyzer.create_stmt(sir::ExpandedMetaStmt{});

    SymbolCollector(analyzer).collect_in_meta_block(meta_block);

    for (sir::Node &node : meta_block.nodes) {
        decl_block.decls.push_back(node.as<sir::Decl>());
    }

    index--;
}

void MetaExpansion::evaluate_meta_if_stmt(sir::Block &block, unsigned &index) {
    sir::MetaIfStmt &meta_if_stmt = block.stmts[index].as<sir::MetaIfStmt>();

    for (sir::MetaIfCondBranch &cond_branch : meta_if_stmt.cond_branches) {
        if (ConstEvaluator(analyzer).evaluate_to_bool(cond_branch.condition)) {
            expand(block, index, cond_branch.block);
            return;
        }
    }

    if (meta_if_stmt.else_branch) {
        expand(block, index, meta_if_stmt.else_branch->block);
    }
}

void MetaExpansion::evaluate_meta_for_stmt(sir::Block &block, unsigned &index) {
    sir::MetaForStmt &meta_for_stmt = block.stmts[index].as<sir::MetaForStmt>();

    std::vector<sir::Expr> values;
    if (evaluate_meta_for_range(meta_for_stmt.range, values) != Result::SUCCESS) {
        return;
    }

    block.stmts[index] = analyzer.create_stmt(sir::ExpandedMetaStmt{});

    for (sir::Expr &value : values) {
        analyzer.push_scope().generic_args.insert({meta_for_stmt.ident.value, value});

        for (sir::Node &node : meta_for_stmt.block.nodes) {
            index += 1;

            sir::Stmt clone = sir::Cloner(*analyzer.cur_sir_mod).clone_stmt(node.as<sir::Stmt>());
            block.stmts.insert(block.stmts.begin() + index, clone);
            StmtAnalyzer(analyzer).analyze(block, index);
        }

        analyzer.pop_scope();
    }
}

Result MetaExpansion::evaluate_meta_for_range(sir::Expr range, std::vector<sir::Expr> &out_values) {
    sir::Expr evaluated_range = ConstEvaluator(analyzer).evaluate(range);

    if (auto array_literal = evaluated_range.match<sir::ArrayLiteral>()) {
        out_values = array_literal->values;
        return Result::SUCCESS;
    } else if (auto tuple_expr = evaluated_range.match<sir::TupleExpr>()) {
        out_values = tuple_expr->exprs;
        return Result::SUCCESS;
    } else if (auto symbol_expr = evaluated_range.match<sir::SymbolExpr>()) {
        sir::Expr type = symbol_expr->type;

        if (auto tuple_expr = type.match<sir::TupleExpr>()) {
            out_values.resize(tuple_expr->exprs.size());

            for (unsigned i = 0; i < tuple_expr->exprs.size(); i++) {
                out_values[i] = analyzer.create_expr(sir::FieldExpr{
                    .ast_node = nullptr,
                    .type = tuple_expr->exprs[i],
                    .base = symbol_expr,
                    .field_index = i,
                });
            }

            return Result::SUCCESS;
        } else {
            analyzer.report_generator.report_err_compile_time_unknown(range);
            return Result::ERROR;
        }
    } else {
        analyzer.report_generator.report_err_meta_for_cannot_iter(range);
        return Result::ERROR;
    }
}

void MetaExpansion::expand(sir::Block &block, unsigned &index, sir::MetaBlock &meta_block) {
    block.stmts[index] = analyzer.create_stmt(sir::ExpandedMetaStmt{});

    for (sir::Node &node : meta_block.nodes) {
        index += 1;
        block.stmts.insert(block.stmts.begin() + index, node.as<sir::Stmt>());
        StmtAnalyzer(analyzer).analyze(block, index);
    }
}

} // namespace sema

} // namespace lang

} // namespace banjo
