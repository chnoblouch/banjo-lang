#include "module_manager.hpp"

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/ast_utils.hpp"
#include "banjo/ast/std_config_module.hpp"
#include "banjo/lexer/lexer.hpp"
#include "banjo/reports/report_manager.hpp"
#include "banjo/source/module_discovery.hpp"
#include "banjo/source/module_loader.hpp"
#include "banjo/symbol/module_path.hpp"
#include "banjo/utils/paths.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace banjo {

namespace lang {

ModuleManager::ModuleManager(ModuleLoader &module_loader, ReportManager &report_manager)
  : module_loader(module_loader),
    report_manager(report_manager) {}

void ModuleManager::add_search_path(std::filesystem::path path) {
    module_discovery.add_search_path(std::move(path));
}

void ModuleManager::add_standard_stdlib_search_path() {
    std::filesystem::path stdlib_path = Paths::executable().parent_path().parent_path() / "lib" / "stdlib2";
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

ASTModule *ModuleManager::load(const ModuleTreeNode &module_tree_node) {
    ASTModule *mod = load(module_tree_node.file);
    if (!mod) {
        return nullptr;
    }

    for (const ModuleTreeNode &child : module_tree_node.children) {
        ASTModule *sub_module = load(child);
        link_sub_module(mod, sub_module);
    }

    return mod;
}

ASTModule *ModuleManager::load(const ModuleFile &module_file) {
    ParsedAST parsed_ast = parse_module(module_file);
    ASTModule *mod = parsed_ast.module_;
    if (!mod) {
        return nullptr;
    }

    mod->set_file_path(module_file.file_path);

    module_list.add(mod);
    report_manager.merge_result(std::move(parsed_ast.reports), parsed_ast.is_valid);
    module_loader.after_load(module_file, mod, parsed_ast.is_valid);

    return mod;
}

ASTModule *ModuleManager::reload(ASTModule *mod) {
    std::optional<ModuleFile> module_file = module_discovery.find_module(mod->get_path());
    if (!module_file) {
        return nullptr;
    }

    ParsedAST parsed_ast = parse_module(*module_file);
    ASTModule *new_mod = parsed_ast.module_;
    if (!new_mod) {
        return nullptr;
    }

    new_mod->set_file_path(module_file->file_path);

    module_list.replace(mod, new_mod);
    report_manager.merge_result(std::move(parsed_ast.reports), parsed_ast.is_valid);
    module_loader.after_load(*module_file, new_mod, parsed_ast.is_valid);

    return new_mod;
}

ASTModule *ModuleManager::load_for_completion(const ModulePath &path, TextPosition completion_point) {
    std::optional<ModuleFile> module_file = module_discovery.find_module(path);
    if (!module_file) {
        return nullptr;
    }

    std::istream *stream = module_loader.open(*module_file);
    if (!stream) {
        return nullptr;
    }

    Lexer lexer(*stream);
    lexer.enable_completion(completion_point);
    std::vector<Token> tokens = lexer.tokenize();
    delete stream;

    Parser parser(tokens, module_file->path);
    parser.enable_completion();
    return parser.parse_module().module_;
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
        if (mod->get_path().get_size() == 1) {
            paths.push_back(mod->get_path());
        }
    }

    return paths;
}

void ModuleManager::link_sub_modules(ASTModule *mod, const ModuleFile &module_file) {
    for (const ModulePath &sub_path : module_discovery.find_sub_modules(module_file)) {
        ASTModule *sub_module = module_list.get_by_path(sub_path);
        link_sub_module(mod, sub_module);
    }
}

void ModuleManager::link_sub_module(ASTModule *mod, ASTModule *sub_mod) {
    SymbolTable *symbol_table = ASTUtils::get_module_symbol_table(mod);

    std::string name = sub_mod->get_path().get_path().back();
    SymbolRef symbol = SymbolRef(sub_mod).as_sub_module();
    symbol_table->add_symbol(name, symbol);

    mod->add_sub_mod(sub_mod);
}

ParsedAST ModuleManager::parse_module(const ModuleFile &module_file) {
    if (module_file.path == ModulePath{"std", "config"}) {
        return ParsedAST{
            .module_ = new StdConfigModule(),
            .is_valid = true,
            .reports = {},
        };
    }

    std::istream *stream = module_loader.open(module_file);
    if (!stream) {
        return ParsedAST{
            .module_ = nullptr,
            .is_valid = false,
            .reports = {},
        };
    }

    std::vector<Token> tokens = Lexer(*stream).tokenize();
    delete stream;

    return Parser(tokens, module_file.path).parse_module();
}

} // namespace lang

} // namespace banjo
