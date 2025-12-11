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
#include <sstream>
#include <string_view>
#include <utility>
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

static const ArgumentParser::Positional POSITIONAL_NAME{
    "name",
};

static const ArgumentParser::Positional POSITIONAL_TOOL{
    "tool",
};

static const ArgumentParser::Positional POSITIONAL_FORMAT_FILE{
    "file",
};

static const ArgumentParser::Positional POSITIONAL_BINDGEN_SOURCE{
    "file",
};

static const ArgumentParser::Command COMMAND_NEW{
    "new",
    "Create a new package in the current working directory",
    {
        OPTION_HELP,
    },
    {
        POSITIONAL_NAME,
    },
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
        OPTION_DEBUG_COMPILER,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_RUN{
    "run",
    "Build and run the current package",
    {
        OPTION_HELP,
        OPTION_HOT_RELOAD,
        OPTION_CONFIG,
        OPTION_OPT_LEVEL,
        OPTION_FORCE_ASM,
        OPTION_DEBUG_COMPILER,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_TEST{
    "test",
    "Build and run tests of the current package",
    {
        OPTION_HELP,
        OPTION_CONFIG,
        OPTION_OPT_LEVEL,
        OPTION_FORCE_ASM,
        OPTION_DEBUG_COMPILER,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_INVOKE{
    "invoke",
    "Run a program from the toolchain (compiler, assembler, or linker)",
    {
        OPTION_HELP,
        OPTION_TARGET,
        OPTION_CONFIG,
        OPTION_OPT_LEVEL,
        OPTION_FORCE_ASM,
        OPTION_DEBUG_COMPILER,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
    {
        POSITIONAL_TOOL,
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

static const ArgumentParser::Command COMMAND_LSP{
    "lsp",
    "Launch the language server",
    {
        OPTION_HELP,
        OPTION_TARGET,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
};

static const ArgumentParser::Command COMMAND_FORMAT{
    "format",
    "Format source files",
    {
        OPTION_HELP,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
    {
        POSITIONAL_FORMAT_FILE,
    }
};

static const ArgumentParser::Command COMMAND_BINDGEN{
    "bindgen",
    "Generate bindings to C libraries",
    {
        OPTION_HELP,
        OPTION_BINDGEN_GENERATOR,
        OPTION_BINDGEN_INCLUDE_PATH,
        OPTION_QUIET,
        OPTION_VERBOSE,
    },
    {
        POSITIONAL_BINDGEN_SOURCE,
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
        .commands{
            COMMAND_NEW,
            COMMAND_BUILD,
            COMMAND_RUN,
            COMMAND_TEST,
            COMMAND_INVOKE,
            COMMAND_TARGETS,
            COMMAND_LSP,
            COMMAND_FORMAT,
            COMMAND_BINDGEN,
            COMMAND_TOOLCHAINS,
            COMMAND_HELP,
            COMMAND_VERSION,
        },
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
            single_line_output = false;
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
        } else if (name == OPTION_HOT_RELOAD.name) {
            hot_reloading_enabled = true;
        } else if (name == OPTION_DEBUG_COMPILER.name) {
            extra_compiler_args.push_back("--debug");
        } else if (name == OPTION_HELP.name) {
            arg_parser.print_command_help(*args.command);
            return;
        } else if (name == OPTION_QUIET.name) {
            quiet = true;
        } else if (name == OPTION_VERBOSE.name) {
            verbose = true;
            single_line_output = false;
        }
    }

    if (Utils::is_one_of(args.command->name, {COMMAND_BUILD.name, COMMAND_RUN.name})) {
        start_time = std::chrono::steady_clock::now();
    }

    if (args.command->name == COMMAND_TARGETS.name) {
        execute_targets();
    } else if (args.command->name == COMMAND_TOOLCHAINS.name) {
        execute_toolchains();
    } else if (args.command->name == COMMAND_NEW.name) {
        execute_new(args);
    } else if (args.command->name == COMMAND_BUILD.name) {
        execute_build();
    } else if (args.command->name == COMMAND_RUN.name) {
        execute_run();
    } else if (args.command->name == COMMAND_TEST.name) {
        execute_test();
    } else if (args.command->name == COMMAND_INVOKE.name) {
        execute_invoke(args);
    } else if (args.command->name == COMMAND_LSP.name) {
        execute_lsp();
    } else if (args.command->name == COMMAND_FORMAT.name) {
        execute_format(args);
    } else if (args.command->name == COMMAND_BINDGEN.name) {
        execute_bindgen(args);
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

    std::filesystem::path toolchains_dir = paths::toolchains_dir();

    if (!std::filesystem::is_directory(toolchains_dir)) {
        std::cout << "\n";
        return;
    }

    for (std::filesystem::path path : std::filesystem::directory_iterator(toolchains_dir)) {
        if (std::filesystem::is_regular_file(path) && path.extension() == ".json") {
            std::cout << "  - " << path.filename().string() << "\n";
        }
    }

    std::cout << "\n";
}

void CLI::execute_version() {
    std::cout << BANJO_VERSION << "\n";
}

void CLI::execute_new(const ArgumentParser::Result &args) {
    if (args.command_positionals.empty()) {
        error("missing positional argument 'new'");
        return;
    }

    const std::string &name = args.command_positionals[0];
    std::filesystem::path package_path = name;
    std::filesystem::path src_path = package_path / "src";
    std::filesystem::path main_path = src_path / "main.bnj";
    std::filesystem::path manifest_path = package_path / "banjo.json";

    std::filesystem::create_directory(package_path);
    std::filesystem::create_directory(src_path);

    Utils::write_string_file("func main() {\n    println(\"Hello, World!\");\n}\n", main_path);
    Utils::write_string_file("{\n  \"name\": \"" + name + "\",\n  \"type\": \"executable\"\n}\n", manifest_path);
}

void CLI::execute_build() {
    load_config();
    build();
    print_clear_line();
}

void CLI::execute_run() {
    load_config();
    build();

    if (hot_reloading_enabled) {
        run_hot_reloader();
    } else {
        run_build();
    }
}

void CLI::execute_test() {
    load_config();

    extra_compiler_args.push_back("--testing");
    package_type = PackageType::EXECUTABLE;

    ProcessResult compiler_result = invoke_compiler();

    if (force_assembler) {
        invoke_assembler();
    }

    invoke_linker();
    print_empty_line();

    std::string tests_raw = Utils::convert_eol_to_lf(compiler_result.stdout_buffer);
    std::vector<std::string_view> tests = Utils::split_string(tests_raw, '\n');

    unsigned longest_name_length = 0;

    for (std::string_view test : tests) {
        longest_name_length = std::max(longest_name_length, static_cast<unsigned>(test.size()));
    }

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
    if (args.command_positionals.empty()) {
        error("missing positional argument 'tool'");
        return;
    }

    const std::string &tool = args.command_positionals[0];

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
    std::string path = args.command_positionals[0];

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
    process->wait();
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
        if (value.option->name == OPTION_BINDGEN_GENERATOR.name) {
            bindgen_args.push_back("--generator");
            bindgen_args.push_back(*value.value);
        } else if (value.option->name == OPTION_BINDGEN_INCLUDE_PATH.name) {
            bindgen_args.push_back("-I");
            bindgen_args.push_back(*value.value);
        }
    }

    bindgen_args.push_back(args.command_positionals[0]);

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
    std::filesystem::path toolchain_path = get_toolchain_path();

    if (std::filesystem::exists(toolchain_path)) {
        std::optional<std::string> toolchain_string = Utils::read_string_file(toolchain_path);

        if (!toolchain_string) {
            error("failed to load cached toolchain file for target " + target.to_string());
        }

        toolchain = Toolchain{
            .properties = JSONParser(*toolchain_string).parse_object(),
        };
    } else {
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
            load_manifest(sub_manifest);
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

void CLI::set_up_toolchain() {
    single_line_output = false;

    print_step("Setting up toolchain for target " + target.to_string());

    Target host = Target::host();

    if (target.os == "windows") {
        if (target.env == "msvc") {
            toolchain.properties = MSVCToolchain::detect().serialize();
        } else if (target.env == "gnu") {
            toolchain.properties = MinGWToolchain::detect().serialize();
        } else {
            ASSERT_UNREACHABLE;
        }
    } else if (target.os == "linux") {
        if (host.os == "linux") {
            toolchain.properties = UnixToolchain::detect().serialize();
        } else {
            toolchain.properties = UnixToolchain::install(target.arch).serialize();
        }
    } else if (target.os == "macos") {
        toolchain.properties = MacOSToolchain::detect().serialize();
    } else if (target.arch == "wasm") {
        if (target.os == "emscripten") {
            toolchain.properties = EmscriptenToolchain::detect().serialize();
        } else {
            toolchain.properties = WasmToolchain::detect().serialize();
        }
    } else {
        ASSERT_UNREACHABLE;
    }

    print_step("  Caching toolchain...");

    std::filesystem::path toolchain_path = get_toolchain_path();
    std::filesystem::create_directories(toolchain_path.parent_path());

    std::ofstream toolchain_stream(toolchain_path, std::ios::binary);
    JSONSerializer(toolchain_stream).serialize(toolchain.properties);
}

Manifest CLI::parse_manifest(const std::filesystem::path &path) {
    if (!std::filesystem::exists(path)) {
        error("could not find manifest at '" + path.string() + "'");
    }

    if (std::optional<Manifest> manifest = try_parse_manifest(path)) {
        return *manifest;
    } else {
        error("could not open manifest at '" + path.string() + "'");
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

    JSONObject json = JSONParser(buffer).parse_object();
    return parse_manifest(json);
}

Manifest CLI::parse_manifest(const JSONObject &json) {
    std::optional<std::string> name;
    std::string type = "executable";
    std::vector<std::string> args;
    std::vector<std::string> linker_args;
    std::vector<std::string> libraries;
    std::vector<std::string> macos_frameworks;
    std::vector<std::string> packages;
    std::vector<std::pair<Target, Manifest>> target_manifests;
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
            // TODO
        } else if (member_name == "packages") {
            packages = unwrap_json_string_array(member_name, member_value);
        } else if (member_name == "targets") {
            // TODO: Error handling

            for (const auto &[target_string, json_properties] : member_value.as_object()) {
                Target target = parse_target(target_string);
                Manifest target_manifest = parse_manifest(json_properties.as_object());
                target_manifests.push_back({target, target_manifest});
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
        .target_manifests = target_manifests,
        .build_script = build_script,
    };
}

std::string CLI::unwrap_json_string(const std::string &name, const JSONValue &value) {
    if (value.is_string()) {
        return value.as_string();
    } else {
        error("failed to load manifest: '" + name + "' expected to be a string");
    }
}

std::vector<std::string> CLI::unwrap_json_string_array(const std::string &name, const JSONValue &value) {
    std::vector<std::string> values;
    bool valid = true;

    if (value.is_array()) {
        for (const JSONValue &member : value.as_array()) {
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

    if (!Utils::contains(Target::list_available(), target)) {
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

    if (result.exit_code != 0) {
        print_empty_line();
        std::cerr << result.stderr_buffer;
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

    if (target.os == "windows") {
        if (target.env == "msvc") {
            invoke_msvc_linker();
        } else if (target.env == "gnu") {
            invoke_mingw_linker();
        } else {
            ASSERT_UNREACHABLE;
        }
    } else if (target.os == "linux") {
        invoke_unix_linker();
    } else if (target.os == "macos") {
        invoke_darwin_linker();
    } else if (target.arch == "wasm") {
        if (target.os == "emscripten") {
            invoke_emscripten_linker();
        } else {
            invoke_wasm_linker();
        }
    } else {
        ASSERT_UNREACHABLE;
    }
}

void CLI::invoke_msvc_linker() {
    std::filesystem::path msvc_tools_root_path(toolchain.properties.get_string("tools"));
    std::filesystem::path msvc_lib_root_path(toolchain.properties.get_string("lib"));

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
    args.push_back("shlwapi.lib");
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
    std::filesystem::path linker_path(toolchain.properties.get_string("linker_path"));
    std::vector<std::string> lib_dirs = toolchain.properties.get_string_array("lib_dirs");

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
    args.push_back("-lshlwapi");
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
    std::filesystem::path linker_path(toolchain.properties.get_string("linker_path"));
    std::vector<std::string> linker_args = toolchain.properties.get_string_array("linker_args");
    std::vector<std::string> additional_libraries = toolchain.properties.get_string_array("additional_libraries");
    std::vector<std::string> lib_dirs = toolchain.properties.get_string_array("lib_dirs");
    std::filesystem::path crt_dir(toolchain.properties.get_string("crt_dir"));

    std::vector<std::string> args;
    args.insert(args.end(), linker_args.begin(), linker_args.end());
    args.push_back("output.o");
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

void CLI::invoke_darwin_linker() {
    std::filesystem::path linker_path(toolchain.properties.get_string("linker_path"));
    std::vector<std::string> linker_args = toolchain.properties.get_string_array("extra_args");
    std::filesystem::path sysroot_path(toolchain.properties.get_string("sysroot"));

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

    args.insert(args.end(), linker_args.begin(), linker_args.end());

    Command command{
        .executable = linker_path,
        .args = args,
    };

    print_command("linker", command);

    std::optional<Process> process = Process::spawn(command);
    ProcessResult result = process->wait();
    process_tool_result("linker", result);

    std::filesystem::remove("output.o");
}

void CLI::invoke_wasm_linker() {
    std::filesystem::path linker_path(toolchain.properties.get_string("linker_path"));

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
    std::filesystem::path linker_path(toolchain.properties.get_string("linker_path"));

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
    return std::filesystem::path("out") / (target.to_string() + "-debug");
}

} // namespace cli
} // namespace banjo
