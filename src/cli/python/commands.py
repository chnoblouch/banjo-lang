import os
import sys
import time
import subprocess
import platform
from fnmatch import fnmatch
from functools import partial

import configuration
import packages
import toolchain_loader
import output
import error

from tools import Compiler, Assembler, Linker, Archiver, Program, HotReloader
from linker import LinkerInput
from target import Target
from pathlib import Path


def load_setup(args):
    if hasattr(args, "target") and args.target:
        target = Target.from_triple(args.target)
    else:
        target = Target.from_host()

    if target not in Target.list_available():
        error.report_fatal(f"target '{target}' is not supported")

    cached_path = target.toolchain_file_path

    if os.path.isfile(cached_path):
        toolchain = toolchain_loader.load_from_file(cached_path, target)
    else:
        toolchain = toolchain_loader.from_target(target)

        output.single_line = False
        output.print_step(f"Setting up toolchain for: {target}")
        toolchain.setup()
        toolchain.store_in_file()

    validation = toolchain.validate()
    if validation:
        error.report_fatal(f"Toolchain error: {validation}")

    config = configuration.load(toolchain.target)
    config.build_config = getattr(args, "config", "debug")
    config.opt_level = getattr(args, "opt_level", "0")
    config.force_asm = getattr(args, "force_asm", False)
    return config, toolchain


def get_output_dir(config, toolchain, suffix=None):
    dir_name = f"{toolchain.target}-{config.build_config}"
    if suffix:
        dir_name = f"{dir_name}-{suffix}"

    output_dir = f"out{os.sep}{dir_name}"
    if not os.path.isdir(output_dir):
        os.makedirs(output_dir)
    return output_dir


def create_linker_input(config, toolchain):
    output_dir = get_output_dir(config, toolchain)
    return LinkerInput(config, toolchain, output_dir)


def do_build(config, toolchain, args):
    time_before = time.perf_counter()

    if config.build_script:
        python_executable = "python" if platform.system() == "Windows" else "python3"
        command = [python_executable, config.build_script]
        Program("build script", not output.quiet).run_command(command)

    packages.load(config, toolchain)

    output.print_step("Compiling...")

    extra_args = []
    extra_args.extend(args.compiler_args)

    if args.debug:
        extra_args.append("--debug")

    compiler = Compiler()
    compiler.extra_args = extra_args
    compiler.run(config, toolchain)

    use_assembler = config.force_asm or toolchain.target.requires_assembler()

    if use_assembler:
        Assembler().run(toolchain)

    if config.type == "executable" or config.type == "shared_library":
        output.print_step("Linking...")
        Linker().run(create_linker_input(config, toolchain))
    elif config.type == "static_library":
        output.print_step("Archiving...")
        Archiver().run(toolchain)

    if use_assembler:
        Assembler.cleanup(toolchain)

    time_after = time.perf_counter()
    time_between = time_after - time_before

    if not output.single_line:
        output.print_step("Build finished! (%.2f seconds)" % time_between)


def build(args):
    config, toolchain = load_setup(args)
    do_build(config, toolchain, args)


def run(args):
    config, toolchain = load_setup(args)
    if args.hot_reload:
        config.hot_reload = True

    do_build(config, toolchain, args)

    path = os.path.join(get_output_dir(config, toolchain), f"{config.name}")

    if not args.hot_reload:
        output.print_final_step("Running...")
        Program("build", True).run_command([path])
    else:
        HotReloader().run(config, toolchain, get_output_dir(config, toolchain))


def test(args):
    args.config = "debug"
    args.force_asm = False

    config, toolchain = load_setup(args)
    config.type = "executable"
    packages.load(config, toolchain)

    output.print_step("Compiling...")
    compiler = Compiler()
    compiler.extra_args = ["--testing"]
    compiler.run(config, toolchain)

    output.print_step("Linking...")
    linker_input = create_linker_input(config, toolchain)
    linker_input.output_dir = get_output_dir(config, toolchain, "test")
    Linker().run(linker_input)

    tests = compiler.stdout.splitlines()
    tests = list(filter(partial(fnmatch, pat=args.pattern), tests))

    if len(tests) == 0:
        print("No tests to run")
        return

    path = os.path.join(linker_input.output_dir, config.name)
    output.print_final_step("Running...")

    max_name_len = max([len(name) for name in tests])
    pass_count = 0
    failures = []

    print("\nTests:")

    for test in tests:
        num_spaces = max_name_len - len(test) + 3
        print("  " + test + " " + ("." * num_spaces) + " ", end="")

        result = subprocess.run([path, test], stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        if result.returncode == 0:
            print("\u001b[1;32mok\u001b[1;0m")
            pass_count += 1
        else:
            print("\u001b[1;31mfailed\u001b[1;0m")
            failures.append((test, result.stdout.decode().splitlines()[0]))

    if pass_count != len(tests):
        print("\nFailures:")

        for failure in failures:
            print(f"  {failure[0]}: {failure[1]}")

    print(f"\nPassed: {pass_count}/{len(tests)}\n")


def invoke(args):
    config, toolchain = load_setup(args)
    packages.load(config, toolchain)

    tool = args.tool
    if tool == "compiler":
        compiler = Compiler()
        compiler.extra_args = args.compiler_args
        compiler.run(config, toolchain)
    elif tool == "assembler":
        Assembler().run(toolchain)
    elif tool == "linker":
        Linker().run(create_linker_input(config, toolchain))


def toolchain(args):
    command = args.toolchain_command

    if command == "list":
        toolchains = []
        for target in Target.list_available():
            path = target.toolchain_file_path
            if os.path.isfile(path):
                toolchains.append(
                    toolchain_loader.load_from_file(path, target))

        print("\nToolchains:")

        for toolchain in toolchains:
            pairs = toolchain.store().items()
            print(f"\n  - Target {toolchain.target}")
            for key, value in pairs:
                print("    - " + toolchain.keys[key] + ": " + str(value))
        print()
    elif command == "add":
        target = Target.from_triple(args.target)
        toolchain = toolchain_loader.from_target(target)

        config = {}
        for key, name in toolchain.keys.items():
            print(f"{name}: ", end="")
            config[key] = input()

        toolchain.load(config)
        toolchain.store_in_file()
    elif command == "detect":
        target = Target.from_triple(args.target)
        toolchain = toolchain_loader.from_target(target)
        toolchain.detect()
        toolchain.store_in_file()
    elif command == "install":
        target = Target.from_triple(args.target)
        toolchain = toolchain_loader.from_target(target)
        toolchain.install()
        toolchain.store_in_file()
    elif command == "setup":
        target = Target.from_triple(args.target)
        toolchain = toolchain_loader.from_target(target)
        toolchain.setup()
        toolchain.store_in_file()


def new(args):
    package_name = args.name
    os.mkdir(package_name)
    os.chdir(package_name)
    os.mkdir("src")

    main_file = open("src/main.bnj", "w")
    main_file.write("func main() {\n\tprintln(\"Hello World!\");\n}")
    main_file.close()

    banjo_file = open("banjo.json", "w")
    banjo_file.write("{\n  \"name\": \"" + package_name + "\"\n}")
    banjo_file.close()

    os.chdir("../")


def targets(args):
    host_target = Target.from_host()

    print("\nAvailable targets: ")

    for target in Target.list_available():
        string = str(target)
        if target == host_target:
            string += " (host)"

        print("  - " + string)

    print()


def lsp(args):
    config, toolchain = load_setup(args)

    command = [
        "banjo-lsp",
        "--arch", toolchain.target.arch,
        "--os", toolchain.target.os,
    ]

    if toolchain.target.env:
        command += ["--env", toolchain.target.env]

    command += ["--path", "src/"]

    for path in config.paths:
        command.extend(["--path", path])

    command += config.arguments

    subprocess.run(command, text=False)


def bindgen(args):
    bindgen_path = Path(__file__).parents[2] / "scripts" / "bindgen"
    sys.path.append(str(bindgen_path))

    import bindgen as bindgen_main
    bindgen_main.run(args.file, args.generator, args.include_paths)
