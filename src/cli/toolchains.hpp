#ifndef BANJO_CLI_TOOLCHAINS_H
#define BANJO_CLI_TOOLCHAINS_H

#include "banjo/utils/json.hpp"
#include "target.hpp"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace banjo::cli {

typedef std::vector<std::pair<std::string_view, std::string_view>> ToolchainProperties;

class Toolchain {

public:
    virtual ~Toolchain() = default;
    virtual const ToolchainProperties &properties() = 0;
    virtual json::Object serialize() = 0;
    virtual void remove(const Target &target) {}
};

class MSVCToolchain final : public Toolchain {

private:
    static const ToolchainProperties PROPERTIES;

public:
    std::string tools_path;
    std::string lib_path;

    static MSVCToolchain detect();
    static MSVCToolchain deserialize(json::Object &object);

    const ToolchainProperties &properties() override { return PROPERTIES; }
    json::Object serialize() override;

private:
    void find_msvc();
    std::optional<std::filesystem::path> find_vswhere();
    std::optional<std::filesystem::path> find_vs_installation(const std::filesystem::path &vswhere_path);
    json::Array run_vswhere(const std::filesystem::path &vswhere_path, const std::vector<std::string> &args);
    std::optional<std::string> find_latest_msvc_version(const std::filesystem::path &versions_path);

    void find_winsdk();
    std::optional<std::filesystem::path> find_winsdk_root();
    std::optional<std::string> find_latest_winsdk_version(const std::filesystem::path &versions_path);

    std::optional<std::string> get_max_version(const std::vector<std::string> &versions, unsigned num_components);
};

class MinGWToolchain final : public Toolchain {

private:
    static const ToolchainProperties PROPERTIES;

public:
    std::string linker_path;
    std::vector<std::string> lib_dirs;

    static MinGWToolchain detect();
    static MinGWToolchain deserialize(json::Object &object);

    const ToolchainProperties &properties() override { return PROPERTIES; }
    json::Object serialize() override;

private:
    void find_linker();
    void find_lib_dirs();
    std::filesystem::path find_c_compiler();
};

class UnixToolchain final : public Toolchain {

private:
    static const ToolchainProperties PROPERTIES;

public:
    std::string linker_path;
    std::vector<std::string> linker_args;
    std::vector<std::string> extra_libs;
    std::vector<std::string> lib_dirs;
    std::string crt_dir;

    static UnixToolchain detect();
    static UnixToolchain install(std::string arch);
    static UnixToolchain deserialize(json::Object &object);

    const ToolchainProperties &properties() override { return PROPERTIES; }
    json::Object serialize() override;
    void remove(const Target &target) override;

private:
    void find_linker();
    void find_lib_dirs();
    void find_crt_dir();
    std::filesystem::path find_c_compiler();

    static std::filesystem::path cross_sysroot_path(const std::string &arch);
};

class MacOSToolchain final : public Toolchain {

private:
    static const ToolchainProperties PROPERTIES;

public:
    std::string linker_path;
    std::vector<std::string> linker_args;
    std::string sysroot_path;

    static MacOSToolchain detect();
    static MacOSToolchain install();
    static MacOSToolchain deserialize(json::Object &object);

    const ToolchainProperties &properties() override { return PROPERTIES; }
    json::Object serialize() override;
    void remove(const Target &target) override;

private:
    static std::filesystem::path cross_sysroot_path();
};

class WasmToolchain final : public Toolchain {

private:
    static const ToolchainProperties PROPERTIES;

public:
    std::string linker_path;

    static WasmToolchain detect();
    static WasmToolchain deserialize(json::Object &object);

    const ToolchainProperties &properties() override { return PROPERTIES; }
    json::Object serialize() override;
};

class EmscriptenToolchain final : public Toolchain {

private:
    static const ToolchainProperties PROPERTIES;

public:
    std::string linker_path;

    static EmscriptenToolchain detect();
    static EmscriptenToolchain deserialize(json::Object &object);

    const ToolchainProperties &properties() override { return PROPERTIES; }
    json::Object serialize() override;
};

std::vector<std::string> parse_gcc_lib_dirs(std::string_view search_dirs_output);

} // namespace banjo::cli

#endif
