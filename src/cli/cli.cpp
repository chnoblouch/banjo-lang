#include "cli.hpp"

#include "banjo/utils/json_parser.hpp"
#include "banjo/utils/paths.hpp"

#include "common.hpp"
#include "process.hpp"
#include "target.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

namespace banjo {
namespace cli {

static constexpr std::string_view HELP_TEXT =
    R"(
Usage: banjo [options] command

Options:
  -v, --version   Print version and exit
  -h, --help      Print help and exit

Commands:
  build     Build the current package
  run       Build and run the current package
  targets   Print the list of supported targets
  help      Print help and exit
  version   Print version and exit
)";

void CLI::run(int argc, const char *argv[]) {
    if (argc < 2) {
        execute_help();
        return;
    }

    for (int i = 1; i < argc; i++) {
        std::string_view arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            execute_help();
            return;
        } else if (arg == "-v" || arg == "--version") {
            execute_version();
            return;
        }
    }

    if (argc >= 3 && std::string(argv[2]) == "-q") {
        quiet = true;
    }

    load_config();

    std::string sub_command = argv[1];

    if (sub_command == "targets") {
        execute_targets();
    } else if (sub_command == "build") {
        execute_build();
    } else if (sub_command == "run") {
        execute_run();
    } else if (sub_command == "help") {
        execute_help();
    } else if (sub_command == "version") {
        execute_version();
    } else {
        error("unknown command '" + sub_command + "'");
    }
}

void CLI::execute_targets() {
    std::cout << "\n";
    std::cout << "Available targets:\n";

    for (const Target &target : Target::list_available()) {
        std::cout << "  " << target.to_string() << "\n";
    }

    std::cout << "\n";
}

void CLI::execute_version() {
    std::cout << BANJO_VERSION << "\n";
}

void CLI::execute_build() {
    build();
    print_clear_line();
}

void CLI::execute_run() {
    build();
    run_build();
}

void CLI::execute_help() {
    std::cout << HELP_TEXT << "\n";
}

void CLI::load_config() {
    target = Target::host();
    manifest = load_manifest("banjo.json");

    for (std::string_view package : manifest.packages) {
        load_package(package);
    }
}

Manifest CLI::load_manifest(const std::filesystem::path &path) {
    Manifest manifest = parse_manifest(path);
    libraries.insert(libraries.end(), manifest.libraries.begin(), manifest.libraries.end());

    for (const auto &[manifest_target, sub_manifest] : manifest.target_manifests) {
        if (manifest_target == target) {
            libraries.insert(libraries.end(), sub_manifest.libraries.begin(), sub_manifest.libraries.end());
        }
    }

    return manifest;
}

void CLI::load_package(std::string_view name) {
    std::filesystem::path path = std::filesystem::path("packages") / name;
    load_manifest(path / "banjo.json");

    std::filesystem::path lib_path = path / "lib";
    std::filesystem::path target_lib_path = lib_path / target.to_string();

    library_paths.push_back(lib_path.string());
    library_paths.push_back(target_lib_path.string());
}

Manifest CLI::parse_manifest(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    stream.seekg(0, std::ios::end);
    std::size_t file_size = static_cast<std::size_t>(stream.tellg());
    stream.seekg(0, std::ios::beg);

    std::string buffer;
    buffer.resize(file_size);
    stream.read(buffer.data(), file_size);

    JSONObject json = JSONParser(buffer).parse_object();
    return parse_manifest(json);
}

Manifest CLI::parse_manifest(const JSONObject &json) {
    Manifest manifest;
    manifest.name = json.get_string_or("name", "");

    if (auto json_libraries = json.try_get_array("libraries")) {
        for (const JSONValue &json_library : *json_libraries) {
            manifest.libraries.push_back(json_library.as_string());
        }
    }

    if (auto json_targets = json.try_get_object("targets")) {
        for (const auto &[target_string, json_properties] : *json_targets) {
            Target target = parse_target(target_string);
            Manifest target_manifest = parse_manifest(json_properties.as_object());
            manifest.target_manifests.push_back({target, target_manifest});
        }
    }

    if (auto json_packages = json.try_get_array("packages")) {
        for (const JSONValue &json_package : *json_packages) {
            manifest.packages.push_back(json_package.as_string());
        }
    }

    return manifest;
}

Target CLI::parse_target(std::string_view string) {
    std::vector<std::string> components;

    std::string buffer;
    unsigned index = 0;

    while (index < string.size()) {
        buffer.push_back(string[index]);
        index += 1;

        if (index == string.size() || string[index] == '-') {
            if (!buffer.empty()) {
                components.push_back(buffer);
                buffer.clear();
            }

            index += 1;
        }
    }

    if (components.size() == 2) {
        return Target(components[0], components[1]);
    } else if (components.size() == 3) {
        return Target(components[0], components[1], components[2]);
    } else {
        return Target();
    }
}

void CLI::build() {
    std::filesystem::create_directories(get_output_dir());
    invoke_compiler();
    invoke_linker();
}

void CLI::invoke_compiler() {
    print_step("Compiling...");

    std::vector<std::string> args;

    args.push_back("--type");
    args.push_back("executable");

    args.push_back("--arch");
    args.push_back(target.arch);

    args.push_back("--os");
    args.push_back(target.os);

    if (target.env) {
        args.push_back("--env");
        args.push_back(*target.env);
    }

    args.push_back("--opt-level");
    args.push_back("0");

    args.push_back("--path");
    args.push_back("src/");

    for (const std::string &package : manifest.packages) {
        args.push_back("--path");
        args.push_back("packages/" + package + "/src");
    }

    args.push_back("--optional-semicolons");

    std::optional<Process> process = Process::spawn(
        Command{
            .executable = "banjo-compiler",
            .args = args,
        }
    );

    ProcessResult result = process->wait();

    if (result.exit_code != 0) {
        std::exit(1);
    }
}

void CLI::invoke_linker() {
    print_step("Linking...");

    if (target.os == "windows") {
        invoke_windows_linker();
    } else if (target.os == "linux") {
        invoke_unix_linker();
    }
}

void CLI::invoke_windows_linker() {
    std::vector<std::string> args{
        "main.obj",
        "/OUT:" + get_output_path(),
        "msvcrt.lib",
        "kernel32.lib",
        "user32.lib",
        "legacy_stdio_definitions.lib",
        "ws2_32.lib",
        "shlwapi.lib",
        "dbghelp.lib",
        "/SUBSYSTEM:CONSOLE",
        "/MACHINE:x64",
        "/DEBUG:FULL",
    };

    for (const std::string &library_path : library_paths) {
        args.push_back("/LIBPATH:" + library_path);
    }

    args.push_back("user32.lib");
    args.push_back("gdi32.lib");
    args.push_back("shell32.lib");

    for (const std::string &library : libraries) {
        args.push_back(library + ".lib");
    }

    std::optional<Process> process = Process::spawn(
        Command{
            .executable = "lld-link",
            .args = args,
        }
    );

    process->wait();
}

void CLI::invoke_unix_linker() {
    std::optional<Process> process = Process::spawn(
        Command{
            .executable = "ld.lld",
            .args{
                "-L/usr/lib",
                "-L/usr/lib/x86_64-linux-gnu",
                "-L/usr/lib/gcc/x86_64-linux-gnu/13",
                "-L/usr/lib64",
                "-L/usr/lib/llvm-18/lib/clang/18",
                "-lc",
                "-lgcc_s",
                "-lm",
                "-ldl",
                "-lpthread",
                "--dynamic-linker",
                "/lib64/ld-linux-x86-64.so.2",
                "main.o",
                "-o",
                get_output_path(),
                "/usr/lib/x86_64-linux-gnu/crt1.o",
                "/usr/lib/x86_64-linux-gnu/crti.o",
                "/usr/lib/x86_64-linux-gnu/crtn.o",
            },
        }
    );

    process->wait();
}

void CLI::run_build() {
    print_step("Running...");
    print_clear_line();

    std::optional<Process> process = Process::spawn(
        Command{
            .executable = get_output_path(),
        }
    );

    process->wait();
}

std::string CLI::get_output_path() {
    return (get_output_dir() / (manifest.name + ".exe")).string();
}

std::filesystem::path CLI::get_output_dir() {
    return std::filesystem::path(".") / "out" / (target.to_string() + "-debug");
}

} // namespace cli
} // namespace banjo
