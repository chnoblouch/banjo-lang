#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "ast/ast_module.hpp"
#include "ast/module_list.hpp"
#include "parser/parser.hpp"
#include "source/module_discovery.hpp"
#include "symbol/module_path.hpp"

#include <filesystem>
#include <vector>

namespace lang {

class ModuleLoader;
class ReportManager;

class ModuleManager {

private:
    ModuleLoader &module_loader;
    ReportManager &report_manager;

    ModuleList module_list;
    ModuleDiscovery module_discovery;

public:
    ModuleManager(ModuleLoader &module_loader, ReportManager &report_manager);

    ModuleList &get_module_list() { return module_list; }
    ReportManager &get_report_manager() { return report_manager; }

    void load_all();
    ASTModule *load(const ModuleTreeNode &module_tree_node);
    ASTModule *load(const ModuleFile &module_file);
    void reload(ASTModule *mod);
    void clear();

    std::optional<std::filesystem::path> find_source_file(const ModulePath &path);
    std::vector<ModulePath> enumerate_root_paths();
    void link_sub_modules(ASTModule *mod, const ModuleFile &module_file);
    void link_sub_module(ASTModule *mod, ASTModule *sub_mod);

private:
    ParsedAST parse_module(const ModuleFile &module_file);
};

} // namespace lang

#endif
