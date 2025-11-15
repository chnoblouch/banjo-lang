#include "module_manager.hpp"

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/std_config_module.hpp"
#include "banjo/lexer/lexer.hpp"
#include "banjo/parser/parser.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/source/module_discovery.hpp"
#include "banjo/source/module_path.hpp"
#include "banjo/source/source_file.hpp"
#include "banjo/utils/paths.hpp"

#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace banjo {

namespace lang {

ModuleManager::ModuleManager(ReportManager &report_manager) : report_manager(report_manager) {}

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
        load_tree(root_node);
    }
}

SourceFile *ModuleManager::load_tree(const ModuleTreeNode &module_tree_node) {
    SourceFile *parsed_file = load(module_tree_node.file);
    if (!parsed_file) {
        return nullptr;
    }

    for (const ModuleTreeNode &child : module_tree_node.children) {
        SourceFile *sub_mod = load_tree(child);
        parsed_file->sub_mod_paths.push_back(sub_mod->mod_path);
    }

    return parsed_file;
}

SourceFile *ModuleManager::load(const ModuleFile &location) {
    std::unique_ptr<SourceFile> parsed_file = parse_module(location);
    if (!parsed_file || !parsed_file->ast_mod) {
        return nullptr;
    }

    SourceFile *file = module_list.add(std::move(parsed_file));
    report_manager.merge_result(std::move(file->ast_mod->reports), file->ast_mod->is_valid);
    return file;
}

void ModuleManager::reparse(SourceFile *file) {
    file->tokens = Lexer{*file}.tokenize();
    file->ast_mod = Parser{*file, file->tokens}.parse_module();

    report_manager.merge_result(std::move(file->ast_mod->reports), file->ast_mod->is_valid);
}

std::unique_ptr<ASTModule> ModuleManager::parse_for_completion(SourceFile *file, TextPosition completion_point) {
    Lexer lexer{*file};
    lexer.enable_completion(completion_point);
    std::vector<Token> tokens = lexer.tokenize();

    Parser parser{*file, tokens};
    parser.enable_completion();
    return parser.parse_module();
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

    for (const std::unique_ptr<SourceFile> &file : module_list) {
        if (file->mod_path.get_size() == 1) {
            paths.push_back(file->mod_path);
        }
    }

    return paths;
}

std::unique_ptr<SourceFile> ModuleManager::parse_module(const ModuleFile &module_file) {
    if (module_file.path == ModulePath{"std", "config"}) {
        std::unique_ptr<SourceFile> file = std::make_unique<SourceFile>(SourceFile{
            .mod_path = module_file.path,
            .fs_path = std::filesystem::absolute(module_file.file_path),
            .buffer{},
            .tokens{},
            .ast_mod = nullptr,
            .sir_mod = nullptr,
        });

        file->ast_mod = std::make_unique<StdConfigModule>(*file);
        return file;
    }

    std::ifstream stream{module_file.file_path};
    if (!stream) {
        return nullptr;
    }

    std::unique_ptr<SourceFile> file = SourceFile::read(module_file.path, module_file.file_path, stream);
    file->tokens = Lexer{*file}.tokenize();
    file->ast_mod = Parser{*file, file->tokens}.parse_module();
    return file;
}

} // namespace lang

} // namespace banjo
