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

#include "completion_engine.hpp"
#include "index.hpp"

#include <filesystem>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace banjo::lsp {

struct CompletionInfo {
    sir::Module sir_mod;
    sema::CompletionContext context;
    std::vector<sir::Symbol> preamble_symbols;
};

class Workspace {

private:
    ModuleManager module_manager;
    ReportManager report_manager;

    sir::Unit sir_unit;
    std::unordered_map<sir::Symbol, SymbolKey> symbol_defs;
    Index index;

    Config &config;
    std::unique_ptr<target::Target> target;

public:
    CompletionEngine completion_engine;

public:
    Workspace();

    std::vector<SourceFile *> initialize();
    std::vector<SourceFile *> update(const std::filesystem::path &fs_path, std::string new_content);

    CompletionInfo run_completion(SourceFile *file, TextPosition completion_point, sir::Module &out_sir_mod);

    SourceFile *find_file(const std::filesystem::path &fs_path);
    SourceFile *find_file(const ModulePath &mod_path);

    Index &get_index() { return index; }
    ModuleIndex *find_index(sir::Module *mod);
    const SymbolRef &get_index_symbol(const SymbolKey &key);

    const ModuleList &get_mod_list() { return module_manager.get_module_list(); }

private:
    void build_index(sema::ExtraAnalysis &analysis, const std::vector<sir::Module *> &mods);
    void collect_dependents(sir::Module &mod, std::unordered_set<ModulePath> &dependents);
};

} // namespace banjo::lsp

#endif
