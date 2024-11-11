from linker import UnixLinker
from toolchain import Toolchain
from toolchain_utils import locate_exec, run_detection_tool
from target import get_toolchains_dir
import cloud_storage
import output

import subprocess
from pathlib import Path
import platform
import zipfile
import os


class LinuxToolchain(Toolchain):

    def __init__(self, target):
        super().__init__(target)

        self.keys = {
            "linker_path": "Linker",
            "linker_args": "Target-specific linker arguments",
            "additional_libraries": "Additional libraries",
            "lib_dirs": "Library search directories",
            "crt_dir": "CRT directory"
        }

        self.linker_path = None
        self.linker_args = []
        self.additional_libraries = []
        self.lib_dirs = []
        self.crt_dir = None

    def detect(self):
        output.print_step("Locating Linux toolchain...")

        path = locate_exec("ld.lld")
        version = subprocess.check_output([path, "--version"]).decode().strip()
        self.linker_path = path

        output.print_step(f"  Found LLD: {path}")
        output.print_step(f"  Version: {version}")

        search_dirs = run_detection_tool(["clang", "--print-search-dirs"])
        line = next(line for line in search_dirs.splitlines() if line.startswith("libraries: "))

        lib_dirs_string = line[11:]
        if lib_dirs_string.startswith("="):
            lib_dirs_string = lib_dirs_string[1:]
        
        self.lib_dirs = lib_dirs_string.split(":")

        # Resolve paths.
        self.lib_dirs = [str(Path(lib_dir).resolve()) for lib_dir in self.lib_dirs]
        
        # Remove duplicates.
        self.lib_dirs = list(set(self.lib_dirs))

        output.print_step(f"  Library directories:")
        for lib_dir in self.lib_dirs:
            output.print_step(f"    - {lib_dir}")

        for lib_dir in self.lib_dirs:
            if Path(lib_dir, "crt1.o").exists():
                self.crt_dir = lib_dir
                output.print_step(f"  CRT directory: {self.crt_dir}")
    
    def install(self):
        print("Installing Linux toolchain...")

        self.linker_path = locate_exec("lld")
        if self.linker_path:
            output.print_step(f"  Found LLD: {self.linker_path}")
            self.linker_args = ["-flavor", "ld"]
        else:
            output.print_step("  Failed to find LLD")
            return

        sysroot_path = self.get_sysroot_path()        
        if sysroot_path.is_dir():
            output.print_step(f"  Found existing sysroot: {sysroot_path}")
        else:
            output.print_step("  Downloading system libraries...")

            remote_path = f"toolchain/{self.get_sysroot_name()}.zip"
            local_path = get_toolchains_dir() / f"{self.get_sysroot_name()}.zip"
            cloud_storage.download(remote_path, local_path)
            
            output.print_step("  Extracting...")
            file = zipfile.ZipFile(local_path)
            file.extractall(get_toolchains_dir())
            file.close()
            os.remove(local_path)

        self.lib_dirs = [str(self.get_sysroot_path())]
        self.crt_dir = str(self.get_sysroot_path())
        self.additional_libraries = ["c_nonshared", "gcc"]

    def setup(self):
        if platform.system() == "Linux":
            self.detect()
        else:
            self.install()

    def get_sysroot_path(self):
        return get_toolchains_dir() / self.get_sysroot_name()

    def get_sysroot_name(self):
        return f"sysroot-{self.target.arch}-linux-gnu"

    def get_linker(self):
        if self.target.arch == "x86_64":
            dynamic_linker_path = "/lib64/ld-linux-x86-64.so.2"
        elif self.target.arch == "aarch64":
            dynamic_linker_path = "/lib/ld-linux-aarch64.so.1"

        args = self.linker_args + \
            ["-L" + lib_dir for lib_dir in self.lib_dirs] + \
            ["-lc", "-lgcc_s", "-lm", "-ldl", "-lpthread"] + \
            ["-l" + lib for lib in self.additional_libraries] + \
            ["--dynamic-linker", dynamic_linker_path]

        crt_files = [
            str(Path(self.crt_dir, "crt1.o")),
            str(Path(self.crt_dir, "crti.o")),
            str(Path(self.crt_dir, "crtn.o"))
        ]

        return UnixLinker(self.linker_path, args, crt_files)