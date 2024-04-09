from linker import MinGWLinker
from toolchain import Toolchain
from toolchain_utils import locate_exec, run_detection_tool
from target import Target, get_toolchains_dir
import output

import subprocess
from pathlib import Path
import urllib.request
import zipfile
import tarfile
import os


LLVM_MINGW_VERSION = "20231128"


class MinGWToolchain(Toolchain):

    def __init__(self, target):
        super().__init__(target)

        self.keys = {
            "linker_path": "Linker",
            "lib_dirs": "Library search directories",
            "crt_dir": "CRT directory",
            "runtime_path": "Runtime path"
        }

        self.linker_path =  None
        self.linker_args = []
        self.lib_dirs = []
        self.crt_dir = None
        self.runtime_path = None

    def setup(self):
        output.print_step("Installing MinGW toolchain...")

        host = Target.from_host()

        if host.os == "windows":
            llvm_mingw_platform_suffix = host.arch
            llvm_mingw_archive_extension = "zip"
        elif host.os == "linux":
            llvm_mingw_platform_suffix = "ubuntu-20.04-" + host.arch
            llvm_mingw_archive_extension = "tar.xz"
        elif host.os == "macos":
            llvm_mingw_platform_suffix = "macos-universal"
            llvm_mingw_archive_extension = "tar.xz"

        llvm_mingw_name = f"llvm-mingw-{LLVM_MINGW_VERSION}-ucrt-{llvm_mingw_platform_suffix}"
        llvm_mingw_archive = f"{llvm_mingw_name}.{llvm_mingw_archive_extension}"
        
        llvm_mingw_url = "https://github.com/mstorsjo/llvm-mingw/releases/download/20231128/" + llvm_mingw_archive
        print("  llvm-mingw release URL: " + llvm_mingw_url)

        print("  Downloading llvm-mingw...")
        llvm_mingw_archive_path = get_toolchains_dir() / llvm_mingw_archive
        urllib.request.urlretrieve(llvm_mingw_url, llvm_mingw_archive_path)

        print("  Extracting...")
        if llvm_mingw_archive_extension == "zip":
            with zipfile.ZipFile(llvm_mingw_archive_path, "r") as zip_ref:
                zip_ref.extractall(get_toolchains_dir())
        elif llvm_mingw_archive_extension == "tar.xz":
            with tarfile.open(llvm_mingw_archive_path) as tar_ref:
                tar_ref.extractall(get_toolchains_dir())

        os.remove(llvm_mingw_archive_path)

        tools_dir = get_toolchains_dir() / llvm_mingw_name / "bin"

        if host.os == "windows":
            linker_path = str(tools_dir / "ld.lld")
        else:
            linker_path = str(tools_dir / "x86_64-w64-mingw32-ld")

        version = subprocess.check_output([linker_path, "--version"]).decode().strip()
        self.linker_path = linker_path

        output.print_step(f"  Found LLD: {linker_path}")
        output.print_step(f"  Version: {version}")

        compiler_path = str(tools_dir / "x86_64-w64-mingw32-clang")
        search_dirs = run_detection_tool([compiler_path, "--print-search-dirs"])
        line = next(line for line in search_dirs.splitlines() if line.startswith("libraries: "))

        lib_dirs_string = line[11:]
        if lib_dirs_string.startswith("="):
            lib_dirs_string = lib_dirs_string[1:]
        
        if host.os == "windows":
            self.lib_dirs = lib_dirs_string.split(";")
        else:
            self.lib_dirs = lib_dirs_string.split(":")

        # Resolve paths.
        self.lib_dirs = [str(Path(lib_dir).resolve()) for lib_dir in self.lib_dirs]
        
        # Remove duplicates.
        self.lib_dirs = list(set(self.lib_dirs))

        output.print_step(f"  Library directories:")
        for lib_dir in self.lib_dirs:
            output.print_step(f"    - {lib_dir}")

        for lib_dir in self.lib_dirs:
            if Path(lib_dir, "crt2.o").exists():
                self.crt_dir = lib_dir
                output.print_step(f"  CRT directory: {self.crt_dir}")

        for lib_dir in self.lib_dirs:
            runtime_path = Path(lib_dir, "lib", "windows", "libclang_rt.builtins-x86_64.a")
            if runtime_path.exists():
                self.runtime_path = str(runtime_path)
                output.print_step(f"  Found Clang runtime: {runtime_path}")

    def get_linker(self):
        args = self.linker_args + ["-L" + lib_dir for lib_dir in self.lib_dirs]
        return MinGWLinker(self.linker_path, args, self.crt_dir, self.runtime_path)