#include "cli.hpp"

#include "banjo/utils/json.hpp"
#include "banjo/utils/json_parser.hpp"
#include "banjo/utils/paths.hpp"
#include "banjo/utils/utils.hpp"

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
    ArgumentParser::Option::Type::FLAG,
    "help",
    'h',
    "Print help and exit",
};

static const ArgumentParser::Option OPTION_VERSION{
    ArgumentParser::Option::Type::FLAG,
    "version",
    'v',
    "Print version and exit",
};

static const ArgumentParser::Option OPTION_QUIET{
    ArgumentParser::Option::Type::FLAG,
    "quiet",
    'q',
    "Don't print status messages",
};

static const ArgumentParser::Option OPTION_VERBOSE{
    ArgumentParser::Option::Type::FLAG,
    "verbose",
    'V',
    "Print commands before execution",
};

static const ArgumentParser::Option OPTION_TARGET{
    ArgumentParser::Option::Type::VALUE,
    "target",
    "{target}",
    "Target to build for (default: host)",
};

static const ArgumentParser::Option OPTION_CONFIG{
    ArgumentParser::Option::Type::VALUE,
    "config",
    "{debug,release}",
    "Build configuration (default: debug)",
};

static const ArgumentParser::Option OPTION_OPT_LEVEL{
    ArgumentParser::Option::Type::VALUE,
    "opt-level",
    "{0,1,2}",
    "Compiler optimization level",
};

static const ArgumentParser::Option OPTION_FORCE_ASM{
    ArgumentParser::Option::Type::FLAG,
    "force-asm",
    "Force the use of an external assembler",
};

static const ArgumentParser::Command COMMAND_BUILD{
    "build",
    "Build the current package",
    {
        OPTION_HELP,
        OPTION_TARGET,
        OPTION_CONFIG,
        OPTION_OPT_LEVEL,
        OPTION_FORCE_ASM,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_RUN{
    "run",
    "Build and run the current package",
    {
        OPTION_HELP,
        OPTION_CONFIG,
        OPTION_OPT_LEVEL,
        OPTION_FORCE_ASM,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_TOOLCHAINS{
    "toolchains",
    "Print the cached toolchains",
    {},
};

static const ArgumentParser::Command COMMAND_TARGETS{
    "targets",
    "Print the supported targets",
    {
        OPTION_HELP,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_HELP{
    "help",
    "Print help and exit",
    {
        OPTION_HELP,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_VERSION{
    "version",
    "Print version and exit",
    {
        OPTION_HELP,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
};

void CLI::run(int argc, const char *argv[]) {
    arg_parser = ArgumentParser{
        .argc = argc,
        .argv = argv,
        .name = "banjo",
        .options{OPTION_HELP, OPTION_VERSION, OPTION_QUIET, OPTION_VERBOSE},
        .commands{COMMAND_BUILD, COMMAND_RUN, COMMAND_TARGETS, COMMAND_TOOLCHAINS, COMMAND_HELP, COMMAND_VERSION},
    };

    ArgumentParser::Result args = arg_parser.parse();

    for (const ArgumentParser::OptionValue &option_value : args.global_options) {
        if (option_value.option->name == OPTION_HELP.name) {
            execute_help();
            return;
        } else if (option_value.option->name == OPTION_VERSION.name) {
            execute_version();
            return;
        } else if (option_value.option->name == OPTION_QUIET.name) {
            quiet = true;
        } else if (option_value.option->name == OPTION_VERBOSE.name) {
            verbose = true;
        }
    }

    if (!args.command) {
        execute_help();
        return;
    }

    for (const ArgumentParser::OptionValue &option_value : args.command_options) {
        std::string_view name = option_value.option->name;

        if (name == OPTION_TARGET.name) {
            target_override = parse_target(*option_value.value);
        } else if (name == OPTION_CONFIG.name) {
            if (option_value.value == "debug") {
                build_config = BuildConfig::DEBUG;
            } else if (option_value.value == "release") {
                build_config = BuildConfig::RELEASE;
            } else {
                error("unexpected build config '" + *option_value.value + "'");
            }
        } else if (name == OPTION_OPT_LEVEL.name) {
            if (option_value.value == "0") {
                opt_level = 0;
            } else if (option_value.value == "1") {
                opt_level = 1;
            } else if (option_value.value == "2") {
                opt_level = 2;
            } else {
                error("unexpected optimization level '" + *option_value.value + "'");
            }
        } else if (name == OPTION_FORCE_ASM.name) {
            force_assembler = true;
        } else if (name == OPTION_HELP.name) {
            arg_parser.print_command_help(*args.command);
            return;
        } else if (name == OPTION_QUIET.name) {
            quiet = true;
        } else if (name == OPTION_VERBOSE.name) {
            verbose = true;
        }
    }

    if (args.command->name == COMMAND_BUILD.name || args.command->name == COMMAND_RUN.name) {
        start_time = std::chrono::steady_clock::now();
    }

    load_config();

    if (args.command->name == COMMAND_TARGETS.name) {
        execute_targets();
    } else if (args.command->name == COMMAND_TOOLCHAINS.name) {
        execute_toolchains();
    } else if (args.command->name == COMMAND_BUILD.name) {
        execute_build();
    } else if (args.command->name == COMMAND_RUN.name) {
        execute_run();
    } else if (args.command->name == COMMAND_HELP.name) {
        execute_help();
    } else if (args.command->name == COMMAND_VERSION.name) {
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

void CLI::execute_toolchains() {
    std::cout << "\n";
    std::cout << "Available toolchains:\n";

    for (std::filesystem::path path : std::filesystem::directory_iterator(get_toolchains_dir())) {
        if (std::filesystem::is_regular_file(path) && path.extension() == ".json") {
            std::cout << "  - " << path.filename().string() << "\n";
        }
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
    target = target_override ? *target_override : Target::host();
    load_toolchain();

    source_paths.push_back("src");

    manifest = parse_manifest("banjo.json");
    load_root_manifest(manifest);
}

void CLI::load_toolchain() {
    std::filesystem::path toolchain_path = get_toolchains_dir() / (target.to_string() + ".json");
    std::optional<std::string> toolchain_string = Utils::read_string_file(toolchain_path);

    if (!toolchain_string) {
        error("failed to load cached toolchain file for target " + target.to_string());
    }

    toolchain = Toolchain{
        .properties = JSONParser(*toolchain_string).parse_object(),
    };
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

    if (force_assembler) {
        invoke_assembler();
    }

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

    if (opt_level) {
        args.push_back(std::to_string(*opt_level));
    } else if (build_config == BuildConfig::DEBUG) {
        args.push_back("0");
    } else if (build_config == BuildConfig::RELEASE) {
        args.push_back("1");
    }

    if (force_assembler) {
        args.push_back("--force-asm");
    }

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

void CLI::invoke_assembler() {
    print_step("Assembling...");

    if (target.arch == "x86_64") {
        invoke_nasm_assembler();
    } else if (target.arch == "aarch64") {
        invoke_aarch64_assembler();
    }
}

void CLI::invoke_nasm_assembler() {
    std::vector<std::string> args;

    if (target.os == "windows") {
        args.push_back("-fwin64");
    } else if (target.os == "linux") {
        args.push_back("-felf64");
    }

    args.push_back("main.asm");

    Command command{
        .executable = "nasm",
        .args = args,
    };

    print_command("assembler", command);

    std::optional<Process> process = Process::spawn(command);
    process->wait();

    std::filesystem::remove("main.asm");
}

void CLI::invoke_aarch64_assembler() {
    std::vector<std::string> args;
    args.push_back("-c");
    args.push_back("-target");

    if (target.os == "linux") {
        args.push_back("aarch64-linux");
    } else if (target.os == "macos") {
        args.push_back("aarch64-darwin");
    }

    args.push_back("main.s");

    Command command{
        .executable = "clang",
        .args = args,
    };

    print_command("assembler", command);

    std::optional<Process> process = Process::spawn(command);
    process->wait();

    std::filesystem::remove("main.s");
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
    std::filesystem::path msvc_tools_root_path(toolchain.properties.get_string("tools"));
    std::filesystem::path msvc_lib_root_path(toolchain.properties.get_string("lib"));

    std::filesystem::path msvc_tools_path = msvc_tools_root_path / "bin" / "Hostx64" / "x64";
    std::filesystem::path msvc_lib_path = msvc_tools_root_path / "lib" / "x64";
    std::filesystem::path msvc_um_lib_path = msvc_lib_root_path / "um" / "x64";
    std::filesystem::path msvc_ucrt_lib_path = msvc_lib_root_path / "ucrt" / "x64";

    std::vector<std::string> args{
        "main.obj",
        "/OUT:" + get_output_path(),
        "/LIBPATH:" + msvc_lib_path.string(),
        "/LIBPATH:" + msvc_um_lib_path.string(),
        "/LIBPATH:" + msvc_ucrt_lib_path.string(),
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
        .executable = (msvc_tools_path / "link").string(),
        .args = args,
    };

    print_command("linker", command);

    std::optional<Process> process = Process::spawn(command);
    process->wait();

    std::filesystem::remove("main.obj");
}

void CLI::invoke_unix_linker() {
    std::filesystem::path linker_path(toolchain.properties.get_string("linker_path"));
    std::vector<std::string> linker_args = toolchain.properties.get_string_array("linker_args");
    std::vector<std::string> additional_libraries = toolchain.properties.get_string_array("additional_libraries");
    std::vector<std::string> lib_dirs = toolchain.properties.get_string_array("lib_dirs");
    std::filesystem::path crt_dir(toolchain.properties.get_string("crt_dir"));

    std::vector<std::string> args;
    args.insert(args.end(), linker_args.begin(), linker_args.end());
    args.push_back("main.o");
    args.push_back("-o");
    args.push_back(get_output_path());

    for (const std::string &lib_dir : lib_dirs) {
        args.push_back("-L" + lib_dir);
    }

    args.push_back("-lc");
    args.push_back("-lgcc_s");
    args.push_back("-lm");
    args.push_back("-ldl");
    args.push_back("-lpthread");

    for (const std::string &lib : additional_libraries) {
        args.push_back("-l" + lib);
    }

    args.push_back("--dynamic-linker");

    if (target.arch == "x86_64") {
        args.push_back("/lib64/ld-linux-x86-64.so.2");
    } else if (target.arch == "aarch64") {
        args.push_back("/lib/ld-linux-aarch64.so.1");
    }

    if (package_type == PackageType::EXECUTABLE) {
        args.push_back((crt_dir / "crt1.o").string());
        args.push_back((crt_dir / "crti.o").string());
        args.push_back((crt_dir / "crtn.o").string());
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
        .executable = linker_path.string(),
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

std::filesystem::path CLI::get_toolchains_dir() {
    return Paths::executable().parent_path().parent_path() / "toolchains";
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
