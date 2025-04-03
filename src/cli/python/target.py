import platform
from pathlib import Path
from typing import List
import os


class Target:
    def __init__(self, arch="unknown", os="unknown", env=None):
        self.arch = arch
        self.os = os
        self.env = env if env else Target.get_default_env(os)

    @staticmethod
    def from_host() -> 'Target':
        target = Target()

        machine = platform.machine().lower()
        if machine == "x86_64" or machine == "amd64":
            target.arch = "x86_64"
        elif machine == "aarch64" or machine == "arm64":
            target.arch = "aarch64"

        system = platform.system()
        if system == "Windows":
            target.os = "windows"
        elif system == "Linux":
            target.os = "linux"
        elif system == "Darwin":
            target.os = "macos"

        target.env = Target.get_default_env(target.os)
        return target

    @staticmethod
    def from_triple(triple) -> 'Target':
        components = triple.split("-")
        arch = components[0]
        os = components[1]
        env = components[2] if len(components) > 2 else Target.get_default_env(os)
        return Target(arch, os, env)

    @staticmethod
    def list_available() -> List['Target']:
        return [
            Target("x86_64", "windows", "msvc"),
            Target("x86_64", "windows", "gnu"),
            Target("x86_64", "linux", "gnu"),
            Target("x86_64", "macos"),
            Target("x86_64", "android"),
            Target("aarch64", "windows", "msvc"),
            Target("aarch64", "linux", "gnu"),
            Target("aarch64", "macos"),
            Target("aarch64", "android"),
            Target("aarch64", "ios")
        ]

    @property
    def triple(self):
        if self.env:
            return f"{self.arch}-{self.os}-{self.env}"
        else:
            return f"{self.arch}-{self.os}"

    @property
    def toolchain_file_path(self):
        return get_toolchains_dir() / (str(self) + ".json")

    @staticmethod
    def get_default_env(os):
        if os == "windows":
            return "msvc"
        elif os == "linux":
            return "gnu"
        else:
            return None

    def __str__(self):
        return self.triple

    def __eq__(self, other):
        return self.arch == other.arch and self.os == other.os and self.env == other.env
    
    def requires_assembler(self):
        return self not in (
            Target("x86_64", "windows", "msvc"),
            Target("x86_64", "windows", "gnu"),
            Target("x86_64", "linux", "gnu"),
            Target("aarch64", "macos"),
        )


def get_toolchains_dir():
    # if platform.system() == "Windows":
    #     toolchains_dir = Path(os.getenv("LOCALAPPDATA")) / "Programs" / "Banjo" / "toolchains"
    # else:
    #     toolchains_dir = Path.home() / "banjo" / "toolchains"

    toolchains_dir = Path.home() / ".banjo" / "toolchains"
    toolchains_dir.mkdir(parents=True, exist_ok=True)
    return toolchains_dir
