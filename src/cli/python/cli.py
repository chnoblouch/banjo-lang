import argparse
from argparse import ArgumentParser

import commands
import configuration
import output

import os
import subprocess
import error


parser = ArgumentParser("banjo")
parser.add_argument("-v", "--version", action="store_true")

subparsers = parser.add_subparsers(dest="command")


def add_standard_args(parser):
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("-q", "--quiet", action="store_true")


def add_common_build_args(parser):
    parser.add_argument("--config", choices=["debug", "release"], default="debug")
    parser.add_argument("--opt-level", default=None)
    parser.add_argument("--force-asm", action="store_true")
    parser.add_argument("--debug", action="store_true")
    parser.add_argument("compiler_args", nargs=argparse.REMAINDER)


build_parser = subparsers.add_parser("build")
add_common_build_args(build_parser)
build_parser.add_argument("--target", default=None)
add_standard_args(build_parser)

run_parser = subparsers.add_parser("run")
add_common_build_args(run_parser)
run_parser.add_argument("--hot-reload", action="store_true")
add_standard_args(run_parser)

test_parser = subparsers.add_parser("test")
test_parser.add_argument("pattern", nargs=argparse.OPTIONAL, default="*")
test_parser.add_argument("compiler_args", nargs=argparse.REMAINDER)
add_common_build_args(test_parser)
add_standard_args(test_parser)

invoke_parser = subparsers.add_parser("invoke")
invoke_parser.add_argument("tool", choices=["compiler", "assembler", "linker"])
add_common_build_args(invoke_parser)
invoke_parser.add_argument("--target", default=None)
add_standard_args(invoke_parser)

toolchain_parser = subparsers.add_parser("toolchain")

toolchain_subparsers = toolchain_parser.add_subparsers(
    dest="toolchain_command")

toolchain_list_parser = toolchain_subparsers.add_parser(
    "list", help="List cached toolchains")

toolchain_add_parser = toolchain_subparsers.add_parser(
    "add", help="Create a new toolchain from config")
toolchain_add_parser.add_argument("target")

toolchain_detect_parser = toolchain_subparsers.add_parser(
    "detect", help="Try to detect a toolchain on the system")
toolchain_detect_parser.add_argument("target")
add_standard_args(toolchain_detect_parser)

toolchain_install_parser = toolchain_subparsers.add_parser(
    "install", help="Download and install a toolchain")
toolchain_install_parser.add_argument("target")
add_standard_args(toolchain_install_parser)

toolchain_setup_parser = toolchain_subparsers.add_parser(
    "setup", help="Set up a toolchain by either detecting or installing it")
toolchain_setup_parser.add_argument("target")
add_standard_args(toolchain_setup_parser)

new_parser = subparsers.add_parser("new")
new_parser.add_argument("name")

targets_parser = subparsers.add_parser("targets")

bindgen_parser = subparsers.add_parser("bindgen")
bindgen_parser.add_argument("file")
bindgen_parser.add_argument("--generator", default=None)
bindgen_parser.add_argument("-I", action="append",
                            default=[], dest="include_paths")

args = parser.parse_args()
if args.version:
    subprocess.run(["banjo-compiler", "--version"])
    exit(1)

command = args.command

output.verbose = hasattr(args, "verbose") and args.verbose
output.quiet = hasattr(args, "quiet") and args.quiet

if command not in ["toolchain", "new", "targets", "bindgen"] and not os.path.isfile(configuration.FILENAME):
    error.report_fatal("not a Banjo project")

try:
    if command == "build":
        commands.build(args)
    elif command == "run":
        commands.run(args)
    elif command == "test":
        commands.test(args)
    elif command == "invoke":
        commands.invoke(args)
    elif command == "toolchain":
        commands.toolchain(args)
    elif command == "new":
        commands.new(args)
    elif command == "targets":
        commands.targets(args)
    elif command == "bindgen":
        commands.bindgen(args)
except KeyboardInterrupt:
    exit(0)
