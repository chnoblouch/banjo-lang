#ifndef BANJO_CLI_H
#define BANJO_CLI_H

#include "banjo/utils/json.hpp"
#include "manifest.hpp"
#include "target.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace banjo {
namespace cli {

class CLI {

private:
    Manifest manifest;
    Target target;

    std::vector<std::string> libraries;
    std::vector<std::string> library_paths;
    std::vector<std::string> object_files;

public:
    void run(int argc, const char *argv[]);

private:
    void execute_targets();
    void execute_version();
    void execute_build();
    void execute_run();
    void execute_help();

    void load_config();
    Manifest load_manifest(const std::filesystem::path &path);
    void load_package(std::string_view name);

    Manifest parse_manifest(const std::filesystem::path &path);
    Manifest parse_manifest(const JSONObject &json);
    Target parse_target(std::string_view string);

    void build();
    void invoke_compiler();
    void invoke_linker();
    void invoke_windows_linker();
    void invoke_unix_linker();
    void run_build();

    std::string get_output_path();
    std::filesystem::path get_output_dir();
};

} // namespace cli
} // namespace banjo

#endif
