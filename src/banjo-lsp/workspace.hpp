#ifndef BANJO_LSP_WORKSPACE_H
#define BANJO_LSP_WORKSPACE_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/sema/completion_context.hpp"
#include "banjo/sema/extra_analysis.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/source/module_manager.hpp"
#include "banjo/source/module_path.hpp"
#include "index.hpp"

#include <filesystem>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

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
    lang::sema::CompletionInfection infection;
    std::vector<lang::sir::Symbol> preamble_symbols;
};

class Workspace {

private:
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

    CompletionInfo run_completion(
        const File *file,
        lang::TextPosition completion_point,
        lang::sir::Module &out_sir_mod
    );
    void undo_infection(lang::sema::CompletionInfection &infection);

    File *find_file(const std::filesystem::path &fs_path);
    File *find_file(const lang::ModulePath &mod_path);
    File *find_file(const lang::sir::Module &mod);
    File *find_or_load_file(const std::filesystem::path &fs_path);

    Index &get_index() { return index; }
    ModuleIndex *find_index(lang::sir::Module *mod);
    const SymbolRef &get_index_symbol(const SymbolKey &key);

    const lang::ModuleList &get_mod_list() { return module_manager.get_module_list(); }
    std::vector<lang::ModulePath> list_sub_mods(lang::sir::Module *mod);

private:
    std::unique_ptr<std::istream> open_module(const lang::ModuleFile &module_file);

    void build_index(lang::sema::ExtraAnalysis &analysis, const std::vector<lang::sir::Module *> &mods);
    void collect_dependents(lang::sir::Module &mod, std::unordered_set<lang::ModulePath> &dependents);
};

} // namespace lsp

} // namespace banjo

#endif
