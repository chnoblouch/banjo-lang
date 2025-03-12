import subprocess
import sys
import platform
import os

from configuration import BuildType
from linker import LinkerInput
import output


class Program:

    def __init__(self, name, show_output=False):
        self.name = name
        self.show_output = show_output

    def run_command(self, command):
        output.print_command(self.name, command)

        try:
            proc_out = sys.stdout if self.show_output else subprocess.PIPE
            proc_err = sys.stderr if self.show_output else subprocess.PIPE
            child = subprocess.Popen(command, stdout=proc_out, stderr=proc_err)
            out, err = child.communicate()

            if child.returncode != 0:
                if output.verbose:
                    print(32 * "-")

                print(
                    f"{self.name} returned with status code {child.returncode}", file=sys.stderr)
                if out:
                    print(out.decode("utf-8"), file=sys.stderr, end="")
                if err:
                    print(err.decode("utf-8"), file=sys.stderr, end="")

                if output.verbose:
                    print(32 * "-")

                exit(1)
        except Exception as e:
            print(f"failed to run {self.name}: {e}")
            exit(1)


class Compiler:

    def __init__(self):
        self.extra_args = []
        self.stdout = ""
        self.stderr = ""

    def run(self, config, toolchain):
        if config.opt_level is None:
            opt_level = "0" if config.build_config == "debug" else "1"
        else:
            opt_level = config.opt_level

        command = [
            "banjo-compiler",
            "--type", config.type,
            "--arch", toolchain.target.arch,
            "--os", toolchain.target.os,
        ]

        if toolchain.target.env:
            command.extend(["--env", toolchain.target.env])

        command.extend([
            "--opt-level", opt_level,
            "--path", "src/",
        ])

        if config.type == BuildType.SHARED_LIBRARY:
            command.append("--pic")

        if config.hot_reload:
            command.append("--hot-reload")

        for path in config.paths:
            command.extend(["--path", path])

        if config.force_asm:
            command.extend(["--force-asm"])

        command.extend(config.arguments)
        command.extend(self.extra_args)

        output.print_command("compiler", command)

        result = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )

        self.stdout = result.stdout.decode()
        self.stderr = result.stderr.decode()

        if output.verbose:
            print(self.stdout, end="")

        if result.returncode != 0:
            print()

            if not output.quiet:
                print("" if result.returncode == 1 else f"compiler returned with status code {result.returncode}")

            print(self.stderr, file=sys.stderr, end="")
            exit(1)
        elif len(self.stderr) > 0:
            print("\x1b[2m\x1b[2K\r")
            print(self.stderr, file=sys.stderr, end="")
            print()


class Assembler(Program):

    def __init__(self):
        super().__init__("assembler")

    def run(self, toolchain):
        if toolchain.target.arch == "x86_64":
            self.run_nasm(toolchain)
        elif toolchain.target.arch == "aarch64":
            self.run_clang(toolchain)

    def run_nasm(self, toolchain):
        nasm_format = None

        if toolchain.target.os == "windows":
            nasm_format = "win64"
        elif toolchain.target.os == "linux":
            nasm_format = "elf64"
        elif toolchain.target.os == "macos":
            nasm_format = "macho64"

        self.run_command(["nasm", "-f", nasm_format, "main.asm"])

    def run_clang(self, toolchain):
        clang_target = f"{toolchain.target.arch}-{toolchain.target.os}"
        self.run_command(["clang", "-c", "-target", clang_target, "main.s"])

    @staticmethod
    def cleanup(toolchain):
        if toolchain.target.arch == "x86_64":
            os.remove("main.asm")
        elif toolchain.target.arch == "aarch64":
            os.remove("main.s")


class Linker(Program):

    def __init__(self):
        super().__init__("linker")

    def run(self, input: LinkerInput):
        linker = input.toolchain.get_linker()
        command = linker.build_command(input)
        self.run_command(command)
        linker.cleanup(input)


class Archiver(Program):

    def __init__(self):
        super().__init__("archiver")

    def run(self, toolchain):
        command = ["lib", "main.obj", "/OUT:build.lib"]
        self.run_command(command)
        os.remove("main.obj")


class HotReloader(Program):

    def __init__(self):
        super().__init__("hot reloader", True)

    def run(self, config, toolchain, output_dir):
        opt_level = "0" if config.build_config == "debug" else "1"

        executable = f"{output_dir}{os.sep}{config.name}"
        if toolchain.target.os == "windows":
            executable += ".exe"

        command = [
            "banjo-hot-reloader",
            "--executable", executable,
            "--dir", "src/",
            "--type", config.type,
            "--arch", toolchain.target.arch,
            "--os", toolchain.target.os,
            "--opt-level", opt_level,
            "--path", "src/"
        ]

        if config.type == BuildType.SHARED_LIBRARY:
            command.append("--pic")

        for path in config.paths:
            command.extend(["--path", path])

        if config.force_asm:
            command.extend(["--force-asm"])

        command.extend(config.arguments)

        self.run_command(command)
