#include "module_manager.hpp"

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/std_config_module.hpp"
#include "banjo/lexer/lexer.hpp"
#include "banjo/parser/parser.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/source/module_discovery.hpp"
#include "banjo/source/module_path.hpp"
#include "banjo/source/source_file.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/paths.hpp"

#include <fstream>
#include <optional>
#include <utility>
#include <vector>

namespace banjo {

namespace lang {

ModuleManager::ModuleManager(ReportManager &report_manager)
  : open_func(open_module_file),
    report_manager(report_manager) {}

ModuleManager::ModuleManager(OpenFunc open_func, ReportManager &report_manager)
  : open_func(std::move(open_func)),
    report_manager(report_manager) {}

void ModuleManager::add_search_path(std::filesystem::path path) {
    module_discovery.add_search_path(std::move(path));
}

void ModuleManager::add_standard_stdlib_search_path() {
    if (!Config::instance().is_stdlib_enabled()) {
        return;
    }

    std::filesystem::path stdlib_path = Paths::executable().parent_path().parent_path() / "lib" / "stdlib";
    add_search_path(stdlib_path);
}

void ModuleManager::add_config_search_paths(const Config &config) {
    for (std::filesystem::path path : config.paths) {
        add_search_path(std::move(path));
    }
}

void ModuleManager::load_all() {
    std::vector<ModuleTreeNode> root_nodes = module_discovery.find_all_modules();

    for (const ModuleTreeNode &root_node : root_nodes) {
        load(root_node);
    }
}

SourceFile *ModuleManager::load(const ModuleTreeNode &module_tree_node) {
    SourceFile *parsed_file = load(module_tree_node.file);
    if (!parsed_file) {
        return nullptr;
    }

    for (const ModuleTreeNode &child : module_tree_node.children) {
        SourceFile *sub_mod = load(child);
        parsed_file->sub_mod_paths.push_back(sub_mod->mod_path);
    }

    return parsed_file;
}

SourceFile *ModuleManager::load(const ModuleFile &module_file) {
    SourceFile *parsed_file = parse_module(module_file);
    ASTModule *mod = parsed_file->ast_mod;
    if (!mod) {
        return nullptr;
    }

    module_list.add(mod);
    report_manager.merge_result(std::move(mod->reports), mod->is_valid);

    return parsed_file;
}

SourceFile *ModuleManager::reload(ASTModule *mod) {
    std::optional<ModuleFile> module_file = module_discovery.find_module(mod->file.mod_path);
    if (!module_file) {
        return nullptr;
    }

    SourceFile *parsed_file = parse_module(*module_file);
    if (!parsed_file->ast_mod) {
        return nullptr;
    }

    module_list.replace(mod, parsed_file->ast_mod);
    report_manager.merge_result(std::move(parsed_file->ast_mod->reports), parsed_file->ast_mod->is_valid);

    return parsed_file;
}

SourceFile *ModuleManager::load_for_completion(const ModulePath &path, TextPosition completion_point) {
    std::optional<ModuleFile> module_file = module_discovery.find_module(path);
    if (!module_file) {
        return nullptr;
    }

    std::unique_ptr<std::istream> stream = open_func(*module_file);
    if (!stream) {
        return nullptr;
    }

    SourceFile *file = new SourceFile(SourceFile::read(module_file->path, module_file->file_path, *stream));

    Lexer lexer{*file};
    lexer.enable_completion(completion_point);
    file->tokens = lexer.tokenize();

    Parser parser{*file};
    parser.enable_completion();
    file->ast_mod = parser.parse_module();

    return file;
}

void ModuleManager::clear() {
    module_list.clear();
}

std::optional<std::filesystem::path> ModuleManager::find_source_file(const ModulePath &path) {
    std::optional<ModuleFile> module_file = module_discovery.find_module(path);
    if (!module_file) {
        return {};
    }

    return module_file->file_path;
}

std::vector<ModulePath> ModuleManager::enumerate_root_paths() {
    std::vector<ModulePath> paths;

    for (ASTModule *mod : module_list) {
        if (mod->file.mod_path.get_size() == 1) {
            paths.push_back(mod->file.mod_path);
        }
    }

    return paths;
}

std::unique_ptr<std::istream> ModuleManager::open_module_file(const ModuleFile &module_file) {
    return std::make_unique<std::ifstream>(module_file.file_path, std::ios::binary);
}

SourceFile *ModuleManager::parse_module(const ModuleFile &module_file) {
    if (module_file.path == ModulePath{"std", "config"}) {
        SourceFile *file = new SourceFile{
            .mod_path = module_file.path,
            .fs_path = module_file.file_path,
            .buffer{},
            .tokens{},
            .ast_mod = nullptr,
            .sir_mod = nullptr,
        };

        file->ast_mod = new StdConfigModule(*file);
        return file;
    }

    std::unique_ptr<std::istream> stream = open_func(module_file);
    if (!stream) {
        // FIXME
        ASSERT_UNREACHABLE;
    }

    SourceFile *file = new SourceFile(SourceFile::read(module_file.path, module_file.file_path, *stream));
    file->tokens = Lexer{*file}.tokenize();
    file->ast_mod = Parser{*file}.parse_module();
    return file;
}

} // namespace lang

} // namespace banjo
