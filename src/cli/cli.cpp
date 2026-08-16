#include "cli.hpp"

#include "banjo/utils/json.hpp"
#include "banjo/utils/json_parser.hpp"
#include "banjo/utils/json_serializer.hpp"
#include "banjo/utils/macros.hpp"
#include "banjo/utils/utils.hpp"

#include "argument_parser.hpp"
#include "common.hpp"
#include "paths.hpp"
#include "process.hpp"
#include "target.hpp"
#include "toolchains.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace banjo::cli {

static const ArgumentParser::Option OPTION_HELP{
    ArgumentParser::Option::Type::FLAG,
    "help",
    'h',
    "Print help and exit",
};

static const ArgumentParser::Option OPTION_VERSION{
    ArgumentParser::Option::Type::FLAG,
    "version",
    'V',
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
    'v',
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

static const ArgumentParser::Option OPTION_HOT_RELOAD{
    ArgumentParser::Option::Type::FLAG,
    "hot-reload",
    "Enable hot reloading",
};

static const ArgumentParser::Option OPTION_BINDGEN_GENERATOR{
    ArgumentParser::Option::Type::VALUE,
    "generator",
    'g',
    "{path}",
    "Path to the generator Python script",
};

static const ArgumentParser::Option OPTION_BINDGEN_INCLUDE_PATH{
    ArgumentParser::Option::Type::VALUE,
    "include",
    'I',
    "{path}",
    "Add a path as a C include directory",
};

static const ArgumentParser::Option OPTION_DEBUG_COMPILER{
    ArgumentParser::Option::Type::FLAG,
    "debug-compiler",
    "Emit internal data for debugging the compiler",
};

static const ArgumentParser::Positional POSITIONAL_NAME{"name"};
static const ArgumentParser::Positional POSITIONAL_TEST_NAME{"test name", true};
static const ArgumentParser::Positional POSITIONAL_TOOL{"tool"};
static const ArgumentParser::Positional POSITIONAL_TOOLCHAIN_TARGET{"target", true};
static const ArgumentParser::Positional POSITIONAL_FORMAT_FILE{"file"};
static const ArgumentParser::Positional POSITIONAL_BINDGEN_SOURCE{"file"};

static const ArgumentParser::Command COMMAND_NEW{
    .name = "new",
    .description = "Create a new package in the current working directory",
    .options{
        &OPTION_HELP,
    },
    .positional = &POSITIONAL_NAME,
};

static const ArgumentParser::Command COMMAND_BUILD{
    .name = "build",
    .description = "Build the current package",
    .options{
        &OPTION_HELP,
        &OPTION_TARGET,
        &OPTION_CONFIG,
        &OPTION_OPT_LEVEL,
        &OPTION_FORCE_ASM,
        &OPTION_DEBUG_COMPILER,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_RUN{
    .name = "run",
    .description = "Build and run the current package",
    .options{
        &OPTION_HELP,
        &OPTION_TARGET,
        &OPTION_HOT_RELOAD,
        &OPTION_CONFIG,
        &OPTION_OPT_LEVEL,
        &OPTION_FORCE_ASM,
        &OPTION_DEBUG_COMPILER,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_TEST{
    .name = "test",
    .description = "Build and run tests of the current package",
    .options{
        &OPTION_HELP,
        &OPTION_CONFIG,
        &OPTION_OPT_LEVEL,
        &OPTION_FORCE_ASM,
        &OPTION_DEBUG_COMPILER,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
    .positional = &POSITIONAL_TEST_NAME,
};

static const ArgumentParser::Command COMMAND_INVOKE{
    .name = "invoke",
    .description = "Run a program from the toolchain (compiler, assembler, or linker)",
    .options{
        &OPTION_HELP,
        &OPTION_TARGET,
        &OPTION_CONFIG,
        &OPTION_OPT_LEVEL,
        &OPTION_FORCE_ASM,
        &OPTION_DEBUG_COMPILER,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
    .positional = &POSITIONAL_TOOL,
};

static const ArgumentParser::Command COMMAND_TOOLCHAIN_LIST{
    .name = "list",
    .description = "Print cached toolchains",
    .options{
        &OPTION_HELP,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_TOOLCHAIN_INFO{
    .name = "info",
    .description = "Print information about a toolchain",
    .options{
        &OPTION_HELP,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
    .positional = &POSITIONAL_TOOLCHAIN_TARGET,
};

static const ArgumentParser::Command COMMAND_TOOLCHAIN_SETUP{
    .name = "setup",
    .description = "Set up a toolchain by scanning the system for tools and downloading required libraries",
    .options{
        &OPTION_HELP,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
    .positional = &POSITIONAL_TOOLCHAIN_TARGET,
};

static const ArgumentParser::Command COMMAND_TOOLCHAIN_REMOVE{
    .name = "remove",
    .description = "Remove a toolchain",
    .options{
        &OPTION_HELP,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
    .positional = &POSITIONAL_TOOLCHAIN_TARGET,
};

static const ArgumentParser::Command COMMAND_TOOLCHAIN{
    .name = "toolchain",
    .description = "Manage system toolchains",
    .options{
        &OPTION_HELP,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
    .subcommands{
        &COMMAND_TOOLCHAIN_LIST,
        &COMMAND_TOOLCHAIN_INFO,
        &COMMAND_TOOLCHAIN_SETUP,
        &COMMAND_TOOLCHAIN_REMOVE,
    },
};

static const ArgumentParser::Command COMMAND_TARGETS{
    .name = "targets",
    .description = "Print the supported targets",
    .options{
        &OPTION_HELP,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_LSP{
    .name = "lsp",
    .description = "Launch the language server",
    .options{
        &OPTION_HELP,
        &OPTION_TARGET,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_FORMAT{
    .name = "format",
    .description = "Format source files",
    .options{
        &OPTION_HELP,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
    .positional = &POSITIONAL_FORMAT_FILE,
};

static const ArgumentParser::Command COMMAND_BINDGEN{
    .name = "bindgen",
    .description = "Generate bindings to C libraries",
    .options{
        &OPTION_HELP,
        &OPTION_BINDGEN_GENERATOR,
        &OPTION_BINDGEN_INCLUDE_PATH,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
    .positional = &POSITIONAL_BINDGEN_SOURCE,
};

static const ArgumentParser::Command COMMAND_HELP{
    .name = "help",
    .description = "Print help and exit",
    .options{
        &OPTION_HELP,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_VERSION{
    .name = "version",
    .description = "Print version and exit",
    .options{
        &OPTION_HELP,
        &OPTION_QUIET,
        &OPTION_VERBOSE,
    },
};

void CLI::run(int argc, const char *argv[]) {
    arg_parser = ArgumentParser{
        .argc = argc,
        .argv = argv,
        .name = "banjo",
        .options{&OPTION_HELP, &OPTION_VERSION, &OPTION_QUIET, &OPTION_VERBOSE},
        .commands{
            &COMMAND_NEW,
            &COMMAND_BUILD,
            &COMMAND_RUN,
            &COMMAND_TEST,
            &COMMAND_INVOKE,
            &COMMAND_TARGETS,
            &COMMAND_LSP,
            &COMMAND_FORMAT,
            &COMMAND_BINDGEN,
            &COMMAND_TOOLCHAIN,
            &COMMAND_HELP,
            &COMMAND_VERSION,
        },
    };

    ArgumentParser::Result args = arg_parser.parse();

    for (const ArgumentParser::OptionValue &option_value : args.global_options) {
        if (option_value.option == &OPTION_HELP) {
            execute_help();
            return;
        } else if (option_value.option == &OPTION_VERSION) {
            execute_version();
            return;
        } else if (option_value.option == &OPTION_QUIET) {
            quiet = true;
        } else if (option_value.option == &OPTION_VERBOSE) {
            verbose = true;
            single_line_output = false;
        }
    }

    if (!args.command) {
        execute_help();
        return;
    }

    for (const ArgumentParser::OptionValue &option_value : args.command_options) {
        const ArgumentParser::Option *option = option_value.option;

        if (option == &OPTION_TARGET) {
            target_override = parse_target(*option_value.value);
        } else if (option == &OPTION_CONFIG) {
            if (option_value.value == "debug") {
                build_config = BuildConfig::DEBUG;
            } else if (option_value.value == "release") {
                build_config = BuildConfig::RELEASE;
            } else {
                error("unexpected build config '" + *option_value.value + "'");
            }
        } else if (option == &OPTION_OPT_LEVEL) {
            if (option_value.value == "0") {
                opt_level = 0;
            } else if (option_value.value == "1") {
                opt_level = 1;
            } else if (option_value.value == "2") {
                opt_level = 2;
            } else {
                error("unexpected optimization level '" + *option_value.value + "'");
            }
        } else if (option == &OPTION_FORCE_ASM) {
            force_assembler = true;
        } else if (option == &OPTION_HOT_RELOAD) {
            hot_reloading_enabled = true;
        } else if (option == &OPTION_DEBUG_COMPILER) {
            extra_compiler_args.push_back("--debug");
        } else if (option == &OPTION_HELP) {
            arg_parser.print_command_help(*args.command);
            return;
        } else if (option == &OPTION_QUIET) {
            quiet = true;
        } else if (option == &OPTION_VERBOSE) {
            verbose = true;
            single_line_output = false;
        }
    }

    if (utils::is_one_of(args.command, {&COMMAND_BUILD, &COMMAND_RUN})) {
        start_time = std::chrono::steady_clock::now();
    }

    if (args.command == &COMMAND_TARGETS) {
        execute_targets();
    } else if (args.command == &COMMAND_TOOLCHAIN_LIST) {
        execute_toolchain_list();
    } else if (args.command == &COMMAND_TOOLCHAIN_INFO) {
        execute_toolchain_info(args);
    } else if (args.command == &COMMAND_TOOLCHAIN_SETUP) {
        execute_toolchain_setup(args);
    } else if (args.command == &COMMAND_TOOLCHAIN_REMOVE) {
        execute_toolchain_remove(args);
    } else if (args.command == &COMMAND_NEW) {
        execute_new(args);
    } else if (args.command == &COMMAND_BUILD) {
        execute_build();
    } else if (args.command == &COMMAND_RUN) {
        execute_run();
    } else if (args.command == &COMMAND_TEST) {
        execute_test(args);
    } else if (args.command == &COMMAND_INVOKE) {
        execute_invoke(args);
    } else if (args.command == &COMMAND_LSP) {
        execute_lsp();
    } else if (args.command == &COMMAND_FORMAT) {
        execute_format(args);
    } else if (args.command == &COMMAND_BINDGEN) {
        execute_bindgen(args);
    } else if (args.command == &COMMAND_HELP) {
        execute_help();
    } else if (args.command == &COMMAND_VERSION) {
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

void CLI::execute_toolchain_list() {
    std::cout << "\n";
    std::cout << "Available toolchains:\n";

    std::filesystem::path toolchains_dir = paths::toolchains_dir();

    if (!std::filesystem::is_directory(toolchains_dir)) {
        std::cout << "\n";
        return;
    }

    for (std::filesystem::path path : std::filesystem::directory_iterator(toolchains_dir)) {
        if (std::filesystem::is_regular_file(path) && path.extension() == ".json") {
            std::cout << "  - " << path.stem().string() << "\n";
        }
    }

    std::cout << "\n";
}

void CLI::execute_toolchain_info(const ArgumentParser::Result &args) {
    target = args.command_positional ? parse_target(*args.command_positional) : Target::host();

    if (!load_cached_toolchain()) {
        error("failed to load toolchain for target '" + target.to_string() + "'");
    }

    std::cout << "\n";
    std::cout << "Target: " << target.to_string() << "\n";
    std::cout << "Config path: " << get_toolchain_path().string() << "\n";

    const ToolchainProperties &properties = toolchain->properties();
    json::Object serialized = toolchain->serialize();

    for (const auto &[key, name] : properties) {
        const json::Value &value = serialized.get(std::string{key});

        if (value.is_string()) {
            std::cout << "\n" << name << ":\n  \"" << value.as_string() << "\"\n";
        } else if (value.is_array()) {
            std::cout << "\n" << name << ":\n";

            for (const json::Value &element : value.as_array()) {
                std::cout << "  - \"" << element.as_string() << "\"\n";
            }
        }
    }

    std::cout << "\n";
}

void CLI::execute_toolchain_setup(const ArgumentParser::Result &args) {
    target = args.command_positional ? parse_target(*args.command_positional) : Target::host();
    set_up_toolchain();
}

void CLI::execute_toolchain_remove(const ArgumentParser::Result &args) {
    target = args.command_positional ? parse_target(*args.command_positional) : Target::host();

    if (!load_cached_toolchain()) {
        return;
    }

    toolchain->remove(target);

    std::error_code error;
    std::filesystem::remove(get_toolchain_path(), error);
}

void CLI::execute_version() {
    std::cout << BANJO_VERSION << "\n";
}

void CLI::execute_new(const ArgumentParser::Result &args) {
    const std::string &name = *args.command_positional;
    std::filesystem::path package_path = name;
    std::filesystem::path src_path = package_path / "src";
    std::filesystem::path main_path = src_path / "main.bnj";
    std::filesystem::path manifest_path = package_path / "banjo.json";

    std::filesystem::create_directory(package_path);
    std::filesystem::create_directory(src_path);

    utils::write_string_file("func main() {\n    println(\"Hello, World!\");\n}\n", main_path);
    utils::write_string_file("{\n  \"name\": \"" + name + "\",\n  \"type\": \"executable\"\n}\n", manifest_path);
}

void CLI::execute_build() {
    load_config();
    build();
    print_clear_line();
}

void CLI::execute_run() {
    load_config();

    if (!target.is_executable_on_host()) {
        error("cannot run '" + target.to_string() + "' binaries on this platform");
    }

    build();

    if (hot_reloading_enabled) {
        run_hot_reloader();
    } else {
        run_build();
    }
}

void CLI::execute_test(const ArgumentParser::Result &args) {
    const std::optional<std::string> &name = args.command_positional;

    load_config();

    extra_compiler_args.push_back("--testing");
    package_type = PackageType::EXECUTABLE;

    ProcessResult compiler_result = invoke_compiler();

    if (force_assembler) {
        invoke_assembler();
    }

    invoke_linker();

    std::string tests_raw = utils::convert_eol_to_lf(compiler_result.stdout_buffer);
    std::vector<std::string_view> tests = utils::split_string(tests_raw, '\n');

    unsigned longest_name_length = 0;

    for (std::string_view test : tests) {
        longest_name_length = std::max(longest_name_length, static_cast<unsigned>(test.size()));
    }

    if (name) {
        bool found = false;

        for (std::string_view test : tests) {
            if (test == *name) {
                tests = {test};
                found = true;
                break;
            }
        }

        if (!found) {
            error("test '" + *name + "' not found");
        }
    }

    print_empty_line();
    std::cout << "Tests:\n";
    std::vector<std::pair<std::string_view, std::string>> failures;

    for (std::string_view test : tests) {
        std::cout << "  " << test << " " << std::string(longest_name_length - test.size() + 3, '.') << " ";

        Command run_command{
            .executable = get_output_path(),
            .args{std::string(test)},
        };

        std::optional<Process> run_process = Process::spawn(run_command);
        ProcessResult run_result = run_process->wait();

        if (run_result.exit_code == 0) {
            std::cout << "\x1b[1;32mok\x1b[1;0m\n";
        } else {
            failures.push_back({test, run_result.stdout_buffer});
            std::cout << "\x1b[1;31mfailed\x1b[1;0m\n";
        }
    }

    if (!failures.empty()) {
        std::cout << "\nFailures:\n\n";
        bool indent = true;

        for (const auto &[test, stderr_buffer] : failures) {
            std::cout << "  " << test << ":\n";

            for (char c : stderr_buffer) {
                if (indent) {
                    std::cout << "    ";
                    indent = false;
                }

                std::cout << c;

                if (c == '\n') {
                    indent = true;
                }
            }

            if (!stderr_buffer.ends_with("\n\n") && !stderr_buffer.ends_with("\r\n\r\n")) {
                std::cout << "\n";
            }
        }
    } else {
        std::cout << "\n";
    }

    unsigned passed = tests.size() - failures.size();
    std::cout << "Passed: " << passed << "/" << tests.size() << "\n\n";
}

void CLI::execute_invoke(const ArgumentParser::Result &args) {
    const std::string &tool = *args.command_positional;

    if (tool == "compiler") {
        load_config();
        invoke_compiler();
    } else if (tool == "assembler") {
        load_config();
        invoke_assembler();
    } else if (tool == "linker") {
        load_config();
        invoke_linker();
    } else {
        error("unexpected tool '" + tool + "'");
    }
}

void CLI::execute_lsp() {
    quiet = true;
    load_config();

    std::vector<std::string> args;
    append_compilation_args(args);

    Command command{
        .executable = "banjo-lsp",
        .args = args,
        .stdin_stream = Command::Stream::INHERIT,
        .stdout_stream = Command::Stream::INHERIT,
        .stderr_stream = Command::Stream::INHERIT,
    };

    std::optional<Process> process = Process::spawn(command);
    process->wait();
}

void CLI::execute_format(const ArgumentParser::Result &args) {
    std::string path = *args.command_positional;

    if (!std::filesystem::is_regular_file(path)) {
        error("source file '" + path + "' not found");
    }

    quiet = true;
    load_config();

    std::vector<std::string> command_args;
    append_compilation_args(command_args);
    command_args.push_back(path);

    Command command{
        .executable = "banjo-format",
        .args = command_args,
    };

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();

    if (result.exit_code != 0) {
        print_empty_line();
        std::cerr << result.stderr_buffer;
        std::exit(1);
    }
}

void CLI::execute_bindgen(const ArgumentParser::Result &args) {
    single_line_output = false;

    std::filesystem::path bindgen_path = paths::installation_dir() / "scripts" / "bindgen";
    std::filesystem::path venv_path = bindgen_path / ".venv";

    if (!std::filesystem::is_directory(venv_path)) {
        print_step("Creating Python virtual environment...");

        Command venv_command{
            .executable = get_python_executable(),
            .args{"-m", "venv", venv_path.string()},
            .stdout_stream = Command::Stream::INHERIT,
            .stderr_stream = Command::Stream::INHERIT,
        };

        print_command("python", venv_command);

        std::optional<Process> venv_process = Process::spawn(venv_command);
        venv_process->wait();

        print_step("Installing libclang package...");

#if OS_WINDOWS
        std::filesystem::path pip_path = venv_path / "Scripts" / "pip";
#else
        std::filesystem::path pip_path = venv_path / "bin" / "pip";
#endif

        Command pip_command{
            .executable = pip_path.string(),
            .args{"install", "--disable-pip-version-check", "libclang"},
            .stdout_stream = Command::Stream::INHERIT,
            .stderr_stream = Command::Stream::INHERIT,
        };

        print_command("pip", venv_command);

        std::optional<Process> pip_process = Process::spawn(pip_command);
        pip_process->wait();
    }

#if OS_WINDOWS
    std::filesystem::path python_path = venv_path / "Scripts" / "python";
#else
    std::filesystem::path python_path = venv_path / "bin" / "python";
#endif

    std::vector<std::string> bindgen_args;
    bindgen_args.push_back((bindgen_path / "bindgen.py").string());

    for (const ArgumentParser::OptionValue &value : args.command_options) {
        if (value.option == &OPTION_BINDGEN_GENERATOR) {
            bindgen_args.push_back("--generator");
            bindgen_args.push_back(*value.value);
        } else if (value.option == &OPTION_BINDGEN_INCLUDE_PATH) {
            bindgen_args.push_back("-I");
            bindgen_args.push_back(*value.value);
        }
    }

    bindgen_args.push_back(*args.command_positional);

    Command bindgen_command{
        .executable = python_path.string(),
        .args = std::move(bindgen_args),
        .stdout_stream = Command::Stream::INHERIT,
        .stderr_stream = Command::Stream::INHERIT,
    };

    print_command("bindgen", bindgen_command);

    std::optional<Process> bindgen_process = Process::spawn(bindgen_command);
    bindgen_process->wait();
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
    if (!load_cached_toolchain()) {
        set_up_toolchain();
    }
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
    extra_compiler_args.insert(extra_compiler_args.end(), manifest.args.begin(), manifest.args.end());
    linker_args.insert(linker_args.end(), manifest.linker_args.begin(), manifest.linker_args.end());
    libraries.insert(libraries.end(), manifest.libraries.begin(), manifest.libraries.end());
    macos_frameworks.insert(macos_frameworks.end(), manifest.macos_frameworks.begin(), manifest.macos_frameworks.end());

    if (manifest.build_script) {
        // TODO: Use relative path for package build scripts.
        run_build_script(*manifest.build_script);
    }

    for (std::string_view package : manifest.packages) {
        load_package(package);
    }

    for (const auto &[manifest_target, sub_manifest] : manifest.target_manifests) {
        if (manifest_target == target) {
            load_manifest(*sub_manifest);
        }
    }
}

void CLI::load_package(std::string_view name) {
    install_package(name);

    std::filesystem::path path = std::filesystem::path("packages") / name;
    Manifest manifest = parse_manifest(path / "banjo.json");
    load_manifest(manifest);

    packages.push_back(std::string(name));

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

CLI::ToolchainKind CLI::toolchain_kind() {
    if (target.os == "windows") {
        if (target.env == "msvc") {
            return ToolchainKind::MSVC;
        } else if (target.env == "gnu") {
            return ToolchainKind::MINGW;
        } else {
            ASSERT_UNREACHABLE;
        }
    } else if (target.os == "linux") {
        return ToolchainKind::UNIX;
    } else if (target.os == "macos") {
        return ToolchainKind::MACOS;
    } else if (target.arch == "wasm") {
        if (target.os == "emscripten") {
            return ToolchainKind::EMSCRIPTEN;
        } else if (target.os == "unknown") {
            return ToolchainKind::WASM;
        } else {
            ASSERT_UNREACHABLE;
        }
    } else {
        ASSERT_UNREACHABLE;
    }
}

bool CLI::load_cached_toolchain() {
    std::optional<std::string> toolchain_string = utils::read_string_file(get_toolchain_path());
    if (!toolchain_string) {
        return false;
    }

    json::Object serialized = json::Parser(*toolchain_string).parse()->as_object();

    switch (toolchain_kind()) {
        case ToolchainKind::MSVC:
            toolchain = std::make_unique<MSVCToolchain>(MSVCToolchain::deserialize(serialized));
            break;
        case ToolchainKind::MINGW:
            toolchain = std::make_unique<MinGWToolchain>(MinGWToolchain::deserialize(serialized));
            break;
        case ToolchainKind::UNIX:
            toolchain = std::make_unique<UnixToolchain>(UnixToolchain::deserialize(serialized));
            break;
        case ToolchainKind::MACOS:
            toolchain = std::make_unique<MacOSToolchain>(MacOSToolchain::deserialize(serialized));
            break;
        case ToolchainKind::EMSCRIPTEN:
            toolchain = std::make_unique<EmscriptenToolchain>(EmscriptenToolchain::deserialize(serialized));
            break;
        case ToolchainKind::WASM:
            toolchain = std::make_unique<WasmToolchain>(WasmToolchain::deserialize(serialized));
            break;
    }

    return true;
}

void CLI::set_up_toolchain() {
    single_line_output = false;

    print_step("Setting up toolchain for target " + target.to_string());

    ToolchainKind kind = toolchain_kind();

    if (kind == ToolchainKind::MSVC) {
        toolchain = std::make_unique<MSVCToolchain>(MSVCToolchain::detect());
    } else if (kind == ToolchainKind::MINGW) {
        toolchain = std::make_unique<MinGWToolchain>(MinGWToolchain::detect());
    } else if (kind == ToolchainKind::UNIX) {
        if (Target::host() == target) {
            toolchain = std::make_unique<UnixToolchain>(UnixToolchain::detect());
        } else {
            toolchain = std::make_unique<UnixToolchain>(UnixToolchain::install(target.arch));
        }
    } else if (kind == ToolchainKind::MACOS) {
        if (Target::host().os == target.os) {
            toolchain = std::make_unique<MacOSToolchain>(MacOSToolchain::detect());
        } else {
            toolchain = std::make_unique<MacOSToolchain>(MacOSToolchain::install());
        }
    } else if (kind == ToolchainKind::EMSCRIPTEN) {
        toolchain = std::make_unique<EmscriptenToolchain>(EmscriptenToolchain::detect());
    } else if (kind == ToolchainKind::WASM) {
        toolchain = std::make_unique<WasmToolchain>(WasmToolchain::detect());
    } else {
        ASSERT_UNREACHABLE;
    }

    print_step("  Caching toolchain...");

    std::filesystem::path toolchain_path = get_toolchain_path();
    std::filesystem::create_directories(toolchain_path.parent_path());

    std::ofstream toolchain_stream(toolchain_path, std::ios::binary);
    json::Serializer(toolchain_stream).serialize(toolchain->serialize());
}

Manifest CLI::parse_manifest(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) {
        error("could not find manifest at '" + path.string() + "'");
    }

    if (std::optional<Manifest> manifest = try_parse_manifest(path)) {
        return std::move(*manifest);
    } else {
        error("could not read manifest at '" + path.string() + "'");
    }
}

std::optional<Manifest> CLI::try_parse_manifest(const std::filesystem::path &path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return {};
    }

    stream.seekg(0, std::ios::end);
    std::size_t file_size = static_cast<std::size_t>(stream.tellg());
    stream.seekg(0, std::ios::beg);

    std::string buffer;
    buffer.resize(file_size);
    stream.read(buffer.data(), file_size);

    if (!stream) {
        return {};
    }

    if (std::optional<json::Value> json = json::Parser(buffer).parse()) {
        if (json->is_object()) {
            return parse_manifest(json->as_object());
        } else {
            return {};
        }
    } else {
        return {};
    }
}

Manifest CLI::parse_manifest(const json::Object &json) {
    std::optional<std::string> name;
    std::string type = "executable";
    std::vector<std::string> args;
    std::vector<std::string> linker_args;
    std::vector<std::string> libraries;
    std::vector<std::string> macos_frameworks;
    std::vector<std::string> packages;
    std::vector<std::pair<Target, std::unique_ptr<Manifest>>> target_manifests;
    std::optional<std::string> build_script;

    for (const auto &[member_name, member_value] : json) {
        if (member_name == "name") {
            name = unwrap_json_string(member_name, member_value);
        } else if (member_name == "type") {
            type = unwrap_json_string(member_name, member_value);
        } else if (member_name == "args") {
            args = unwrap_json_string_array(member_name, member_value);
        } else if (member_name == "linker_args") {
            linker_args = unwrap_json_string_array(member_name, member_value);
        } else if (member_name == "libraries") {
            libraries = unwrap_json_string_array(member_name, member_value);
        } else if (member_name == "library_paths") {
            std::vector<std::string> new_paths = unwrap_json_string_array(member_name, member_value);
            library_paths.insert(library_paths.end(), new_paths.begin(), new_paths.end());
        } else if (member_name == "packages") {
            packages = unwrap_json_string_array(member_name, member_value);
        } else if (member_name == "targets") {
            // TODO: Error handling

            for (const auto &[target_string, json_properties] : member_value.as_object()) {
                Target target = parse_target(target_string);
                Manifest target_manifest = parse_manifest(json_properties.as_object());
                target_manifests.push_back({target, std::make_unique<Manifest>(std::move(target_manifest))});
            }
        } else if (member_name == "windows.subsystem") {
            // TODO
        } else if (member_name == "windows.resources") {
            // TODO
        } else if (member_name == "macos.frameworks") {
            macos_frameworks = unwrap_json_string_array(member_name, member_value);
        } else if (member_name == "build_script") {
            build_script = unwrap_json_string(member_name, member_value);
        } else {
            error("failed to load manifest: unknown member " + member_name);
        }
    }

    return Manifest{
        .name = name.value_or(""),
        .type = type,
        .args = args,
        .linker_args = linker_args,
        .libraries = libraries,
        .macos_frameworks = macos_frameworks,
        .packages = packages,
        .target_manifests = std::move(target_manifests),
        .build_script = build_script,
    };
}

std::string CLI::unwrap_json_string(const std::string &name, const json::Value &value) {
    if (value.is_string()) {
        return value.as_string();
    } else {
        error("failed to load manifest: '" + name + "' expected to be a string");
    }
}

std::vector<std::string> CLI::unwrap_json_string_array(const std::string &name, const json::Value &value) {
    std::vector<std::string> values;
    bool valid = true;

    if (value.is_array()) {
        for (const json::Value &member : value.as_array()) {
            if (member.is_string()) {
                values.push_back(member.as_string());
            } else {
                valid = false;
                break;
            }
        }
    } else {
        valid = false;
    }

    if (!valid) {
        error("failed to load manifest: '" + name + "' expected to be a string array");
    } else {
        return values;
    }
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

    Target target;

    if (components.size() == 2) {
        const std::string &arch = components[0];
        const std::string &os = components[1];
        target = Target{arch, os, Target::get_default_env(os)};
    } else if (components.size() == 3) {
        const std::string &arch = components[0];
        const std::string &os = components[1];
        const std::string &env = components[2];
        target = Target{arch, os, env};
    } else {
        error("invalid target '" + std::string(string) + "'");
    }

    if (!utils::contains(Target::list_available(), target)) {
        error("target '" + std::string(string) + "' is not supported");
    }

    return target;
}

void CLI::install_package(std::string_view package) {
    std::filesystem::path packages_path("packages");

    if (std::filesystem::is_directory(packages_path / package)) {
        return;
    }

    single_line_output = false;
    print_step("Installing package '" + std::string(package) + "'...");

    run_utility_script("install_package.py", {std::string(package), packages_path.string()});
}

void CLI::run_build_script(const std::filesystem::path &path) {
    Command command{
        .executable = get_python_executable(),
        .args{path.string()},
        .stdin_stream = Command::Stream::INHERIT,
        .stdout_stream = Command::Stream::INHERIT,
        .stderr_stream = Command::Stream::INHERIT,
    };

    print_command("build script", command);

    std::optional<Process> process = Process::spawn(command);
    process->wait();
}

void CLI::build() {
    invoke_compiler();

    if (force_assembler) {
        invoke_assembler();
    }

    invoke_linker();

    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    std::chrono::duration duration = end_time - start_time;
    unsigned duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    float duration_s = static_cast<float>(duration_ms) / 1000.0f;

    if (!single_line_output) {
        std::ostringstream string_stream;
        string_stream << std::fixed << std::setprecision(2) << duration_s;
        print_step("Build finished (" + string_stream.str() + " seconds)");
    }
}

ProcessResult CLI::invoke_compiler() {
    print_step("Compiling...");

    std::vector<std::string> args;
    append_compilation_args(args);

    if (force_assembler) {
        args.push_back("--force-asm");
    }

    if (hot_reloading_enabled) {
        args.push_back("--hot-reload");
    }

    Command command{
        .executable = "banjo-compiler",
        .args = args,
    };

    print_command("compiler", command);

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();

    if (!result.stderr_buffer.empty()) {
        print_empty_line();
        std::cerr << result.stderr_buffer;
        compiler_output_printed = true;
    }

    if (result.exit_code != 0) {
        std::exit(1);
    }

    return result;
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
    bool is_msvc = target.env == "msvc";
    std::string asm_file_name = is_msvc ? "output.asm" : "output.s";
    std::string obj_file_name = is_msvc ? "output.obj" : "output.o";

    std::vector<std::string> args;

    if (target.os == "windows") {
        args.push_back("-fwin64");
    } else if (target.os == "linux") {
        args.push_back("-felf64");
    }

    args.push_back(asm_file_name);
    args.push_back("-o");
    args.push_back(obj_file_name);

    Command command{
        .executable = "nasm",
        .args = args,
    };

    print_command("assembler", command);

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();
    process_tool_result("assembler", result);

    std::filesystem::remove(asm_file_name);
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

    args.push_back("output.s");

    Command command{
        .executable = "clang",
        .args = args,
    };

    print_command("assembler", command);

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();
    process_tool_result("assembler", result);

    std::filesystem::remove("output.s");
}

void CLI::invoke_linker() {
    print_step("Linking...");
    std::filesystem::create_directories(get_output_dir());

    switch (toolchain_kind()) {
        case ToolchainKind::MSVC: invoke_msvc_linker(); break;
        case ToolchainKind::MINGW: invoke_mingw_linker(); break;
        case ToolchainKind::UNIX: invoke_unix_linker(); break;
        case ToolchainKind::MACOS: invoke_darwin_linker(); break;
        case ToolchainKind::EMSCRIPTEN: invoke_emscripten_linker(); break;
        case ToolchainKind::WASM: invoke_wasm_linker(); break;
    }
}

void CLI::invoke_msvc_linker() {
    MSVCToolchain &toolchain = *static_cast<MSVCToolchain *>(this->toolchain.get());

    std::filesystem::path msvc_tools_root_path(toolchain.tools_path);
    std::filesystem::path msvc_lib_root_path(toolchain.lib_path);

    std::filesystem::path msvc_tools_path = msvc_tools_root_path / "bin" / "Hostx64" / "x64";
    std::filesystem::path msvc_lib_path = msvc_tools_root_path / "lib" / "x64";
    std::filesystem::path msvc_um_lib_path = msvc_lib_root_path / "um" / "x64";
    std::filesystem::path msvc_ucrt_lib_path = msvc_lib_root_path / "ucrt" / "x64";

    std::vector<std::string> args;
    args.push_back("output.obj");
    args.push_back("/OUT:" + get_output_path());
    args.push_back("/LIBPATH:" + msvc_lib_path.string());
    args.push_back("/LIBPATH:" + msvc_um_lib_path.string());
    args.push_back("/LIBPATH:" + msvc_ucrt_lib_path.string());
    args.push_back("msvcrt.lib");
    args.push_back("kernel32.lib");
    args.push_back("user32.lib");
    args.push_back("gdi32.lib");
    args.push_back("shell32.lib");
    args.push_back("ws2_32.lib");
    args.push_back("dbghelp.lib");
    args.push_back("legacy_stdio_definitions.lib");
    args.push_back("/SUBSYSTEM:CONSOLE");
    args.push_back("/MACHINE:x64");
    args.push_back("/DEBUG:FULL");

    if (package_type == PackageType::SHARED_LIBRARY) {
        args.push_back("/DLL");
    }

    for (const std::string &library_path : library_paths) {
        args.push_back("/LIBPATH:" + library_path);
    }

    for (const std::string &library : libraries) {
        args.push_back(library + ".lib");
    }

    args.insert(args.end(), linker_args.begin(), linker_args.end());

    Command command{
        .executable = (msvc_tools_path / "link").string(),
        .args = args,
    };

    print_command("linker", command);

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();
    process_tool_result("linker", result, ToolErrorMessageSource::STDOUT);

    std::filesystem::remove("output.obj");
}

void CLI::invoke_mingw_linker() {
    MinGWToolchain &toolchain = *static_cast<MinGWToolchain *>(this->toolchain.get());

    std::filesystem::path linker_path(toolchain.linker_path);
    std::vector<std::string> lib_dirs = toolchain.lib_dirs;

    std::vector<std::string> args;
    args.push_back("output.o");
    args.push_back("-o");
    args.push_back(get_output_path());
    args.push_back("--subsystem");
    args.push_back("console");

    if (package_type == PackageType::SHARED_LIBRARY) {
        args.push_back("-shared");
    }

    for (const std::string &lib_dir : lib_dirs) {
        args.push_back("-L" + lib_dir);
    }

    args.push_back("-lmsvcrt");
    args.push_back("-lkernel32");
    args.push_back("-luser32");
    args.push_back("-lgdi32");
    args.push_back("-lshell32");
    args.push_back("-lws2_32");
    args.push_back("-ldbghelp");

    for (const std::string &library_path : library_paths) {
        args.push_back("-L" + library_path);
    }

    for (const std::string &library : libraries) {
        args.push_back("-l" + library);
    }

    args.insert(args.end(), linker_args.begin(), linker_args.end());

    Command command{
        .executable = linker_path.string(),
        .args = args,
    };

    print_command("linker", command);

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();
    process_tool_result("linker", result, ToolErrorMessageSource::STDERR);

    std::filesystem::remove("output.o");
}

void CLI::invoke_unix_linker() {
    UnixToolchain &toolchain = *static_cast<UnixToolchain *>(this->toolchain.get());

    std::filesystem::path linker_path(toolchain.linker_path);
    std::vector<std::string> &linker_args = toolchain.linker_args;
    std::vector<std::string> &additional_libraries = toolchain.extra_libs;
    std::vector<std::string> &lib_dirs = toolchain.lib_dirs;
    std::filesystem::path crt_dir(toolchain.crt_dir);

    // Notes about linking order:
    // First are crt1.o and crti.o from glibc, then crtbegin.o from GCC,
    // then the object file, then crtend.o from GCC, and finally crtn.o from
    // glibc. See: http://wiki.osdev.org/Calling_Global_Constructors
    // TODO: What about crtfastmath.o?

    std::vector<std::string> args;
    args.insert(args.end(), linker_args.begin(), linker_args.end());

    if (package_type == PackageType::EXECUTABLE) {
        args.push_back((crt_dir / "crt1.o").string());
        args.push_back((crt_dir / "crti.o").string());

        // TODO
        // args.push_back((crt_dir / "crtbegin.o").string());
    }

    args.push_back("output.o");

    if (package_type == PackageType::EXECUTABLE) {
        // TODO
        // args.push_back((crt_dir / "crtend.o").string());

        args.push_back((crt_dir / "crtn.o").string());
    }

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
    args.push_back("-lc_nonshared");

    for (const std::string &lib : additional_libraries) {
        args.push_back("-l" + lib);
    }

    args.push_back("--dynamic-linker");

    if (target.arch == "x86_64") {
        args.push_back("/lib64/ld-linux-x86-64.so.2");
    } else if (target.arch == "aarch64") {
        args.push_back("/lib/ld-linux-aarch64.so.1");
    }

    args.push_back("-z");
    args.push_back("noexecstack");

    if (package_type == PackageType::SHARED_LIBRARY) {
        args.push_back("-shared");
    }

    for (const std::string &library_path : library_paths) {
        args.push_back("-L" + library_path);
    }

    for (const std::string &library : libraries) {
        args.push_back("-l" + library);
    }

    args.insert(args.end(), this->linker_args.begin(), this->linker_args.end());

    Command command{
        .executable = linker_path.string(),
        .args = args,
    };

    print_command("linker", command);

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();
    process_tool_result("linker", result);

    std::filesystem::remove("output.o");
}

void CLI::invoke_darwin_linker() {
    MacOSToolchain &toolchain = *static_cast<MacOSToolchain *>(this->toolchain.get());

    std::filesystem::path linker_path(toolchain.linker_path);
    std::vector<std::string> &linker_args = toolchain.linker_args;
    std::filesystem::path sysroot_path(toolchain.sysroot_path);

    std::vector<std::string> args;
    args.insert(args.end(), linker_args.begin(), linker_args.end());
    args.push_back("output.o");
    args.push_back("-o");
    args.push_back(get_output_path());
    args.push_back("-arch");
    args.push_back("arm64");
    args.push_back("-platform_version");
    args.push_back("macos");
    args.push_back("14.0.0");
    args.push_back("14.0.0");
    args.push_back("-syslibroot");
    args.push_back(sysroot_path.string());
    args.push_back("-lSystem.B");
    args.push_back("-lobjc.A");

    if (package_type == PackageType::SHARED_LIBRARY) {
        args.push_back("-dylib");
    }

    for (const std::string &library_path : library_paths) {
        args.push_back("-L" + library_path);
    }

    for (const std::string &library : libraries) {
        args.push_back("-l" + library);
    }

    for (const std::string &framework : macos_frameworks) {
        args.push_back("-framework");
        args.push_back(framework);
    }

    args.insert(args.end(), this->linker_args.begin(), this->linker_args.end());

    Command command{
        .executable = linker_path.string(),
        .args = args,
    };

    print_command("linker", command);

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();
    process_tool_result("linker", result);

    std::filesystem::remove("output.o");
}

void CLI::invoke_wasm_linker() {
    WasmToolchain &toolchain = *static_cast<WasmToolchain *>(this->toolchain.get());

    std::filesystem::path linker_path(toolchain.linker_path);

    std::vector<std::string> args;
    args.push_back("output.o");
    args.push_back("-o");
    args.push_back(get_output_path());
    args.push_back("--no-entry");

    for (const std::string &library_path : library_paths) {
        args.push_back("-L" + library_path);
    }

    for (const std::string &library : libraries) {
        args.push_back("-l" + library);
    }

    args.insert(args.end(), linker_args.begin(), linker_args.end());

    Command command{
        .executable = linker_path.string(),
        .args = args,
    };

    print_command("linker", command);

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();
    process_tool_result("linker", result);

    std::filesystem::remove("output.o");
}

void CLI::invoke_emscripten_linker() {
    EmscriptenToolchain &toolchain = *static_cast<EmscriptenToolchain *>(this->toolchain.get());

    std::filesystem::path linker_path(toolchain.linker_path);

    std::vector<std::string> args;
    args.push_back("output.o");
    args.push_back("-o");
    args.push_back(get_output_path());
    args.push_back("-sSTACK_SIZE=1mb");
    args.push_back("-sALLOW_MEMORY_GROWTH=1");
    args.push_back("-sASSERTIONS");

    for (const std::string &library_path : library_paths) {
        args.push_back("-L" + library_path);
    }

    for (const std::string &library : libraries) {
        args.push_back("-l" + library);
    }

    args.insert(args.end(), linker_args.begin(), linker_args.end());

    Command command{
        .executable = linker_path.string(),
        .args = args,
    };

    print_command("linker", command);

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();
    process_tool_result("linker", result);

    std::filesystem::remove("output.o");
}

void CLI::run_build() {
    if (compiler_output_printed) {
        print_empty_line();
    }

    print_step("Running...");
    print_clear_line();

    std::optional<Process> process = Process::spawn(
        Command{
            .executable = get_output_path(),
            .stdin_stream = Command::Stream::INHERIT,
            .stdout_stream = Command::Stream::INHERIT,
            .stderr_stream = Command::Stream::INHERIT,
        }
    );

    process->wait();
}

void CLI::run_hot_reloader() {
    print_step("Running...");
    print_clear_line();

    std::vector<std::string> args;

    args.push_back("--executable");
    args.push_back(get_output_path());

    args.push_back("--dir");
    args.push_back("src");

    append_compilation_args(args);

    Command command{
        .executable = "banjo-hot-reloader",
        .args = args,
        .stdin_stream = Command::Stream::INHERIT,
        .stdout_stream = Command::Stream::INHERIT,
        .stderr_stream = Command::Stream::INHERIT,
    };

    print_command("hot reloader", command);

    std::optional<Process> process = Process::spawn(command);
    process->wait();
}

void CLI::append_compilation_args(std::vector<std::string> &args) {
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

    for (const std::string &path : source_paths) {
        args.push_back("--path");
        args.push_back(path);
    }

    for (const std::string &arg : extra_compiler_args) {
        args.push_back(arg);
    }
}

void CLI::process_tool_result(
    const std::string &tool_name,
    const ProcessResult &result,
    ToolErrorMessageSource error_message_source /*= ToolErrorMessageSource::STDERR*/
) {
    if (result.exit_code != 0) {
        print_empty_line();
        print_error(tool_name + " returned with exit code " + std::to_string(result.exit_code));

        if (error_message_source == ToolErrorMessageSource::STDOUT) {
            std::cerr << result.stdout_buffer;
        } else if (error_message_source == ToolErrorMessageSource::STDERR) {
            std::cerr << result.stderr_buffer;
        }

        print_empty_line();
        exit_error();
    }
}

std::filesystem::path CLI::get_toolchain_path() {
    return paths::toolchains_dir() / (target.to_string() + ".json");
}

std::string CLI::get_output_path() {
    std::string file_name;

    if (package_type == PackageType::EXECUTABLE) {
        std::string base = manifest.name;

        if (hot_reloading_enabled) {
            base += "-hot-reloadable";
        }

        if (target.os == "windows") {
            file_name = base + ".exe";
        } else {
            file_name = base;
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

    if (target.arch == "wasm") {
        if (target.os == "emscripten" && package_type == PackageType::EXECUTABLE) {
            file_name += ".html";
        } else {
            file_name += ".wasm";
        }
    }

    return (get_output_dir() / file_name).string();
}

std::filesystem::path CLI::get_output_dir() {
    std::string build_config_string;

    switch (build_config) {
        case BuildConfig::DEBUG: build_config_string = "debug"; break;
        case BuildConfig::RELEASE: build_config_string = "release"; break;
    }

    return std::filesystem::path("out") / (target.to_string() + "-" + build_config_string);
}

} // namespace banjo::cli
