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
#include "banjo/source/source_file.hpp"
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
    std::unordered_map<lang::sir::Symbol, SymbolKey> symbol_defs;
    Index index;

    lang::Config &config;
    std::unique_ptr<target::Target> target;

public:
    Workspace();

    std::vector<lang::SourceFile *> initialize();
    std::vector<lang::SourceFile *> update(const std::filesystem::path &fs_path, std::string new_content);

    CompletionInfo run_completion(
        lang::SourceFile *file,
        lang::TextPosition completion_point,
        lang::sir::Module &out_sir_mod
    );
    void undo_infection(lang::sema::CompletionInfection &infection);

    lang::SourceFile *find_file(const std::filesystem::path &fs_path);
    lang::SourceFile *find_file(const lang::ModulePath &mod_path);

    Index &get_index() { return index; }
    ModuleIndex *find_index(lang::sir::Module *mod);
    const SymbolRef &get_index_symbol(const SymbolKey &key);

    const lang::ModuleList &get_mod_list() { return module_manager.get_module_list(); }

private:
    void build_index(lang::sema::ExtraAnalysis &analysis, const std::vector<lang::sir::Module *> &mods);
    void collect_dependents(lang::sir::Module &mod, std::unordered_set<lang::ModulePath> &dependents);
};

} // namespace lsp

} // namespace banjo

#endif
