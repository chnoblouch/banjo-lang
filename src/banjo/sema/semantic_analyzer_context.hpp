#ifndef SEMANTIC_ANALYZER_CONTEXT_H
#define SEMANTIC_ANALYZER_CONTEXT_H

#include "banjo/ast/ast_block.hpp"
#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "banjo/ast/module_list.hpp"
#include "banjo/sema/semantic_analysis.hpp"
#include "banjo/source/module_manager.hpp"
#include "banjo/symbol/data_type_manager.hpp"
#include "banjo/symbol/module_path.hpp"
#include "banjo/symbol/symbol_ref.hpp"
#include "banjo/target/target.hpp"

#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace banjo {

namespace lang {

class SemanticAnalyzerContext;
class SemanticAnalyzer;

struct ASTContext {
    ASTModule *cur_module;
    Function *cur_func;
    ASTBlock *cur_block;
    SymbolRef enclosing_symbol = {};
    std::unordered_map<std::string, DataType *> generic_types;

    SymbolTable *get_cur_symbol_table() const;

    template <typename T>
    void set_generic_types(GenericEntityInstance<T> *instance) {
        generic_types.clear();

        for (int i = 0; i < instance->args.size(); i++) {
            const std::string &name = instance->generic_entity->get_generic_params()[i].name;
            DataType *type = instance->args[i];
            generic_types.insert({name, type});
        }
    }

    bool is_generic_type(const std::string &name) const { return generic_types.count(name); }
    DataType *get_generic_type_value(const std::string &name) const { return generic_types.at(name); }
};

struct MoveScope {
    MoveScope *parent;
    ASTNode *node;
    ASTBlock *block;
    std::vector<ValueMove> moves;

    MoveScope(ASTNode *node, ASTBlock *block, SemanticAnalyzerContext &context);
};

class SemanticAnalyzerContext {

private:
    SemanticAnalyzer &sema;

    ModuleManager &module_manager;
    DataTypeManager &type_manager;
    target::Target *target;

    std::stack<ASTContext> ast_contexts;
    std::vector<ASTModule *> new_modules;
    std::vector<std::pair<ASTNode *, ASTContext>> new_nodes;

    std::unordered_map<SymbolTable *, unsigned> symbol_table_meta_stmts;

    bool create_main_func = true;
    bool track_dependencies = false;
    bool track_symbol_uses = false;

    SemanticAnalysis analysis{.is_valid = true, .reports = {}};
    std::vector<Report> new_reports;

    MoveScope *cur_move_scope = nullptr;
    std::vector<ValueMove> cur_sub_scope_moves;

public:
    SemanticAnalyzerContext(
        SemanticAnalyzer &sema,
        ModuleManager &module_manager,
        DataTypeManager &type_manager,
        target::Target *target
    );

    SemanticAnalyzer &get_sema() { return sema; }
    ModuleManager &get_module_manager() { return module_manager; }
    DataTypeManager &get_type_manager() { return type_manager; }
    target::Target *get_target() { return target; }

    Report &register_error(TextRange range);
    Report &register_warning(TextRange range);
    void register_error(std::string message, ASTNode *node);
    void set_invalid();
    SemanticAnalysis &get_analysis() { return analysis; }
    void record_new_reports();
    void discard_new_reports();

    const ASTContext &get_ast_context();
    bool has_ast_context();
    ASTContext &push_ast_context();
    void pop_ast_context();

    std::vector<ASTModule *> &get_new_modules() { return new_modules; }
    std::vector<std::pair<ASTNode *, ASTContext>> &get_new_nodes() { return new_nodes; }
    std::unordered_map<SymbolTable *, unsigned> &get_symbol_table_meta_stmts() { return symbol_table_meta_stmts; }

    bool is_create_main_func() { return create_main_func; }
    bool is_track_dependencies() { return track_dependencies; }
    bool is_track_symbol_uses() { return track_symbol_uses; }

    void set_create_main_func(bool create_main_func) { this->create_main_func = create_main_func; }
    void set_track_dependencies(bool track_dependencies) { this->track_dependencies = track_dependencies; }
    void set_track_symbol_uses(bool track_symbol_uses) { this->track_symbol_uses = track_symbol_uses; }

    template <typename... FormatArgs>
    Report &register_error(ASTNode *node, ReportText::ID text_id, FormatArgs... format_args) {
        SourceLocation location{get_ast_context().cur_module->get_path(), node->get_range()};
        new_reports.push_back({Report::Type::ERROR, {location, text_id, format_args...}});
        return new_reports.back();
    }

    bool is_complete(SymbolTable *symbol_table);
    void add_meta_stmt(SymbolTable *symbol_table);
    void sub_meta_stmt(SymbolTable *symbol_table);

    void process_identifier(SymbolRef symbol, Identifier *use);
    void add_symbol_use(SymbolRef symbol_ref, Identifier *use);

    MoveScope *get_cur_move_scope() { return cur_move_scope; }
    void push_move_scope(MoveScope *move_scope);
    void pop_move_scope();
    void merge_move_scopes_into_parent();
    std::optional<ValueMove> get_prev_move(DeinitInfo *info, MoveScope *scope);
};

enum class SemaResult {
    OK,
    ERROR,
    INCOMPLETE,
};

} // namespace lang

} // namespace banjo

#endif
