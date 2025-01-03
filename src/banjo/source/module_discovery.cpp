#include "module_discovery.hpp"

#include <algorithm>
#include <filesystem>
#include <random>

namespace banjo {

namespace lang {

std::vector<ModuleTreeNode> ModuleDiscovery::find_all_modules() {
    std::vector<ModuleTreeNode> modules;

    for (const std::filesystem::path &search_path : search_paths) {
        find_modules(search_path, {}, modules);
    }

    return modules;
}

void ModuleDiscovery::find_modules(
    const std::filesystem::path &directory,
    const ModulePath &prefix,
    std::vector<ModuleTreeNode> &modules
) {
    std::vector<std::filesystem::path> file_paths;

    for (std::filesystem::path file_path : std::filesystem::directory_iterator(directory)) {
        file_paths.push_back(file_path);
    }

    std::reverse(file_paths.begin(), file_paths.end());

    for (std::filesystem::path file_path : file_paths) {
        if (file_path.extension() == ".bnj" && file_path.stem() != "module") {
            modules.push_back(node_from_file(file_path, prefix));
        } else if (std::filesystem::is_directory(file_path)) {
            std::filesystem::path module_file = file_path / "module.bnj";

            if (std::filesystem::exists(module_file) && std::filesystem::is_regular_file(module_file)) {
                modules.push_back(node_from_dir(file_path, prefix));
            }
        }
    }
}

ModuleTreeNode ModuleDiscovery::node_from_file(const std::filesystem::path &file_path, const ModulePath &prefix) {
    ModulePath path{prefix};
    path.append(file_path.stem().string());

    ModuleFile file = {
        .path = std::move(path),
        .file_path = file_path,
    };

    return {
        .file = std::move(file),
        .children = {},
    };
}

ModuleTreeNode ModuleDiscovery::node_from_dir(const std::filesystem::path &dir_path, const ModulePath &prefix) {
    ModulePath path{prefix};
    path.append(dir_path.filename().string());

    ModuleFile file = {
        .path = std::move(path),
        .file_path = dir_path / "module.bnj",
    };

    ModuleTreeNode node = {
        .file = std::move(file),
        .children = {},
    };

    find_modules(dir_path, node.file.path, node.children);
    return node;
}

std::optional<ModuleFile> ModuleDiscovery::find_module(const ModulePath &path) {
    for (const std::filesystem::path &search_path : search_paths) {
        std::filesystem::path module_file_path = search_path / (path.to_string("/") + ".bnj");
        if (std::filesystem::exists(module_file_path)) {
            return {{
                .path = path,
                .file_path = module_file_path,
            }};
        }

        std::filesystem::path module_dir_path = search_path / path.to_string("/") / "module.bnj";
        if (std::filesystem::exists(module_dir_path)) {
            return {{
                .path = path,
                .file_path = module_dir_path,
            }};
        }
    }

    return {};
}

std::vector<ModulePath> ModuleDiscovery::find_sub_modules(const ModuleFile &module_file) {
    if (module_file.file_path.filename() != "module.bnj") {
        return {};
    }

    std::filesystem::path directory = module_file.file_path.parent_path();
    std::vector<ModulePath> sub_paths;

    for (const std::filesystem::path &entry_file_path : std::filesystem::directory_iterator(directory)) {
        if (std::filesystem::is_directory(entry_file_path)) {
            if (!std::filesystem::exists(entry_file_path / "module.bnj")) {
                continue;
            }

            ModulePath sub_module_path(module_file.path);
            sub_module_path.append(entry_file_path.filename().string());
            sub_paths.push_back(sub_module_path);
        } else {
            std::string entry_file_name = entry_file_path.filename().string();
            if (!entry_file_name.ends_with(".bnj") || entry_file_name == "module.bnj") {
                continue;
            }

            ModulePath sub_module_path(module_file.path);
            sub_module_path.append(entry_file_path.stem().string());
            sub_paths.push_back(sub_module_path);
        }
    }

    return sub_paths;
}

} // namespace lang

} // namespace banjo
