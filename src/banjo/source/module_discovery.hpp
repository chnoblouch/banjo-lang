#ifndef MODULE_DISCOVERY_H
#define MODULE_DISCOVERY_H

#include "symbol/module_path.hpp"

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

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
    ModuleDiscovery();

    std::vector<ModuleTreeNode> find_all_modules();
    std::optional<ModuleFile> find_module(const ModulePath &path);
    std::vector<ModulePath> find_sub_modules(const ModuleFile &module_file);

private:
    void register_search_paths();

    void find_modules(
        const std::filesystem::path &directory,
        const ModulePath &prefix,
        std::vector<ModuleTreeNode> &modules
    );

    ModuleTreeNode node_from_file(const std::filesystem::path &file_path, const ModulePath &prefix);
    ModuleTreeNode node_from_dir(const std::filesystem::path &dir_path, const ModulePath &prefix);
};

} // namespace lang

#endif
