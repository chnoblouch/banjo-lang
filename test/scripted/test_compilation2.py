import os
import fnmatch
import sys
import re
import subprocess
from pathlib import Path
import fnmatch


CONDITION_PREFIX = "# test:"


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
        result = self.build()

        if result.exit_code == 0:
            return TestResult(True)
        else:
            return TestResult(False, "unexpected exit code", 0, result.exit_code)

    def check_error(self, conditions):
        result = self.build()
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

    def build(self):
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
        return result

    def run_executable(self):
        self.build()

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
            "/OUT:test.exe"
        ])

        os.remove("main.obj")
        result = run_process(["./test.exe"])
        os.remove("test.exe")

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

    return tests


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
    pattern = sys.argv[1] if len(sys.argv) >= 2 else "*"
    
    tests = []

    for dir in os.listdir("compilation2"):
        tests += load_tests("compilation2/" + dir, dir)

    tests = [test for test in tests if fnmatch.fnmatch(test.name, pattern)]

    if not tests:
        print("no match found")
        exit(1)

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

    
