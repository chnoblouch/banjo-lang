from dataclasses import dataclass
from pathlib import Path
from configuration import Configuration
from toolchain import Toolchain

import os
from typing import List


@dataclass
class LinkerInput:
    config: Configuration
    toolchain: Toolchain
    output_dir: str


class Linker:

    def build_command(self, input: LinkerInput):
        raise NotImplementedError()

    def cleanup(self, input: LinkerInput):
        raise NotImplementedError()


class WindowsLinker(Linker):

    def __init__(self, executable: str):
        self.executable = executable
    
    def build_command(self, input: LinkerInput):
        arch = input.toolchain.target.arch

        command = [self.executable, self.get_object_file(arch)]

        output_file_extension = "dll" if input.config.type == "shared_library" else "exe"
        output_file = input.config.name + "." + output_file_extension
        output_path = str(Path(input.output_dir) / output_file)
        command.append("/OUT:" + output_path)

        #command.extend(["msvcrt.lib", "ws2_32.lib", "shlwapi.lib"])
        command.extend([
            "msvcrt.lib",
            "kernel32.lib",
            "user32.lib",
            "legacy_stdio_definitions.lib",
            "ws2_32.lib",
            "shlwapi.lib",
            "dbghelp.lib",
        ])
        command.append("/SUBSYSTEM:" + input.config.win_subsystem)

        if arch == "x86_64":
            command.append("/MACHINE:x64")
        elif arch == "aarch64":
            command.append("/MACHINE:arm64")

        if input.config.build_config == "debug":
            command.append("/DEBUG:FULL")

        if input.config.type == "shared_library":
            command.append("/DLL")

        for library_path in input.config.library_paths:
            command.append("/LIBPATH:" + library_path)

        command.extend(input.config.linked_object_files)
        command.extend(lib + ".lib" for lib in input.config.libraries)
        command.extend(res + ".res" for res in input.config.linked_win_resources)

        return command

    def get_object_file(self, arch):        
        if arch == "x86_64":
            return "output.obj"
        elif arch == "aarch64":
            return "output.o"
        else:
            return None

    def cleanup(self, input: LinkerInput):
        os.remove(self.get_object_file(input.toolchain.target.arch))


class MinGWLinker(Linker):

    def __init__(self, executable: str, args: List[str], crt_dir: str, runtime_path: str):
        self.executable = executable
        self.args = args
        self.crt_dir = crt_dir
        self.runtime_path = runtime_path
    
    def build_command(self, input: LinkerInput):
        command = [self.executable] + self.args + ["output.o"]

        if input.config.type == "executable":
            output_file = f"{input.config.name}.exe"
        elif input.config.type == "shared_library":
            output_file = f"lib{input.config.name}.so"

        output_path = str(Path(input.output_dir) / output_file)
        command.extend(["-o", output_path])

        command.extend([
            "-lunwind",
            "-lmoldname",
            "-lmingwex",
            "-lmsvcrt",
            "-ladvapi32",
            "-lshell32",
            "-luser32",
            "-lkernel32",
            "-lmingw32",
            "-lws2_32",
            "-lshlwapi",
            "-ldbghelp",
        ])

        if input.config.type == "executable":
            command.extend([
                str(Path(self.crt_dir, "crt2.o")),
                str(Path(self.crt_dir, "crtbegin.o")),
                str(Path(self.crt_dir, "crtend.o")),
                self.runtime_path
            ])
        elif input.config.type == "shared_library":
            command.extend([
                "-shared",
                "-e", "DllMainCRTStartup",
                str(Path(self.crt_dir, "dllcrt2.o")),
                str(Path(self.crt_dir, "crtbegin.o")),
                str(Path(self.crt_dir, "crtend.o")),
                self.runtime_path
            ])

        for library_path in input.config.library_paths:
            command.append("-L" + library_path)

        for library in input.config.libraries:
            command.append("-l" + library)

        for linked_object_file in input.config.linked_object_files:
            command.append(linked_object_file)

        return command

    def cleanup(self, input: LinkerInput):
        os.remove("output.o")


class UnixLinker(Linker):

    def __init__(self, executable: str, args: List[str], crt_files: List[str]):
        self.executable = executable
        self.args = args
        self.crt_files = crt_files

    def build_command(self, input: LinkerInput):
        command = [self.executable] + self.args + ["output.o"]

        output_file = input.config.name
        if input.config.type == "shared_library":
            output_file = f"lib{output_file}.so"

        output_path = str(Path(input.output_dir) / output_file)
        command.extend(["-o", output_path])

        if input.config.type == "executable":
            command.extend(self.crt_files)
        elif input.config.type == "shared_library":
            command.append("-shared")

        for library_path in input.config.library_paths:
            command.append("-L" + library_path)

        for library in input.config.libraries:
            command.append("-l" + library)

        for linked_object_file in input.config.linked_object_files:
            command.append(linked_object_file)

        return command

    def cleanup(self, input: LinkerInput):
        os.remove("output.o")


class DarwinLinker(Linker):

    def __init__(self, executable, sysroot, extra_args):
        self.executable = executable
        self.sysroot = sysroot
        self.extra_args = extra_args

    def build_command(self, input: LinkerInput):
        arch = input.toolchain.target.arch

        command = [self.executable]
        command.extend(self.extra_args)
        command.extend(["output.o"])

        output_file = input.config.name
        if input.config.type == "shared_library":
            output_file = f"lib{output_file}.dylib"

        output_path = str(Path(input.output_dir) / output_file)
        command.extend(["-o", output_path])

        if arch == "x86_64":
            command.extend(["-arch", "x86_64"])
        elif arch == "aarch64":
            command.extend(["-arch", "arm64"])

        command.extend(["-platform_version", "macos", "14.0.0", "14.0.0"])
        command.extend(["-syslibroot", self.sysroot])
        command.extend(["-lSystem.B", "-lobjc.A"])

        if input.config.type == "shared_library":
            command.append("-dylib")

        for framework in input.config.macos_frameworks:
            command.extend(["-framework", framework])

        for library_path in input.config.library_paths:
            command.append("-L" + library_path)

        for library in input.config.libraries:
            command.append("-l" + library)

        return command

    def cleanup(self, input: LinkerInput):
        os.remove("output.o")
