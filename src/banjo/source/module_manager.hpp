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
#include <istream>
#include <memory>
#include <vector>

namespace banjo {

namespace lang {

class ModuleLoader;
class ReportManager;

class ModuleManager {

private:
    ReportManager &report_manager;

    ModuleList module_list;
    ModuleDiscovery module_discovery;

public:
    ModuleManager(ReportManager &report_manager);

    ModuleList &get_module_list() { return module_list; }

    void add_search_path(std::filesystem::path path);
    void add_standard_stdlib_search_path();
    void add_config_search_paths(const Config &config);

    void load_all();
    SourceFile *load_tree(const ModuleTreeNode &module_tree_node);
    SourceFile *load(const ModuleFile &location);
    void reparse(SourceFile *file);
    std::unique_ptr<ASTModule> parse_for_completion(SourceFile *file, TextPosition completion_point);

    std::optional<std::filesystem::path> find_source_file(const ModulePath &path);
    std::vector<ModulePath> enumerate_root_paths();

private:
    std::unique_ptr<SourceFile> parse_module(const ModuleFile &module_file);
};

} // namespace lang

} // namespace banjo

#endif
