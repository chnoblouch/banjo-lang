#ifndef BANJO_CLI_H
#define BANJO_CLI_H

#include "banjo/utils/json.hpp"

#include "argument_parser.hpp"
#include "manifest.hpp"
#include "process.hpp"
#include "target.hpp"

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace banjo {
namespace cli {

class CLI {

private:
    struct Toolchain {
        JSONObject properties;
    };

    enum class BuildConfig {
        DEBUG,
        RELEASE,
    };

    enum class PackageType {
        EXECUTABLE,
        STATIC_LIBRARY,
        SHARED_LIBRARY,
    };

    enum class ToolErrorMessageSource {
        STDOUT,
        STDERR,
    };

    ArgumentParser arg_parser;

    Target target;
    Toolchain toolchain;
    Manifest manifest;

    std::optional<Target> target_override;
    BuildConfig build_config = BuildConfig::DEBUG;
    std::optional<unsigned> opt_level = {};
    bool force_assembler = false;
    bool hot_reloading_enabled = false;
    std::vector<std::string> extra_compiler_args;

    PackageType package_type;
    std::vector<std::string> packages;
    std::vector<std::string> source_paths;
    std::vector<std::string> libraries;
    std::vector<std::string> library_paths;
    std::vector<std::string> macos_frameworks;

    std::chrono::steady_clock::time_point start_time;

public:
    void run(int argc, const char *argv[]);

private:
    void execute_targets();
    void execute_toolchains();
    void execute_version();
    void execute_new(const ArgumentParser::Result &args);
    void execute_build();
    void execute_run();
    void execute_test();
    void execute_invoke(const ArgumentParser::Result &args);
    void execute_lsp();
    void execute_bindgen(const ArgumentParser::Result &args);
    void execute_help();

    void load_config();
    void load_toolchain();
    void load_root_manifest(const Manifest &manifest);
    void load_manifest(const Manifest &manifest);
    void load_package(std::string_view name);

    void set_up_toolchain();

    Manifest parse_manifest(const std::filesystem::path &path);
    std::optional<Manifest> try_parse_manifest(const std::filesystem::path &path);
    Manifest parse_manifest(const JSONObject &json);
    std::string unwrap_json_string(const std::string &name, const JSONValue &value);
    std::vector<std::string> unwrap_json_string_array(const std::string &name, const JSONValue &value);
    Target parse_target(std::string_view string);

    void install_package(std::string_view package);
    void run_build_script(const std::filesystem::path &path);

    void build();
    ProcessResult invoke_compiler();
    void invoke_assembler();
    void invoke_nasm_assembler();
    void invoke_aarch64_assembler();
    void invoke_linker();
    void invoke_msvc_linker();
    void invoke_mingw_linker();
    void invoke_unix_linker();
    void invoke_darwin_linker();
    void invoke_wasm_linker();
    void invoke_emscripten_linker();
    void run_build();
    void run_hot_reloader();

    void append_compilation_args(std::vector<std::string> &args);

    void process_tool_result(
        const std::string &tool_name,
        const ProcessResult &result,
        ToolErrorMessageSource error_message_source = ToolErrorMessageSource::STDERR
    );

    std::filesystem::path get_toolchain_path();
    std::string get_output_path();
    std::filesystem::path get_output_dir();
};

} // namespace cli
} // namespace banjo

#endif
