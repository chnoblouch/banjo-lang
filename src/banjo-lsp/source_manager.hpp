#ifndef LSP_SOURCE_MANAGER_H
#define LSP_SOURCE_MANAGER_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_node.hpp"
#include "memory_module_loader.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/sema/semantic_analysis.hpp"
#include "banjo/source/module_manager.hpp"
#include "banjo/symbol/data_type_manager.hpp"

#include <filesystem>
#include <unordered_map>
#include <unordered_set>

namespace banjo {

namespace lsp {

struct SourceFile {
    std::filesystem::path file_path;
    std::string source;
    lang::ModulePath module_path;
    lang::ASTModule *module_node;
    bool is_valid;
};

struct LSPTextPosition {
    int line;
    int column;
};

struct LSPTextRange {
    LSPTextPosition start;
    LSPTextPosition end;
};

struct CompletionContext {
    lang::ASTModule *module_node;
    lang::ASTNode *node;
};

class SourceManager {

    friend class MemoryModuleLoader;

private:
    MemoryModuleLoader module_loader;
    lang::ReportManager report_manager;
    lang::ModuleManager module_manager;

    std::unordered_map<std::string, SourceFile> files;
    lang::DataTypeManager type_manager;
    bool is_reparse_required = true;

public:
    SourceManager();
    void parse_full_source();
    CompletionContext parse_for_completion(std::filesystem::path file_path, lang::TextPosition completion_point);
    lang::SemanticAnalysis analyze_isolated_module(lang::ASTModule *module_);
    void on_file_changed(SourceFile &file);
    SourceFile &get_file(std::filesystem::path path);
    bool has_file(std::filesystem::path path);
    SourceFile &get_file(const lang::ModulePath &path);

    const std::vector<lang::Report> &get_reports() { return report_manager.get_reports(); }
    lang::ModuleManager &get_module_manager() { return module_manager; }

private:
    SourceFile &get_or_load_file(const lang::ModulePath &path, std::filesystem::path absolute_file_path);
    void walk_dependents(lang::ASTModule *mod, std::unordered_set<lang::ASTModule *> &dependents);
};

} // namespace lsp

} // namespace banjo

#endif
