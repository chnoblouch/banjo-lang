import subprocess
import argparse
import os
import fnmatch
import sys
import platform
from pathlib import Path


CONDITION_PREFIX = "# test:"


install_dir = None
test_wasm = False


class ProcessResult:
    def __init__(self, stdout, stderr, exit_code):
        self.stdout = stdout
        self.stderr = stderr
        self.exit_code = exit_code


class Test:
    def __init__(self, name, source):
        self.name = name
        self.source = source
        self.sections = {}


class TestResult:
    def __init__(self, passed, failure_reason=None, expected=None, actual=None):
        self.passed = passed
        self.failure_reason = failure_reason
        self.expected = expected
        self.actual = actual


def run_tests(directory, file_name_extension, runner, filter_fn=lambda _: True):
    global install_dir, test_wasm

    parser = argparse.ArgumentParser()
    parser.add_argument("pattern", nargs="?", default="*")
    parser.add_argument("--install-dir")
    parser.add_argument("--wasm", action="store_true")
    args = parser.parse_args()

    install_dir = os.path.abspath(args.install_dir) if args.install_dir else None
    test_wasm = args.wasm

    os.chdir(os.path.dirname(__file__))

    tests = []

    for file_path in Path(directory).iterdir():
        if file_path.is_dir():
            for sub_path in file_path.iterdir():
                if sub_path.suffix != file_name_extension:
                    continue
                
                tests.extend(load_test_file(f"{file_path.stem}.{sub_path.stem}", sub_path))
        else:
            if file_path.suffix != file_name_extension:
                continue
                
            tests.extend(load_test_file(str(file_path.stem), file_path))

    tests = filter(filter_fn, tests)
    tests = [test for test in tests if fnmatch.fnmatch(test.name, args.pattern)]

    if not tests:
        print("no match found")
        return True

    print("\ntests:")

    max_len = max([len(test.name) for test in tests])
    failures = []

    for test in tests:
        print(f"  {test.name} {'.' * (max_len - len(test.name) + 3)} ", end="", flush=True)

        conditions = []

        for line in test.source.splitlines():
            if line.startswith(CONDITION_PREFIX):
                conditions.append(parse_condition(line))

        result = runner(test, conditions)

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
            is_string = type(result.expected) is str and type(result.actual) is str
            
            print(f"  {test.name}: {result.failure_reason}")
            
            if result.expected is None and result.actual is None:
                print("\n")
                continue

            if is_string and ("\n" in result.expected or "\n" in result.actual):
                print(f"\n=== expected:\n\n{result.expected}\n")
                print(f"=== actual:\n\n{result.actual}\n")
            else:
                print(f"    expected: {result.expected}")
                print(f"      actual: {result.actual}\n")

    successful = tests_passed == tests_total
    sys.exit(0 if successful else 1)


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
        section_name = None
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
                if line.startswith(CONDITION_PREFIX + "section"):
                    section_name = line.strip().split(" ")[2]
                    test.sections[section_name] = ""
                    continue

                if has_subtests:
                    enabled = common or j == subtest_index
                else:
                    enabled = True

                if section_name is None:
                    test.source += line if enabled else f"# DISABLED: {line}"
                else:
                    if not has_subtests or j == subtest_index:
                        test.sections[section_name] += line

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


def find_executable(name):
    path = name if install_dir is None else f"{install_dir}/bin/{name}"
    
    if platform.system() == "Windows":
        path = path + ".exe"
    
    return path
