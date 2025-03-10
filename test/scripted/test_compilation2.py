import os
import fnmatch
import sys
import re
import subprocess
import platform
import argparse
from pathlib import Path


SKIPPED_TESTS = [
    "errors.expected_integer.0",
    "errors.expected_integer.1",
    "errors.expr_category.decl.3",
    "errors.expr_category.decl.7",
    "errors.expr_category.stmt.2",
    "errors.expr_category.type.4",
    "features.map_literal.0",
    "features.map_literal.1",
    "features.meta_if.8",
    "features.globals.3",
]


CONDITION_PREFIX = "# test:"


is_windows = platform.system() == "Windows"
is_linux = platform.system() == "Linux"
is_macos = platform.system() == "Darwin"

install_dir = None


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


class TestResult:

    def __init__(self, passed, failure_reason=None, expected=None, actual=None):
        self.passed = passed
        self.failure_reason = failure_reason
        self.expected = expected
        self.actual = actual


class Test:

    def __init__(self, name, source):
        self.name = name
        self.source = source

    def run(self):
        conditions = []
        
        for line in self.source.splitlines():
            if line.startswith(CONDITION_PREFIX):
                conditions.append(parse_condition(line))

        for condition, _ in conditions:
            if condition == "output":
                return self.check_output(conditions)
            elif condition == "exitcode":
                return self.check_exit_code(conditions)
            elif condition == "compiles":
                return self.check_compiles(conditions)
            elif condition == "error":
                return self.check_error(conditions)

    def check_output(self, conditions):
        for condition, args in conditions:
            if condition == "output":
                expected_output = args[0]
                break

        result = self.run_executable()

        if result.stdout == expected_output:
            return TestResult(True)
        else:
            return TestResult(False, "unexpected output", expected_output, result.stdout)

    def check_exit_code(self, conditions):
        for condition, args in conditions:
            if condition == "exitcode":
                expected_exit_code = args[0]
                break

        result = self.run_executable()

        if result.exit_code == expected_exit_code:
            return TestResult(True)
        else:
            return TestResult(False, "unexpected exit code", expected_exit_code, result.exit_code)

    def check_compiles(self, conditions):
        result = self.compile_source()

        if result.exit_code == 0:
            return TestResult(True)
        else:
            return TestResult(False, "unexpected exit code", 0, result.exit_code)

    def check_error(self, conditions):
        result = self.compile_source()
        stderr_lines = result.stderr.splitlines()

        expected_reports = []
        
        for condition, args in conditions:
            if condition == "error":
                expected_reports.append(("error", args[1], args[0]))
            elif condition == "note":
                expected_reports.append(("note", args[1], args[0]))

        reports = []

        def add_report(type, line):
            message = line[len(type) + 2:]
            location = ":".join(stderr_lines[i + 1][3:].split(":")[1:])
            reports.append((type, message, location))

        for i, line in enumerate(stderr_lines):
            if line.startswith("error: "):
                add_report("error", line)
            elif line.startswith("note: "):
                add_report("note", line)

        if len(reports) != len(expected_reports):
            return TestResult(False, "unexpected number of reports", len(expected_reports), len(reports)) 

        for i in range(0, len(reports)):
            type, message, location = reports[i]
            expected_type, expected_message, expected_location = expected_reports[i]

            if type != expected_type:
                return TestResult(False, "unexpected report type", expected_type, type)
            elif message != expected_message:
                return TestResult(False, "unexpected report message", f"\"{expected_message}\"", f"\"{message}\"")
            elif location != expected_location:
                return TestResult(False, "unexpected report location", expected_location, location)

        return TestResult(True)

    def compile_source(self):
        with open("tmp.bnj", "w") as f:
            f.write(self.source)

        if install_dir is None:
            compiler_executable = "banjo-compiler"
        else:
            compiler_executable = f"{install_dir}/bin/banjo-compiler"

        if is_windows:
            result = run_process([
                f"{compiler_executable}.exe",
                "--type", "executable",
                "--arch", "x86_64",
                "--os", "windows",
                "--env", "msvc",
                "--opt-level", "0",
                "--path", ".",
                "--no-color",
            ])
        elif is_linux:
            result = run_process([
                compiler_executable,
                "--type", "executable",
                "--arch", "x86_64",
                "--os", "linux",
                "--env", "gnu",
                "--opt-level", "0",
                "--path", ".",
                "--no-color",
            ])
        elif is_macos:
            result = run_process([
                compiler_executable,
                "--type", "executable",
                "--arch", "aarch64",
                "--os", "macos",
                "--opt-level", "0",
                "--path", ".",
                "--no-color",
            ])

        os.remove("tmp.bnj")
        return result

    def run_executable(self):
        self.compile_source()

        if is_windows:
            if not os.path.exists("main.obj"):
                return ProcessResult("", "", 1)
        
            subprocess.run([
                "lld-link.exe",
                "main.obj",
                "msvcrt.lib",
                "kernel32.lib",
                "user32.lib",
                "legacy_stdio_definitions.lib",
                "ws2_32.lib",
                "shlwapi.lib",
                "dbghelp.lib",
                "/SUBSYSTEM:CONSOLE",
                "/OUT:test.exe",
            ])

            os.remove("main.obj")

            if not os.path.exists("test.exe"):
                return ProcessResult("", "", 1)

            result = run_process(["./test.exe"])
            os.remove("test.exe")
        elif is_linux:
            if not os.path.exists("main.o"):
                return ProcessResult("", "", 1)

            subprocess.run(["clang", "-fuse-ld=lld", "-otest", "main.o"])
            os.remove("main.o")

            if not os.path.exists("test"):
                return ProcessResult("", "", 1)

            result = run_process(["./test"])
            os.remove("test")
        elif is_macos:
            if not os.path.exists("main.s"):
                return ProcessResult("", "", 1)

            subprocess.run(["clang", "-otest", "main.s"])
            os.remove("main.s")

            if not os.path.exists("test"):
                return ProcessResult("", "", 1)

            result = run_process(["./test"])
            os.remove("test")

        return result


def load_tests(directory, prefix):
    tests = []

    for file_name in os.listdir(directory):
        is_file = file_name.endswith(".bnj")
        if not is_file:
            continue
        
        name = file_name[:-4] if is_file else file_name
        file_path = Path(directory, file_name)
        tests.extend(load_test_file(f"{prefix}.{name}", file_path))
            
    return tests


def load_test_file(name, file_path):
    tests = [Test(name, "")]
    has_subtests = False
    
    with open(file_path) as f:
        for i, line in enumerate(f.readlines()):
            if line.startswith(CONDITION_PREFIX + "subtest"):
                if not has_subtests:
                    has_subtests = True
                else:
                    tests.append(Test(name, ""))
                
                tests[-1].name += f".{len(tests) - 1}"

        f.seek(0)

        subtest_index = None
        common = False

        for i, line in enumerate(f.readlines()):
            if line.startswith(CONDITION_PREFIX + "common"):
                common = True
            
            if line.startswith(CONDITION_PREFIX + "subtest"):
                if subtest_index is not None:
                    subtest_index += 1
                else:
                    subtest_index = 0

                common = False

            for j, test in enumerate(tests):
                if has_subtests:
                    enabled = common or j == subtest_index
                else:
                    enabled = True

                test.source += line if enabled else f"# DISABLED: {line}"

    return list(filter(lambda test: test.name not in SKIPPED_TESTS, tests))


def parse_condition(line):
    name = ""
    args = []
    index = len(CONDITION_PREFIX)

    def skip_whitespace():
        nonlocal index

        while index < len(line) and line[index] == " ":
            index += 1

    def scan_until(c):
        nonlocal index

        value = ""

        while index < len(line) and line[index] != c:
            value += line[index]
            index += 1

        return value

    name = scan_until(" ")
    skip_whitespace()

    while index < len(line):
        if line[index] == "\"":
            index += 1
            args.append(scan_until("\""))
            index += 1
        else:
            args.append(scan_until(" "))
            
        skip_whitespace()

    return name, tuple(args)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("pattern", nargs="?", default="*")
    parser.add_argument("--install-dir")
    args = parser.parse_args()

    install_dir = os.path.abspath(args.install_dir)
    os.chdir(os.path.dirname(__file__))

    tests = []

    for dir in os.listdir("compilation2"):
        tests += load_tests("compilation2/" + dir, dir)

    tests = [test for test in tests if fnmatch.fnmatch(test.name, args.pattern)]

    if not tests:
        print("no match found")
        sys.exit(0)

    print("\ntests:")

    max_len = max([len(test.name) for test in tests])
    failures = []

    for test in tests:
        print(f"  {test.name} {'.' * (max_len - len(test.name) + 3)} ", end="", flush=True)

        result = test.run()

        if result.passed:
            print(f"\u001b[32mok\u001b[0m")
        else:
            print(f"\u001b[31mfailed\u001b[0m")
            failures.append((test, result))

    tests_total = len(tests)
    tests_passed = len(tests) - len(failures)

    print()
    print("\u001b[32m" if tests_passed == tests_total else "\u001b[31m", end="")
    print(f"{tests_passed}/{tests_total} tests passed")
    print("\u001b[0m")

    if tests_passed != tests_total:
        print("failures:\n")

        for test, result in failures:
            print(f"  {test.name}: {result.failure_reason}")
            print(f"    expected: {result.expected}")
            print(f"      actual: {result.actual}\n")    

    sys.exit(0 if tests_passed == tests_total else 1)
