#ifndef BANJO_SOURCE_MODULE_MANAGER_H
#define BANJO_SOURCE_MODULE_MANAGER_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/module_list.hpp"
#include "banjo/config/config.hpp"
#include "banjo/parser/parser.hpp"
#include "banjo/source/module_discovery.hpp"
#include "banjo/source/module_path.hpp"
#include "banjo/source/text_range.hpp"

#include <filesystem>
#include <functional>
#include <istream>
#include <memory>
#include <vector>

namespace banjo {

namespace lang {

class ModuleLoader;
class ReportManager;

class ModuleManager {

public:
    typedef std::function<std::unique_ptr<std::istream>(const ModuleFile &module_file)> OpenFunc;

private:
    OpenFunc open_func;
    ReportManager &report_manager;

    ModuleList module_list;
    ModuleDiscovery module_discovery;

public:
    ModuleManager(ReportManager &report_manager);
    ModuleManager(OpenFunc open_func, ReportManager &report_manager);

    ModuleList &get_module_list() { return module_list; }
    ReportManager &get_report_manager() { return report_manager; }

    void add_search_path(std::filesystem::path path);
    void add_standard_stdlib_search_path();
    void add_config_search_paths(const Config &config);

    void load_all();
    ASTModule *load(const ModuleTreeNode &module_tree_node);
    ASTModule *load(const ModuleFile &module_file);
    ASTModule *reload(ASTModule *mod);
    ASTModule *load_for_completion(const ModulePath &path, TextPosition completion_point);
    void clear();

    std::optional<std::filesystem::path> find_source_file(const ModulePath &path);
    std::vector<ModulePath> enumerate_root_paths();
    void link_sub_modules(ASTModule *mod, const ModuleFile &module_file);
    void link_sub_module(ASTModule *mod, ASTModule *sub_mod);

private:
    static std::unique_ptr<std::istream> open_module_file(const ModuleFile &module_file);

    ParsedAST parse_module(const ModuleFile &module_file);
};

} // namespace lang

} // namespace banjo

#endif
