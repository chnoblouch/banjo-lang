#ifndef BANJO_SOURCE_MODULE_DISCOVERY_H
#define BANJO_SOURCE_MODULE_DISCOVERY_H

#include "banjo/source/module_path.hpp"

#include <filesystem>
#include <optional>
#include <utility>
#include <vector>

namespace banjo {

namespace lang {

struct ModuleFile {
    ModulePath path;
    std::filesystem::path file_path;
};

struct ModuleTreeNode {
    ModuleFile file;
    std::vector<ModuleTreeNode> children;
};

class ModuleDiscovery {

private:
    std::vector<std::filesystem::path> search_paths;

public:
    void add_search_path(std::filesystem::path path) { search_paths.push_back(std::move(path)); }

    std::vector<ModuleTreeNode> find_all_modules();
    std::optional<ModuleFile> find_module(const ModulePath &path);
    std::vector<ModulePath> find_sub_modules(const ModuleFile &module_file);

private:
    void find_modules(
        const std::filesystem::path &directory,
        const ModulePath &prefix,
        std::vector<ModuleTreeNode> &modules
    );

    ModuleTreeNode node_from_file(const std::filesystem::path &file_path, const ModulePath &prefix);
    ModuleTreeNode node_from_dir(const std::filesystem::path &dir_path, const ModulePath &prefix);
};

} // namespace lang

} // namespace banjo

#endif
