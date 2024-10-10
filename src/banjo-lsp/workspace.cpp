#include "workspace.hpp"

#include "banjo/config/config.hpp"
#include "banjo/sema2/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_generator.hpp"
#include "banjo/source/module_manager.hpp"
#include "banjo/target/target.hpp"
#include "banjo/utils/macros.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <utility>

namespace banjo {

namespace lsp {

using namespace lang;

Workspace::Workspace()
  : module_loader(*this),
    module_manager(module_loader, report_manager),
    config(Config::instance()),
    target(target::Target::create(config.target, target::CodeModel::LARGE)) {}

void Workspace::initialize() {
    module_manager.add_standard_stdlib_search_path();
    module_manager.add_config_search_paths(config);
    module_manager.load_all();

    sir_unit = SIRGenerator().generate(module_manager.get_module_list());

    sema::SemanticAnalyzer analyzer(sir_unit, target.get(), report_manager, sema::Mode::INDEXING);
    analyzer.analyze();
    build_index(analyzer.get_extra_analysis());

    for (sir::Module *mod : sir_unit.mods) {
        ASTModule *ast_module = module_manager.get_module_list().get_by_path(mod->path);
        File *file = find_file(ast_module->get_file_path());

        if (!file) {
            continue;
        }

        file->ast_module = ast_module;
        file->sir_module = mod;
        files_by_mod_path[mod->path] = file;
    }
}

void Workspace::update(const std::filesystem::path &fs_path, std::string new_content) {
    report_manager.reset();

    File *file = find_file(fs_path);
    file->content = std::move(new_content);

    ASTModule *new_mod = module_manager.reload(file->ast_module);
    file->ast_module = new_mod;

    SIRGenerator().regenerate_mod(sir_unit, new_mod);

    sema::SemanticAnalyzer analyzer(sir_unit, target.get(), report_manager, sema::Mode::INDEXING);
    analyzer.analyze(*file->sir_module);
    build_index(analyzer.get_extra_analysis());
}

CompletionInfo Workspace::run_completion(const File *file, TextPosition completion_point) {
    ASTModule *mod = module_manager.load_for_completion(file->ast_module->get_path(), completion_point);
    assert(mod);
    sir::Module sir_mod = SIRGenerator().generate(mod);

    sema::SemanticAnalyzer analyzer(sir_unit, target.get(), report_manager, sema::Mode::COMPLETION);
    analyzer.analyze(sir_mod);

    return CompletionInfo{
        .sir_mod = std::move(sir_mod),
        .context = analyzer.get_completion_context(),
    };
}

File *Workspace::find_file(const std::filesystem::path &fs_path) {
    std::string fs_path_absolute = std::filesystem::absolute(fs_path).string();
    auto iter = files_by_fs_path.find(fs_path_absolute);
    return iter == files_by_fs_path.end() ? nullptr : iter->second;
}

File *Workspace::find_file(const lang::ModulePath &mod_path) {
    auto iter = files_by_mod_path.find(mod_path);
    return iter == files_by_mod_path.end() ? nullptr : iter->second;
}

File *Workspace::find_or_load_file(const std::filesystem::path &fs_path) {
    if (File *file = find_file(fs_path)) {
        return file;
    }

    std::string fs_path_absolute = std::filesystem::absolute(fs_path).string();

    std::string content;
    std::ifstream stream(fs_path_absolute, std::ios::binary);
    stream.seekg(0, std::ios::end);
    std::istream::pos_type size = stream.tellg();
    content.resize(size);
    stream.seekg(0, std::ios::beg);
    stream.read(content.data(), size);

    files.push_back(File{
        .fs_path = fs_path_absolute,
        .ast_module = nullptr,
        .sir_module = nullptr,
        .content = std::move(content),
    });

    files_by_fs_path.insert({fs_path_absolute, &files.back()});
    return &files.back();
}

ModuleIndex *Workspace::find_index(lang::sir::Module *mod) {
    auto iter = index.mods.find(mod);
    return iter == index.mods.end() ? nullptr : &iter->second;
}

void Workspace::build_index(sema::ExtraAnalysis &analysis) {
    index.mods.clear();

    for (const Report &report : report_manager.get_reports()) {
        ModuleIndex &mod = index.mods[sir_unit.mods_by_path[report.get_message().location->path]];
        mod.reports.push_back(report);
    }

    std::unordered_map<sir::Symbol, SymbolRef> symbol_defs;

    for (auto &[mod, mod_analysis] : analysis.mods) {
        for (sema::ExtraAnalysis::SymbolDef &def : mod_analysis.symbol_defs) {
            SymbolRef ref{
                .range = def.ident_range,
                .symbol = def.symbol,
                .def_mod = mod,
                .def_range = def.ident_range,
            };

            index.mods[mod].symbol_refs.push_back(ref);
            symbol_defs.insert({def.symbol, ref});
        }
    }

    for (auto &[mod, mod_analysis] : analysis.mods) {
        for (sema::ExtraAnalysis::SymbolUse &use : mod_analysis.symbol_uses) {
            SymbolRef ref{
                .range = use.range,
                .symbol = use.symbol,
                .def_mod = nullptr,
                .def_range = {0, 0},
            };

            if (auto ref_mod = use.symbol.match<sir::Module>()) {
                ref.def_mod = ref_mod;
                ref.def_range = ref_mod->block.ast_node->get_range();
            } else {
                auto def = symbol_defs.find(use.symbol);
                if (def != symbol_defs.end()) {
                    ref.def_mod = def->second.def_mod;
                    ref.def_range = def->second.def_range;
                }
            }

            index.mods[mod].symbol_refs.push_back(ref);
        }
    }
}

} // namespace lsp

} // namespace banjo
