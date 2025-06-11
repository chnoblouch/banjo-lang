#include "cli.hpp"

#include "banjo/utils/json_parser.hpp"

#include "argument_parser.hpp"
#include "common.hpp"
#include "process.hpp"
#include "target.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

namespace banjo {
namespace cli {

static const ArgumentParser::Option OPTION_HELP{
    "help",
    'h',
    "Print help and exit",
};

static const ArgumentParser::Option OPTION_VERSION{
    "version",
    'v',
    "Print version and exit",
};

static const ArgumentParser::Option OPTION_QUIET{
    "quiet",
    'q',
    "Don't print status messages",
};

static const ArgumentParser::Option OPTION_VERBOSE{
    "verbose",
    'V',
    "Print commands before execution",
};

void CLI::run(int argc, const char *argv[]) {
    arg_parser = ArgumentParser{
        .name = "banjo",
        .options{OPTION_HELP, OPTION_VERSION, OPTION_QUIET, OPTION_VERBOSE},
        .commands{
            {
                "build",
                "Build the current package",
                {
                    OPTION_HELP,
                    OPTION_QUIET,
                    OPTION_VERBOSE,
                },
            },
            {
                "run",
                "Build and run the current package",
                {
                    OPTION_HELP,
                    OPTION_QUIET,
                    OPTION_VERBOSE,
                },
            },
            {
                "targets",
                "Print the list of supported targets",
                {
                    OPTION_HELP,
                    OPTION_QUIET,
                    OPTION_VERBOSE,
                },
            },
            {
                "help",
                "Print help and exit",
                {
                    OPTION_HELP,
                    OPTION_QUIET,
                    OPTION_VERBOSE,
                },
            },
            {
                "version",
                "Print version and exit",
                {
                    OPTION_HELP,
                    OPTION_QUIET,
                    OPTION_VERBOSE,
                },
            }
        },
    };

    int arg_index = 1;

    while (arg_index < argc) {
        std::string_view arg = argv[arg_index];

        if (arg == "-h" || arg == "--help") {
            execute_help();
            return;
        } else if (arg == "-v" || arg == "--version") {
            execute_version();
            return;
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if (arg == "-V" || arg == "--verbose") {
            verbose = true;
        } else if (!arg.starts_with('-')) {
            break;
        }

        arg_index += 1;
    }

    if (arg_index == argc) {
        execute_help();
        return;
    }

    std::string command_name = argv[arg_index];
    ArgumentParser::Command *command = nullptr;

    for (ArgumentParser::Command &candidate : arg_parser.commands) {
        if (candidate.name == command_name) {
            command = &candidate;
            break;
        }
    }

    if (!command) {
        error("unknown command '" + command_name + "'");
    }

    while (arg_index < argc) {
        std::string_view arg = argv[arg_index];

        if (arg == "-h" || arg == "--help") {
            arg_parser.print_command_help(*command);
            return;
        } else if (arg == "-q" || arg == "--quiet") {
            quiet = true;
        } else if (arg == "-V" || arg == "--verbose") {
            verbose = true;
        }

        arg_index += 1;
    }

    if (command->name == "build" || command->name == "run") {
        start_time = std::chrono::steady_clock::now();
    }

    load_config();

    if (command->name == "targets") {
        execute_targets();
    } else if (command->name == "build") {
        execute_build();
    } else if (command->name == "run") {
        execute_run();
    } else if (command->name == "help") {
        execute_help();
    } else if (command->name == "version") {
        execute_version();
    }
}

void CLI::execute_targets() {
    Target host = Target::host();

    std::cout << "\n";
    std::cout << "Available targets:\n";

    for (const Target &target : Target::list_available()) {
        std::cout << "  - " << target.to_string();

        if (target == host) {
            std::cout << " (host)";
        }

        std::cout << "\n";
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
    arg_parser.print_help();
}

void CLI::load_config() {
    target = Target::host();
    source_paths.push_back("src");

    manifest = parse_manifest("banjo.json");
    load_root_manifest(manifest);
}

void CLI::load_root_manifest(const Manifest &manifest) {
    load_manifest(manifest);

    if (manifest.type == "executable") {
        package_type = PackageType::EXECUTABLE;
    } else if (manifest.type == "static_library") {
        package_type = PackageType::STATIC_LIBRARY;
    } else if (manifest.type == "shared_library") {
        package_type = PackageType::SHARED_LIBRARY;
    }
}

void CLI::load_manifest(const Manifest &manifest) {
    libraries.insert(libraries.end(), manifest.libraries.begin(), manifest.libraries.end());

    for (std::string_view package : manifest.packages) {
        load_package(package);
    }

    for (const auto &[manifest_target, sub_manifest] : manifest.target_manifests) {
        if (manifest_target == target) {
            load_manifest(sub_manifest);
        }
    }
}

void CLI::load_package(std::string_view name) {
    std::filesystem::path path = std::filesystem::path("packages") / name;
    Manifest manifest = parse_manifest(path / "banjo.json");
    load_manifest(manifest);

    std::filesystem::path src_path = path / "src";
    std::filesystem::path lib_path = path / "lib";
    std::filesystem::path target_lib_path = lib_path / target.to_string();

    if (std::filesystem::is_directory(src_path)) {
        source_paths.push_back(src_path.string());
    }

    if (std::filesystem::is_directory(lib_path)) {
        library_paths.push_back(lib_path.string());
    }

    if (std::filesystem::is_directory(target_lib_path)) {
        library_paths.push_back(target_lib_path.string());
    }
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
    manifest.type = json.get_string_or("type", "executable");

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

    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    std::chrono::duration duration = end_time - start_time;
    unsigned duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    float duration_s = static_cast<float>(duration_ms) / 1000.0f;

    if (verbose) {
        std::ostringstream string_stream;
        string_stream << std::fixed << std::setprecision(2) << duration_s;
        print_step("Build finished (" + string_stream.str() + " seconds)");
    }
}

void CLI::invoke_compiler() {
    print_step("Compiling...");

    std::vector<std::string> args;
    args.push_back("--type");

    switch (package_type) {
        case PackageType::EXECUTABLE: args.push_back("executable"); break;
        case PackageType::STATIC_LIBRARY: args.push_back("static_library"); break;
        case PackageType::SHARED_LIBRARY: args.push_back("shared_library"); break;
    }

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

    for (const std::string &path : source_paths) {
        args.push_back("--path");
        args.push_back(path);
    }

    args.push_back("--optional-semicolons");

    Command command{
        .executable = "banjo-compiler",
        .args = args,
    };

    print_command("compiler", command);

    std::optional<Process> process = Process::spawn(command);
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

    if (package_type == PackageType::SHARED_LIBRARY) {
        args.push_back("/DLL");
    }

    for (const std::string &library_path : library_paths) {
        args.push_back("/LIBPATH:" + library_path);
    }

    args.push_back("user32.lib");
    args.push_back("gdi32.lib");
    args.push_back("shell32.lib");

    for (const std::string &library : libraries) {
        args.push_back(library + ".lib");
    }

    Command command{
        .executable = "lld-link",
        .args = args,
    };

    print_command("linker", command);

    std::optional<Process> process = Process::spawn(command);
    process->wait();

    std::filesystem::remove("main.obj");
}

void CLI::invoke_unix_linker() {
    std::vector<std::string> args{
        "main.o",
        "-o",
        get_output_path(),
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
    };

    if (package_type == PackageType::EXECUTABLE) {
        args.push_back("/usr/lib/x86_64-linux-gnu/crt1.o");
        args.push_back("/usr/lib/x86_64-linux-gnu/crti.o");
        args.push_back("/usr/lib/x86_64-linux-gnu/crtn.o");
    } else if (package_type == PackageType::SHARED_LIBRARY) {
        args.push_back("-shared");
    }

    for (const std::string &library_path : library_paths) {
        args.push_back("-L" + library_path);
    }

    for (const std::string &library : libraries) {
        args.push_back("-l" + library);
    }

    Command command{
        .executable = "ld.lld",
        .args = args,
    };

    print_command("linker", command);

    std::optional<Process> process = Process::spawn(command);
    process->wait();

    std::filesystem::remove("main.o");
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
    std::string file_name;

    if (package_type == PackageType::EXECUTABLE) {
        if (target.os == "windows") {
            file_name = manifest.name + ".exe";
        } else {
            file_name = manifest.name;
        }
    } else if (package_type == PackageType::STATIC_LIBRARY) {
        if (target.os == "windows" && target.env == "msvc") {
            file_name = manifest.name + ".lib";
        } else {
            file_name = manifest.name + ".a";
        }
    } else if (package_type == PackageType::SHARED_LIBRARY) {
        if (target.os == "windows") {
            file_name = manifest.name + ".dll";
        } else if (target.os == "macos") {
            file_name = "lib" + manifest.name + ".dylib";
        } else {
            file_name = "lib" + manifest.name + ".so";
        }
    }

    return (get_output_dir() / file_name).string();
}

std::filesystem::path CLI::get_output_dir() {
    return std::filesystem::path("out") / (target.to_string() + "-debug");
}

} // namespace cli
} // namespace banjo
