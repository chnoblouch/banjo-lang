#include "toolchains.hpp"

#include "banjo/utils/json.hpp"
#include "banjo/utils/utils.hpp"

#include "common.hpp"
#include <filesystem>

namespace banjo {
namespace cli {

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
        print_step("  Version: " + get_tool_output(*lld_path, {"-flavor", "gnu", "-v"}));

        linker_path = lld_path->string();
        linker_args = {"-flavor", "gnu"};
        return;
    }

    std::optional<std::filesystem::path> ld_path = find_tool("ld");

    if (ld_path) {
        print_step("  Found LD: " + ld_path->string());
        print_step("  Version: " + get_tool_output(*ld_path, {"-v"}));

        linker_path = ld_path->string();
        return;
    }

    error("failed to find system linker");
}

void UnixToolchain::find_lib_dirs() {
    std::filesystem::path c_compiler_path = find_c_compiler();
    std::string search_dirs_output = get_tool_output(c_compiler_path, {"--print-search-dirs"});
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

    lib_dirs.reserve(raw_lib_search_dirs.size());

    for (std::string_view raw_lib_search_dir : raw_lib_search_dirs) {
        lib_dirs.push_back(std::filesystem::canonical(raw_lib_search_dir).string());
    }

    lib_dirs = Utils::remove_duplicates(lib_dirs);

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

} // namespace cli
} // namespace banjo
