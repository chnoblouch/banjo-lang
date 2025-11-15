#ifndef BANJO_AST_MODULE_LIST_H
#define BANJO_AST_MODULE_LIST_H

#include "banjo/ast/ast_module.hpp"
#include "banjo/source/source_file.hpp"

#include <filesystem>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>

namespace banjo {

namespace lang {

class ModuleList {

private:
    std::list<std::unique_ptr<SourceFile>> mods;
    std::unordered_map<std::string, SourceFile *> mods_by_fs_path;
    std::unordered_map<ModulePath, SourceFile *> mods_by_mod_path;

public:
    ModuleList() {}
    ModuleList(const ModuleList &) = delete;
    ModuleList(ModuleList &&other) noexcept = default;

    ModuleList &operator=(const ModuleList &) = delete;
    ModuleList &operator=(ModuleList &&other) = default;

    SourceFile *add(std::unique_ptr<SourceFile> mod) {
        mods.push_back(std::move(mod));

        SourceFile *mod_ref = mods.back().get();
        mods_by_fs_path[std::filesystem::absolute(mod_ref->fs_path).string()] = mod_ref;
        mods_by_mod_path[mod_ref->mod_path] = mod_ref;
        return mod_ref;
    }

    void remove(SourceFile *mod) {
        mods.remove_if([=](const std::unique_ptr<SourceFile> &candidate) { return candidate.get() == mod; });
        mods_by_fs_path.erase(mod->fs_path.string());
        mods_by_mod_path.erase(mod->mod_path);
    }

    SourceFile *find(const std::filesystem::path &fs_path) {
        std::string key = std::filesystem::absolute(fs_path).string();
        auto iter = mods_by_fs_path.find(key);
        return iter == mods_by_fs_path.end() ? nullptr : iter->second;
    }

    SourceFile *find(const ModulePath &mod_path) {
        auto iter = mods_by_mod_path.find(mod_path);
        return iter == mods_by_mod_path.end() ? nullptr : iter->second;
    }

    unsigned get_size() const { return mods.size(); }

    std::list<std::unique_ptr<SourceFile>>::iterator begin() { return mods.begin(); }
    std::list<std::unique_ptr<SourceFile>>::iterator end() { return mods.end(); }
    std::list<std::unique_ptr<SourceFile>>::const_iterator begin() const { return mods.begin(); }
    std::list<std::unique_ptr<SourceFile>>::const_iterator end() const { return mods.end(); }
};

} // namespace lang

} // namespace banjo

#endif
