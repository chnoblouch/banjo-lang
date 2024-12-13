#ifndef CONFIG_H
#define CONFIG_H

#include "banjo/target/target.hpp"
#include "banjo/target/target_description.hpp"

#include <filesystem>
#include <optional>
#include <vector>

namespace banjo {

namespace lang {

struct Config {
    target::TargetDescription target;
    bool optional_semicolons = false;
    int opt_level = 0;
    bool color_diagnostics = false;
    bool pic = false;
    bool hot_reload = false;
    bool testing = false;
    bool force_asm = false;
    bool disable_std = false;
    bool debug = false;
    std::vector<std::filesystem::path> paths;
    std::optional<target::CodeModel> code_model;

    static Config &instance();
};

} // namespace lang

} // namespace banjo

#endif
