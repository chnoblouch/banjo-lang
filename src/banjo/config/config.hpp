#ifndef CONFIG_H
#define CONFIG_H

#include "banjo/target/target.hpp"
#include "banjo/target/target_description.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace banjo {

namespace lang {

class Config {

private:
    int opt_level = 0;
    bool pic;
    bool hot_reload = false;
    bool force_asm = false;
    bool debug = false;

public:
    std::vector<std::filesystem::path> paths;
    target::TargetDescription target;
    bool disable_std = false;
    bool check_only = false;
    std::optional<target::CodeModel> code_model;
    bool testing = false;
    bool color_diagnostics = false;

public:
    int get_opt_level() const { return opt_level; }
    bool is_pic() const { return pic; }
    bool is_hot_reload() const { return hot_reload; }
    bool is_force_asm() const { return force_asm; }
    bool is_debug() const { return debug; }

    void set_opt_level(int opt_level) { this->opt_level = opt_level; }
    void set_pic(bool pic) { this->pic = pic; }
    void set_hot_reload(bool hot_reload) { this->hot_reload = hot_reload; }
    void set_force_asm(bool force_asm) { this->force_asm = force_asm; }
    void set_debug(bool debug) { this->debug = debug; }

    static Config &instance();
};

} // namespace lang

} // namespace banjo

#endif
