#ifndef BANJO_SOURCE_MODULE_MANAGER_H
#define BANJO_SOURCE_MODULE_MANAGER_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/ast/module_list.hpp"
#include "banjo/config/config.hpp"
#include "banjo/source/module_discovery.hpp"
#include "banjo/source/module_path.hpp"
#include "banjo/source/source_file.hpp"
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

    void add_search_path(std::filesystem::path path);
    void add_standard_stdlib_search_path();
    void add_config_search_paths(const Config &config);

    void load_all();
    SourceFile *load(const ModuleTreeNode &module_tree_node);
    SourceFile *load(const ModuleFile &module_file);
    SourceFile *reload(ASTModule *mod);
    SourceFile *load_for_completion(const ModulePath &path, TextPosition completion_point);
    void clear();

    std::optional<std::filesystem::path> find_source_file(const ModulePath &path);
    std::vector<ModulePath> enumerate_root_paths();

private:
    static std::unique_ptr<std::istream> open_module_file(const ModuleFile &module_file);

    SourceFile *parse_module(const ModuleFile &module_file);
};

} // namespace lang

} // namespace banjo

#endif
