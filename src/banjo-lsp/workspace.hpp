#ifndef BANJO_LSP_WORKSPACE_H
#define BANJO_LSP_WORKSPACE_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/sema/extra_analysis.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/source/module_manager.hpp"
#include "banjo/source/module_path.hpp"
#include "index.hpp"
#include "memory_module_loader.hpp"

#include <filesystem>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace banjo {

namespace lsp {

struct File {
    std::filesystem::path fs_path;
    lang::ASTModule *ast_module;
    lang::sir::Module *sir_module;
    std::string content;
};

struct CompletionInfo {
    lang::sir::Module sir_mod;
    lang::sema::CompletionContext context;
};

class Workspace {

private:
    MemoryModuleLoader module_loader;
    lang::ModuleManager module_manager;
    lang::ReportManager report_manager;

    lang::sir::Unit sir_unit;
    Index index;

    std::list<File> files;
    std::unordered_map<std::string, File *> files_by_fs_path;
    std::unordered_map<lang::ModulePath, File *> files_by_mod_path;

    lang::Config &config;
    std::unique_ptr<target::Target> target;

public:
    Workspace();

    std::vector<lang::sir::Module *> initialize();
    std::vector<lang::sir::Module *> update(const std::filesystem::path &fs_path, std::string new_content);
    CompletionInfo run_completion(const File *file, lang::TextPosition completion_point);

    File *find_file(const std::filesystem::path &fs_path);
    File *find_file(const lang::ModulePath &mod_path);
    File *find_file(const lang::sir::Module &mod);
    File *find_or_load_file(const std::filesystem::path &fs_path);

    Index &get_index() { return index; }
    ModuleIndex *find_index(lang::sir::Module *mod);
    const SymbolRef &get_index_symbol(const SymbolKey &key);

    std::vector<lang::ModulePath> list_sub_mods(lang::sir::Module *mod);

private:
    void build_index(lang::sema::ExtraAnalysis &analysis, const std::vector<lang::sir::Module *> &mods);
    void collect_dependents(lang::sir::Module &mod, std::unordered_set<lang::ModulePath> &dependents);
};

} // namespace lsp

} // namespace banjo

#endif
