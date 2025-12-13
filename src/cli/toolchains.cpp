#include "toolchains.hpp"

#include "banjo/utils/json.hpp"
#include "banjo/utils/json_parser.hpp"
#include "banjo/utils/utils.hpp"

#include "common.hpp"
#include "paths.hpp"

#include <algorithm>
#include <filesystem>
#include <utility>

namespace banjo {
namespace cli {

MSVCToolchain MSVCToolchain::detect() {
    MSVCToolchain toolchain;

    print_step("Locating MSVC toolchain...");
    toolchain.find_msvc();
    toolchain.find_winsdk();

    return toolchain;
}

void MSVCToolchain::find_msvc() {
    std::optional<std::filesystem::path> vswhere_path = find_vswhere();
    if (!vswhere_path) {
        error("failed to find vswhere");
    }

    print_step("  Found vswhere: " + vswhere_path->string());

    std::optional<std::filesystem::path> vs_path = find_vs_installation(*vswhere_path);
    if (!vs_path) {
        error("failed to find visual studio installation");
    }

    print_step("  Found Visual Studio installation: " + vs_path->string());

    std::filesystem::path msvc_versions_path = *vs_path / "VC" / "Tools" / "MSVC";
    std::optional<std::string> msvc_version = find_latest_msvc_version(msvc_versions_path);

    if (!msvc_version) {
        error("failed to determine msvc version");
    }

    print_step("  Using MSVC version " + *msvc_version);
    tools_path = (msvc_versions_path / *msvc_version).string();
}

std::optional<std::filesystem::path> MSVCToolchain::find_vswhere() {
    std::filesystem::path program_files_path;

    if (std::optional<std::string_view> env = Utils::get_env("ProgramFiles(x86)")) {
        program_files_path = *env;
    } else if (std::optional<std::string_view> env = Utils::get_env("ProgramFiles")) {
        program_files_path = *env;
    } else {
        return {};
    }

    std::filesystem::path vswhere_path = program_files_path / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
    vswhere_path = std::filesystem::canonical(vswhere_path);

    if (std::filesystem::is_regular_file(vswhere_path)) {
        return vswhere_path;
    } else {
        return {};
    }
}

std::optional<std::filesystem::path> MSVCToolchain::find_vs_installation(const std::filesystem::path &vswhere_path) {
    JSONArray result;

    // Look for a full Visual Studio installation.
    result = run_vswhere(vswhere_path, {"-latest"});

    if (result.length() > 0) {
        std::string path_raw = result.get_object(0).get_string("installationPath");
        return std::filesystem::canonical(path_raw);
    }

    // Look for a Visual Studio Build Tools installation.
    result = run_vswhere(vswhere_path, {"-latest", "-products", "Microsoft.VisualStudio.Product.BuildTools"});

    if (result.length() > 0) {
        std::string path_raw = result.get_object(0).get_string("installationPath");
        return std::filesystem::canonical(path_raw);
    }

    return {};
}

JSONArray MSVCToolchain::run_vswhere(const std::filesystem::path &vswhere_path, const std::vector<std::string> &args) {
    std::vector<std::string> full_args{"-format", "json"};
    full_args.insert(full_args.end(), args.begin(), args.end());
    std::string output = get_tool_output(vswhere_path, full_args);
    return JSONParser(output).parse_array();
}

std::optional<std::string> MSVCToolchain::find_latest_msvc_version(const std::filesystem::path &versions_path) {
    std::vector<std::string> versions;

    for (std::filesystem::path version_dir : std::filesystem::directory_iterator(versions_path)) {
        versions.push_back(version_dir.filename().string());
    }

    return get_max_version(versions, 3);
}

void MSVCToolchain::find_winsdk() {
    std::optional<std::filesystem::path> winsdk_root_path = find_winsdk_root();
    if (!winsdk_root_path) {
        error("failed to find windows sdk");
    }

    print_step("  Found Windows SDK: " + winsdk_root_path->string());

    std::filesystem::path winsdk_versions_path = *winsdk_root_path / "Lib";
    std::optional<std::string> winsdk_version = find_latest_winsdk_version(winsdk_versions_path);

    if (!winsdk_version) {
        error("failed to determine windows sdk version");
    }

    print_step("  Using Windows SDK version " + *winsdk_version);
    lib_path = (winsdk_versions_path / *winsdk_version).string();
}

std::optional<std::filesystem::path> MSVCToolchain::find_winsdk_root() {
    for (const std::string &env_name : std::initializer_list<std::string>{"ProgramFiles(x86)", "ProgramFiles"}) {
        std::optional<std::string_view> env = Utils::get_env(env_name);

        if (!env) {
            continue;
        }

        std::filesystem::path winsdk_root_path = std::filesystem::path(*env) / "Windows Kits" / "10";

        if (std::filesystem::is_directory(winsdk_root_path)) {
            return winsdk_root_path;
        }
    }

    return {};
}

std::optional<std::string> MSVCToolchain::find_latest_winsdk_version(const std::filesystem::path &versions_path) {
    std::vector<std::string> versions;

    for (std::filesystem::path version_dir : std::filesystem::directory_iterator(versions_path)) {
        versions.push_back(version_dir.filename().string());
    }

    return get_max_version(versions, 4);
}

std::optional<std::string> MSVCToolchain::get_max_version(
    const std::vector<std::string> &versions,
    unsigned num_components
) {
    if (versions.empty()) {
        return {};
    }

    std::vector<std::pair<std::string_view, std::vector<std::uint64_t>>> parsed_versions;

    for (const std::string &version : versions) {
        std::vector<std::string_view> string_components = Utils::split_string(version, '.');

        if (string_components.size() != num_components) {
            continue;
        }

        std::vector<std::uint64_t> number_components(num_components);
        bool is_valid = true;

        for (unsigned i = 0; i < num_components; i++) {
            if (auto number = Utils::parse_u64(string_components[i])) {
                number_components.push_back(*number);
            } else {
                is_valid = false;
                break;
            }
        }

        if (is_valid) {
            parsed_versions.push_back({version, number_components});
        }
    }

    auto sort_comparison_func = [](const auto &lhs, const auto &rhs) {
        for (unsigned i = 0; i < lhs.second.size(); i++) {
            if (lhs.second[i] != rhs.second[i]) {
                return lhs.second[i] < rhs.second[i];
            }
        }

        return false;
    };

    std::sort(parsed_versions.begin(), parsed_versions.end(), sort_comparison_func);
    return std::string(parsed_versions.back().first);
}

JSONObject MSVCToolchain::serialize() {
    JSONObject object;
    object.add("tools", tools_path);
    object.add("lib", lib_path);
    return object;
}

MinGWToolchain MinGWToolchain::detect() {
    MinGWToolchain toolchain;

    print_step("Locating MinGW toolchain...");
    toolchain.find_linker();
    toolchain.find_lib_dirs();

    return toolchain;
}

void MinGWToolchain::find_linker() {
    std::optional<std::filesystem::path> ld_path = find_tool("x86_64-w64-mingw32-ld");
    if (ld_path) {
        print_step("  Found MinGW LD: " + ld_path->string());
        print_step("    Version: " + get_tool_output(*ld_path, {"-v"}));

        linker_path = ld_path->string();
        return;
    }

    error("failed to find mingw linker");
}

void MinGWToolchain::find_lib_dirs() {
    std::filesystem::path c_compiler_path = find_c_compiler();
    std::string search_dirs_output = get_tool_output(c_compiler_path, {"--print-search-dirs"});
    lib_dirs = parse_gcc_lib_dirs(search_dirs_output);

    print_step("  Library directories:");

    for (const std::string &search_dir : lib_dirs) {
        print_step("    - " + search_dir);
    }
}

std::filesystem::path MinGWToolchain::find_c_compiler() {
    std::optional<std::filesystem::path> gcc_path = find_tool("x86_64-w64-mingw32-gcc");
    if (gcc_path) {
        print_step("  Found MinGW GCC: " + gcc_path->string());
        return *gcc_path;
    }

    error("failed to find mingw c compiler");
}

JSONObject MinGWToolchain::serialize() {
    JSONObject object;
    object.add("linker_path", linker_path);
    object.add("lib_dirs", JSONArray{lib_dirs});
    return object;
}

UnixToolchain UnixToolchain::detect() {
    UnixToolchain toolchain;

    print_step("Locating Unix toolchain...");
    toolchain.find_linker();
    toolchain.find_lib_dirs();
    toolchain.find_crt_dir();

    return toolchain;
}

void UnixToolchain::find_linker() {
    std::optional<std::filesystem::path> lld_path = find_tool("lld");
    if (lld_path) {
        print_step("  Found LLD: " + lld_path->string());
        print_step("    Version: " + get_tool_output(*lld_path, {"-flavor", "gnu", "-v"}));

        linker_path = lld_path->string();
        linker_args = {"-flavor", "gnu"};
        return;
    }

    std::optional<std::filesystem::path> ld_path = find_tool("ld");
    if (ld_path) {
        print_step("  Found LD: " + ld_path->string());
        print_step("    Version: " + get_tool_output(*ld_path, {"-v"}));

        linker_path = ld_path->string();
        return;
    }

    error("failed to find system linker");
}

void UnixToolchain::find_lib_dirs() {
    std::filesystem::path c_compiler_path = find_c_compiler();
    std::string search_dirs_output = get_tool_output(c_compiler_path, {"--print-search-dirs"});
    lib_dirs = parse_gcc_lib_dirs(search_dirs_output);

    print_step("  Library directories:");

    for (const std::string &search_dir : lib_dirs) {
        print_step("    - " + search_dir);
    }
}

void UnixToolchain::find_crt_dir() {
    for (const std::string &search_dir : lib_dirs) {
        if (std::filesystem::is_regular_file(std::filesystem::path(search_dir) / "crt1.o")) {
            print_step("  CRT directory: " + search_dir);
            crt_dir = search_dir;
            return;
        }
    }

    error("failed to find system crt directory");
}

std::filesystem::path UnixToolchain::find_c_compiler() {
    std::optional<std::filesystem::path> clang_path = find_tool("clang");
    if (clang_path) {
        print_step("  Found Clang: " + clang_path->string());
        return *clang_path;
    }

    std::optional<std::filesystem::path> gcc_path = find_tool("gcc");
    if (gcc_path) {
        print_step("  Found GCC: " + gcc_path->string());
        return *gcc_path;
    }

    error("failed to find system c compiler");
}

UnixToolchain UnixToolchain::install(std::string arch) {
    UnixToolchain toolchain;
    toolchain.find_linker();

    std::filesystem::path toolchains_dir = paths::toolchains_dir();
    std::filesystem::path toolchain_dir = toolchains_dir / ("sysroot-" + arch + "-linux-gnu");

    if (std::filesystem::exists(toolchain_dir)) {
        print_step("  Using existing Linux sysroot");
    } else {
        print_step("  Installing Linux sysroot...");
        run_utility_script("install_sysroot_linux.py", {std::move(arch), toolchains_dir.string()});
    }

    toolchain.lib_dirs = {toolchain_dir.string()};
    toolchain.crt_dir = {toolchain_dir.string()};
    toolchain.extra_libs = {"c_nonshared", "gcc"};

    return toolchain;
}

JSONObject UnixToolchain::serialize() {
    JSONObject object;
    object.add("linker_path", linker_path);
    object.add("linker_args", JSONArray{linker_args});
    object.add("additional_libraries", JSONArray{extra_libs});
    object.add("lib_dirs", JSONArray{lib_dirs});
    object.add("crt_dir", crt_dir);
    return object;
}

MacOSToolchain MacOSToolchain::detect() {
    MacOSToolchain toolchain;

    print_step("Locating macOS toolchain...");

    std::optional<std::filesystem::path> xcodebuild_path = find_tool("xcodebuild");
    if (!xcodebuild_path) {
        error("failed to find xcodebuild");
    }

    print_step("  Found xcodebuild: " + xcodebuild_path->string());

    std::optional<std::filesystem::path> xcrun_path = find_tool("xcrun");
    if (!xcrun_path) {
        error("failed to find xcrun");
    }

    print_step("  Found xcrun: " + xcrun_path->string());

    std::string linker_path_raw = get_tool_output(xcodebuild_path->string(), {"-find", "ld"});
    std::filesystem::path linker_path = std::filesystem::canonical(linker_path_raw);
    print_step("  Found Xcode linker: " + linker_path.string());
    toolchain.linker_path = linker_path.string();

    std::string sysroot_path_raw = get_tool_output(xcrun_path->string(), {"-sdk", "macosx", "-show-sdk-path"});
    std::filesystem::path sysroot_path = std::filesystem::canonical(sysroot_path_raw);
    print_step("  Found macOS SDK: " + sysroot_path.string());
    toolchain.sysroot_path = sysroot_path.string();

    return toolchain;
}

JSONObject MacOSToolchain::serialize() {
    JSONObject object;
    object.add("linker_path", linker_path);
    object.add("extra_args", JSONArray{linker_args});
    object.add("sysroot", sysroot_path);
    return object;
}

WasmToolchain WasmToolchain::detect() {
    WasmToolchain toolchain;

    print_step("Locating WebAssembly linker...");

    std::optional<std::filesystem::path> wasm_ld_path = find_tool("wasm-ld");
    if (!wasm_ld_path) {
        error("failed to find wasm linker");
    }

    print_step("  Found WebAssembly linker: " + wasm_ld_path->string());
    toolchain.linker_path = wasm_ld_path->string();

    return toolchain;
}

JSONObject WasmToolchain::serialize() {
    JSONObject object;
    object.add("linker_path", linker_path);
    return object;
}

EmscriptenToolchain EmscriptenToolchain::detect() {
    EmscriptenToolchain toolchain;

    print_step("Locating Emscripten linker...");

    std::optional<std::filesystem::path> emcc_path = find_tool("emcc", ".bat");
    if (!emcc_path) {
        error("failed to find emscripten linker");
    }

    print_step("  Found Emscripten linker: " + emcc_path->string());
    toolchain.linker_path = emcc_path->string();

    return toolchain;
}

JSONObject EmscriptenToolchain::serialize() {
    JSONObject object;
    object.add("linker_path", linker_path);
    return object;
}

std::vector<std::string> parse_gcc_lib_dirs(std::string_view search_dirs_output) {
    std::vector<std::string_view> search_dirs_lines = Utils::split_string(search_dirs_output, '\n');
    std::vector<std::string_view> raw_lib_search_dirs;

    for (std::string_view line : search_dirs_lines) {
        if (!line.starts_with("libraries: ")) {
            continue;
        }

        line = line.substr(11);

        if (line.starts_with("=")) {
            line = line.substr(1);
        }

        raw_lib_search_dirs = Utils::split_string(line, ':');
    }

    std::vector<std::string> lib_dirs;
    lib_dirs.reserve(raw_lib_search_dirs.size());

    for (std::string_view raw_lib_search_dir : raw_lib_search_dirs) {
        if (!std::filesystem::exists(raw_lib_search_dir)) {
            continue;
        }

        lib_dirs.push_back(std::filesystem::canonical(raw_lib_search_dir).string());
    }

    return Utils::remove_duplicates(lib_dirs);
}

} // namespace cli
} // namespace banjo
