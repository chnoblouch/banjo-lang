from toolchain import Toolchain
from mingw import MinGWToolchain
from linux import LinuxToolchain
from macos import MacosToolchain
from llvm import LlvmAndroidToolchain
from target import Target

import json
import platform

if platform.system() == "Windows":
    from msvc import MsvcToolchain


def load_from_file(filename, target) -> Toolchain:
    file = open(filename)
    json_root = json.load(file)
    file.close()

    toolchain = from_target(target)
    toolchain.load(json_root)
    return toolchain


def load_from_args(args) -> Toolchain:
    target = Target.from_triple(args.target)
    return from_target(target)


def from_host() -> Toolchain:
    target = Target.from_host()
    return from_target(target)


def from_target(target: Target) -> Toolchain:
    host = Target.from_host()

    if target.os == "windows":
        if target.env == "msvc":
            return MsvcToolchain(target)
        elif target.env == "gnu":
            return MinGWToolchain(target)
    elif target.os == "linux":
        return LinuxToolchain(target)
    elif target.os == "macos":
        return MacosToolchain(target)
    elif target.os == "android":
        return LlvmAndroidToolchain(target)
    elif target.os == "ios":
        return MacosToolchain(target)

    return None