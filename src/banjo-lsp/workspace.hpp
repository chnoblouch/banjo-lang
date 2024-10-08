#ifndef BANJO_LSP_WORKSPACE_H
#define BANJO_LSP_WORKSPACE_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/sema2/extra_analysis.hpp"
#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/source/module_manager.hpp"
#include "banjo/symbol/module_path.hpp"
#include "index.hpp"
#include "memory_module_loader.hpp"

#include <filesystem>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

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

    void initialize();
    void update(const std::filesystem::path &fs_path, std::string new_content);
    CompletionInfo run_completion(const File *file, lang::TextPosition completion_point);

    File *find_file(const std::filesystem::path &fs_path);
    File *find_file(const lang::ModulePath &mod_path);
    File *find_or_load_file(const std::filesystem::path &fs_path);
    ModuleIndex *find_index(lang::sir::Module *mod);

private:
    void build_index(lang::sema::ExtraAnalysis &analysis);
};

} // namespace lsp

} // namespace banjo

#endif
