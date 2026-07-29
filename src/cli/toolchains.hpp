#ifndef BANJO_CLI_TOOLCHAINS_H
#define BANJO_CLI_TOOLCHAINS_H

#include "banjo/utils/json.hpp"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace banjo::cli {

typedef std::vector<std::pair<std::string_view, std::string_view>> ToolchainProperties;

struct MSVCToolchain {
    static const ToolchainProperties PROPERTIES;

    std::string tools_path;
    std::string lib_path;

    static MSVCToolchain detect();
    JSONObject serialize();

private:
    void find_msvc();
    std::optional<std::filesystem::path> find_vswhere();
    std::optional<std::filesystem::path> find_vs_installation(const std::filesystem::path &vswhere_path);
    JSONArray run_vswhere(const std::filesystem::path &vswhere_path, const std::vector<std::string> &args);
    std::optional<std::string> find_latest_msvc_version(const std::filesystem::path &versions_path);

    void find_winsdk();
    std::optional<std::filesystem::path> find_winsdk_root();
    std::optional<std::string> find_latest_winsdk_version(const std::filesystem::path &versions_path);

    std::optional<std::string> get_max_version(const std::vector<std::string> &versions, unsigned num_components);
};

struct MinGWToolchain {
    static const ToolchainProperties PROPERTIES;

    std::string linker_path;
    std::vector<std::string> lib_dirs;

    static MinGWToolchain detect();
    JSONObject serialize();

private:
    void find_linker();
    void find_lib_dirs();
    std::filesystem::path find_c_compiler();
};

struct UnixToolchain {
    static const ToolchainProperties PROPERTIES;

    std::string linker_path;
    std::vector<std::string> linker_args;
    std::vector<std::string> extra_libs;
    std::vector<std::string> lib_dirs;
    std::string crt_dir;

    static UnixToolchain detect();
    static UnixToolchain install(std::string arch);
    JSONObject serialize();

private:
    void find_linker();
    void find_lib_dirs();
    void find_crt_dir();
    std::filesystem::path find_c_compiler();
};

struct MacOSToolchain {
    static const ToolchainProperties PROPERTIES;

    std::string linker_path;
    std::vector<std::string> linker_args;
    std::string sysroot_path;

    static MacOSToolchain detect();
    static MacOSToolchain install();
    JSONObject serialize();
};

struct WasmToolchain {
    static const ToolchainProperties PROPERTIES;

    std::string linker_path;

    static WasmToolchain detect();
    JSONObject serialize();
};

struct EmscriptenToolchain {
    static const ToolchainProperties PROPERTIES;

    std::string linker_path;

    static EmscriptenToolchain detect();
    JSONObject serialize();
};

std::vector<std::string> parse_gcc_lib_dirs(std::string_view search_dirs_output);

} // namespace banjo::cli

#endif
