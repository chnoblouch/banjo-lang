#include "semantic_analyzer_context.hpp"

#include "ast/ast_utils.hpp"
#include "ast/expr.hpp"
#include "symbol/enumeration.hpp"
#include "symbol/symbol.hpp"
#include "symbol/symbol_ref.hpp"

namespace lang {

SymbolTable *ASTContext::get_cur_symbol_table() const {
    if (enclosing_symbol.get_kind() == SymbolKind::STRUCT) {
        return enclosing_symbol.get_struct()->get_symbol_table();
    } else if (enclosing_symbol.get_kind() == SymbolKind::UNION) {
        return enclosing_symbol.get_union()->get_symbol_table();
    } else {
        return ASTUtils::get_module_symbol_table(cur_module);
    }
}

SemanticAnalyzerContext::SemanticAnalyzerContext(
    SemanticAnalyzer &sema,
    ModuleManager &module_manager,
    DataTypeManager &type_manager,
    target::Target *target
)
  : sema(sema),
    module_manager(module_manager),
    type_manager(type_manager),
    target(target) {}

Report &SemanticAnalyzerContext::register_error(TextRange range) {
    SourceLocation location{get_ast_context().cur_module->get_path(), range};
    new_reports.push_back(Report(Report::Type::ERROR, location));
    return new_reports.back();
}

Report &SemanticAnalyzerContext::register_warning(TextRange range) {
    SourceLocation location{get_ast_context().cur_module->get_path(), range};
    new_reports.push_back(Report(Report::Type::WARNING, location));
    return new_reports.back();
}

void SemanticAnalyzerContext::register_error(std::string message, ASTNode *node) {
    register_error(node->get_range()).set_message(message);
}

void SemanticAnalyzerContext::set_invalid() {
    analysis.is_valid = false;
}

void SemanticAnalyzerContext::record_new_reports() {
    analysis.reports.insert(analysis.reports.end(), new_reports.begin(), new_reports.end());
    new_reports.clear();

    for (const Report &report : analysis.reports) {
        if (report.get_type() == Report::Type::ERROR) {
            analysis.is_valid = false;
            break;
        }
    }
}

void SemanticAnalyzerContext::discard_new_reports() {
    new_reports.clear();
}

const ASTContext &SemanticAnalyzerContext::get_ast_context() {
    return ast_contexts.top();
}

bool SemanticAnalyzerContext::has_ast_context() {
    return !ast_contexts.empty();
}

ASTContext &SemanticAnalyzerContext::push_ast_context() {
    if (!ast_contexts.empty()) {
        ast_contexts.push(get_ast_context());
    } else {
        ast_contexts.push({});
    }

    return ast_contexts.top();
}

void SemanticAnalyzerContext::pop_ast_context() {
    ast_contexts.pop();
}

bool SemanticAnalyzerContext::is_complete(SymbolTable *symbol_table) {
    auto iter = symbol_table_meta_stmts.find(symbol_table);
    return iter == symbol_table_meta_stmts.end() || iter->second == 0;
}

void SemanticAnalyzerContext::add_meta_stmt(SymbolTable *symbol_table) {
    symbol_table_meta_stmts[symbol_table] += 1;
}

void SemanticAnalyzerContext::sub_meta_stmt(SymbolTable *symbol_table) {
    symbol_table_meta_stmts[symbol_table] -= 1;
}

void SemanticAnalyzerContext::process_identifier(SymbolRef symbol, Identifier *use) {
    use->set_symbol(symbol);
    add_symbol_use(symbol, use);
}

void SemanticAnalyzerContext::add_symbol_use(SymbolRef symbol_ref, Identifier *use) {
    if (!track_symbol_uses) {
        return;
    }

    Symbol *symbol = symbol_ref.get_symbol();
    if (symbol) {
        ASTModule *cur_module = get_ast_context().cur_module;
        symbol->add_use({cur_module, use});
    }
}

} // namespace lang
