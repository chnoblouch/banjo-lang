#include "workspace.hpp"

#include "banjo/config/config.hpp"
#include "banjo/sema/semantic_analyzer.hpp"
#include "banjo/sir/sir.hpp"
#include "banjo/sir/sir_generator.hpp"
#include "banjo/source/module_manager.hpp"
#include "banjo/target/target.hpp"
#include "banjo/utils/macros.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <utility>

namespace banjo {

namespace lsp {

using namespace lang;

Workspace::Workspace()
  : module_manager(std::bind(&Workspace::open_module, this, std::placeholders::_1), report_manager),
    config(Config::instance()),
    target(target::Target::create(config.target, target::CodeModel::LARGE)) {}

std::vector<lang::sir::Module *> Workspace::initialize() {
    module_manager.add_standard_stdlib_search_path();
    module_manager.add_config_search_paths(config);
    module_manager.load_all();

    sir_unit = SIRGenerator().generate(module_manager.get_module_list());

    sema::SemanticAnalyzer analyzer(sir_unit, target.get(), report_manager, sema::Mode::INDEXING);
    analyzer.analyze();
    build_index(analyzer.get_extra_analysis(), sir_unit.mods);

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

    return sir_unit.mods;
}

std::vector<lang::sir::Module *> Workspace::update(const std::filesystem::path &fs_path, std::string new_content) {
    report_manager.reset();

    File *file = find_file(fs_path);
    file->content = std::move(new_content);

    ASTModule *new_mod = module_manager.reload(file->ast_module);
    file->ast_module = new_mod;

    std::unordered_set<lang::ModulePath> paths_to_analyze;
    paths_to_analyze.insert(file->sir_module->path);
    collect_dependents(*file->sir_module, paths_to_analyze);

    std::vector<lang::sir::Module *> mods_to_analyze;
    mods_to_analyze.reserve(paths_to_analyze.size());

    for (const lang::ModulePath &path : paths_to_analyze) {
        File *file = find_file(path);
        SIRGenerator().regenerate_mod(sir_unit, file->ast_module);
        mods_to_analyze.push_back(file->sir_module);
    }

    sema::SemanticAnalyzer analyzer(sir_unit, target.get(), report_manager, sema::Mode::INDEXING);
    analyzer.analyze(mods_to_analyze);
    build_index(analyzer.get_extra_analysis(), mods_to_analyze);

    return mods_to_analyze;
}

CompletionInfo Workspace::run_completion(
    const File *file,
    TextPosition completion_point,
    lang::sir::Module &out_sir_mod
) {
    ASTModule *mod = module_manager.load_for_completion(file->ast_module->get_path(), completion_point);
    ASSERT(mod);
    out_sir_mod = SIRGenerator().generate(mod);

    sema::SemanticAnalyzer analyzer(sir_unit, target.get(), report_manager, sema::Mode::COMPLETION);
    analyzer.analyze(out_sir_mod);

    std::vector<lang::sir::Symbol> preamble_symbols;
    preamble_symbols.reserve(analyzer.get_preamble_symbols().size());

    for (const auto &[name, symbol] : analyzer.get_preamble_symbols()) {
        preamble_symbols.push_back(symbol);
    }

    return CompletionInfo{
        .sir_mod = std::move(out_sir_mod),
        .context = analyzer.get_completion_context(),
        .infection = analyzer.get_completion_infection(),
        .preamble_symbols = preamble_symbols,
    };
}

void Workspace::undo_infection(lang::sema::CompletionInfection &infection) {
    // HACK: Completion is 'infectious' because it may create specializations in other modules. In
    // completion mode, these changes are recorded so they can be undone here. There has to be a
    // better way to solve this issue...

    for (auto [func_def, num_specializations] : infection.func_specializations) {
        func_def->specializations.resize(func_def->specializations.size() - num_specializations);
    }

    for (auto [struct_def, num_specializations] : infection.struct_specializations) {
        struct_def->specializations.resize(struct_def->specializations.size() - num_specializations);
    }
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

File *Workspace::find_file(const lang::sir::Module &mod) {
    return find_file(mod.path);
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

const SymbolRef &Workspace::get_index_symbol(const SymbolKey &key) {
    return index.get_symbol(key);
}

std::vector<lang::ModulePath> Workspace::list_sub_mods(lang::sir::Module *mod) {
    lang::ASTModule *ast_mod = module_manager.get_module_list().get_by_path(mod->path);
    if (!ast_mod) {
        return {};
    }

    std::vector<lang::ModulePath> paths(ast_mod->get_sub_mods().size());

    for (unsigned i = 0; i < ast_mod->get_sub_mods().size(); i++) {
        paths[i] = ast_mod->get_sub_mods()[i]->get_path();
    }

    return paths;
}

std::unique_ptr<std::istream> Workspace::open_module(const lang::ModuleFile &module_file) {
    File *file = find_or_load_file(module_file.file_path);
    if (!file) {
        return nullptr;
    }

    return std::make_unique<std::stringstream>(file->content);
}

void Workspace::build_index(sema::ExtraAnalysis &analysis, const std::vector<lang::sir::Module *> &mods) {
    for (lang::sir::Module *mod : mods) {
        index.mods.erase(mod);
    }

    for (const Report &report : report_manager.get_reports()) {
        ModuleIndex &mod = index.mods[sir_unit.mods_by_path[report.get_message().location->path]];
        mod.reports.push_back(report);
    }

    std::unordered_map<sir::Symbol, SymbolKey> symbol_defs;

    for (auto &[mod, mod_analysis] : analysis.mods) {
        ModuleIndex &mod_index = index.mods[mod];

        for (sema::ExtraAnalysis::SymbolDef &def : mod_analysis.symbol_defs) {

            SymbolRef ref{
                .range = def.ident_range,
                .symbol = def.symbol,
                .def_mod = mod,
                .def_range = def.ident_range,
            };

            SymbolKey key{
                .mod = mod,
                .index = static_cast<unsigned>(mod_index.symbol_refs.size()),
            };

            mod_index.symbol_refs.push_back(ref);
            symbol_defs.insert({def.symbol, key});
        }
    }

    for (auto &[mod, mod_analysis] : analysis.mods) {
        ModuleIndex &mod_index = index.mods[mod];

        for (sema::ExtraAnalysis::SymbolUse &use : mod_analysis.symbol_uses) {
            SymbolRef ref{
                .range = use.range,
                .symbol = use.symbol,
                .def_mod = nullptr,
                .def_range = {0, 0},
            };

            // SymbolKey key{
            //     .mod = mod,
            //     .index = static_cast<unsigned>(mod_index.symbol_refs.size()),
            // };

            if (auto ref_mod = use.symbol.match<sir::Module>()) {
                ref.def_mod = ref_mod;
                ref.def_range = ref_mod->block.ast_node->range;

                ModuleIndex &def_mod_index = index.mods[ref_mod];
                def_mod_index.dependents.insert(mod->path);
            } else {
                auto def_key = symbol_defs.find(use.symbol);

                if (def_key != symbol_defs.end()) {
                    ModuleIndex &def_mod_index = index.mods[def_key->second.mod];
                    SymbolRef &def = def_mod_index.symbol_refs[def_key->second.index];

                    ref.def_mod = def.def_mod;
                    ref.def_range = def.def_range;

                    // def.uses.push_back(key);
                    def_mod_index.dependents.insert(mod->path);
                }
            }

            mod_index.symbol_refs.push_back(ref);
        }
    }
}

void Workspace::collect_dependents(lang::sir::Module &mod, std::unordered_set<lang::ModulePath> &dependents) {
    ModuleIndex &mod_index = index.mods[&mod];

    for (const lang::ModulePath &path : mod_index.dependents) {
        if (dependents.contains(path)) {
            continue;
        }

        dependents.insert(path);
        collect_dependents(*sir_unit.mods_by_path[path], dependents);
    }
}

} // namespace lsp

} // namespace banjo
