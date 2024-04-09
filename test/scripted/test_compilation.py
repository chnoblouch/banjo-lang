import os
import fnmatch
import sys
import re
import subprocess
from pathlib import Path
import fnmatch


COMPILER_EXIT_CODE_PATTERN = re.compile(
    r"error: compiler returned with status code (\d+)", re.MULTILINE)
ERROR_PATTERN = re.compile(r"error: ([^\n\r]*)", re.MULTILINE)


class ProcessResult:

    def __init__(self, stdout, stderr, exit_code):
        self.stdout = stdout
        self.stderr = stderr
        self.exit_code = exit_code


def run_process(command):
    def decode_stream(stream, default):
        try:
            return stream.decode("utf-8").strip()
        except UnicodeDecodeError:
            return default

    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out = decode_stream(result.stdout, "<<no output>>")
    err = decode_stream(result.stderr, "<<no error>>")

    return ProcessResult(out, err, result.returncode)


class Test:

    CONFIG = "debug"

    @staticmethod
    def load_tests(directory, pattern):
        tests = []
        for file_name in os.listdir(directory):
            is_file = file_name.endswith(".bnj")
            if not is_file:
                continue
            
            name = file_name[:-4] if is_file else file_name
            
            if fnmatch.fnmatch(name, pattern):
                file_path = Path(directory, file_name)
                tests.extend(Test.load_tests_from_file(name, file_path))
                
        return tests

    @staticmethod
    def load_tests_from_file(name, path):
        tests = []
        current_test = None

        with open(path) as f:
            for line in f.readlines():
                if line.startswith("#! check_ok"):
                    tests.append(Test(name))
                    current_test = tests[-1]
                elif line.startswith("#! check_compiles"):
                    tests.append(Test(name))
                    current_test = tests[-1]
                    current_test.run_binary = False
                elif line.startswith("#! output: "):
                    tests.append(Test(name))
                    current_test = tests[-1]
                    current_test.output = line[11:].strip()
                elif line.startswith("#! error: "):
                    tests.append(Test(name))
                    current_test = tests[-1]
                    current_test.error = line[10:].strip()
                else:
                    if current_test is None:
                        tests.append(Test(name))
                        current_test = tests[-1]

                    current_test.source += line

        if len(tests) != 1:
            for i, test in enumerate(tests):
                test.name += "." + str(i)

        return tests

    def __init__(self, name):
        self.name = name
        self.source = ""
        self.output = None
        self.error = None
        self.run_binary = True
        
        self.passed = False
        self.message = "unknown error"

    @property
    def is_positive(self):
        return self.error is None

    def run(self):
        if self.is_positive:
            self._run_positive()
        else:
            self._run_negative()

    def _run_positive(self):
        build_proc = self._build()
        if build_proc.exit_code != 0:
            self.passed = False
            self.message = f"build system returned with exit code {build_proc.exit_code}"
            return

        if not self.run_binary:
            self.passed = True
            return
        
        run_proc = self._run()
        self._check_positive(run_proc)

    def _check_positive(self, result):
        if result.exit_code != 0:
            self.passed = False
            self.message = f"process returned with exit code {result.exit_code}"
            return

        if self.output is None or result.stdout == self.output:
            self.passed = True
        else:
            self.passed = False
            self.message = f"expected \'{self.output}\', got \'{result.stdout}\'"

    def _run_negative(self):
        build_proc = self._build()
        self._check_negative(build_proc)

    def _check_negative(self, result):
        if result.exit_code != 1:
            self.passed = False
            self.message = f"build process returned with exit code {result.exit_code}"
        else:
            match = COMPILER_EXIT_CODE_PATTERN.match(result.stderr)
            if match:
                compiler_exit_code = match.group(1)
                if compiler_exit_code != 1:
                    self.passed = False
                    self.message = f"compiler probably crashed, exit code {compiler_exit_code}"
                    return

            match = ERROR_PATTERN.search(result.stderr)
            if not match:
                self.passed = False
                self.message = f"no error message emitted"
                return

            error = match.group(1)
            if self.error == error:
                self.passed = True
            else:
                self.passed = False
                self.message = f"unexpected error message: {error}"

    def _build(self):
        with open("tmp.bnj", "w") as f:
            f.write(self.source)

        result = run_process([
            "banjo-compiler.exe",
            "--type", "executable",
            "--arch", "x86_64",
            "--os", "windows",
            "--env", "msvc",
            "--opt-level", "0",
            "--path", ".",
            "--no-color"
        ])

        os.remove("tmp.bnj")

        if os.path.isfile("main.obj"):
            if self.run_binary:
                subprocess.run([
                    "lld-link.exe",
                    "main.obj",
                    "../../lib/runtime/bin/x86_64-windows-msvc.lib",
                    "msvcrt.lib", "ws2_32.lib", "shlwapi.lib", "dbghelp.lib", "kernel32.lib", "user32.lib",
                    "/SUBSYSTEM:CONSOLE",
                    "/OUT:test.exe"
                ])

            os.remove("main.obj")

        return result

    def _run(self):
        result = run_process([f"./test.exe"])
        os.remove("test.exe")
        return result


if __name__ == "__main__":
    pattern = sys.argv[1] if len(sys.argv) >= 2 else "*"
    tests = Test.load_tests("compilation", pattern)

    if not tests:
        print("No match found.")
        exit(1)

    max_len = max([len(test.name) for test in tests])

    for test in tests:
        message = f"{test.name}{' ' * (max_len - len(test.name))}  "
        print(message, end="...", flush=True)

        test.run()

        if test.passed:
            print(f"\r{message}\u001b[32mOK \u001b[0m")
        else:
            print(f"\r{message}\u001b[31mFAILED ({test.message})\u001b[0m")

    tests_total = len(tests)
    tests_passed = len([test for test in tests if test.passed])

    print("-" * (max_len + 5))
    print("\u001b[32m" if tests_passed == tests_total else "\u001b[31m", end="")
    print(f"{tests_passed}/{tests_total} TESTS PASSED")
    print("\u001b[0m")
