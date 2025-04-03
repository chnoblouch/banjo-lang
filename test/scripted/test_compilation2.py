import os
import subprocess
import platform

from framework import ProcessResult, TestResult, run_process, run_tests, find_executable


SKIPPED_TESTS = [
    "errors.expected_integer.0",
    "errors.expected_integer.1",
    "errors.expr_category.decl.0",
    "errors.expr_category.decl.3",
    "errors.expr_category.decl.7",
    "errors.expr_category.stmt.2",
    "errors.expr_category.type.4",
    "features.map_literal.0",
    "features.map_literal.1",
    "features.meta_if.8",
    "features.globals.3",
]


is_windows = platform.system() == "Windows"
is_linux = platform.system() == "Linux"
is_macos = platform.system() == "Darwin"


def run_test(test, conditions):
    for condition, _ in conditions:
        if condition == "output":
            return check_output(test, conditions)
        elif condition == "exitcode":
            return check_exit_code(test, conditions)
        elif condition == "compiles":
            return check_compiles(test, conditions)
        elif condition in ("error", "warning"):
            return check_reports(test, conditions)


def check_output(test, conditions):
    for condition, args in conditions:
        if condition == "output":
            expected_output = args[0]
            break

    result = run_executable(test)

    if result.stdout == expected_output:
        return TestResult(True)
    else:
        return TestResult(False, "unexpected output", expected_output, result.stdout)


def check_exit_code(test, conditions):
    for condition, args in conditions:
        if condition == "exitcode":
            expected_exit_code = args[0]
            break

    result = run_executable(test)

    if result.exit_code == expected_exit_code:
        return TestResult(True)
    else:
        return TestResult(False, "unexpected exit code", expected_exit_code, result.exit_code)


def check_compiles(test, conditions):
    result = compile_source(test)

    if result.exit_code == 0:
        return TestResult(True)
    else:
        return TestResult(False, "unexpected exit code", 0, result.exit_code)


def check_reports(test, conditions):
    result = compile_source(test)
    stderr_lines = result.stderr.splitlines()

    expected_reports = []
    
    for condition, args in conditions:
        if condition == "error":
            expected_reports.append(("error", args[1], args[0]))
        elif condition == "warning":
            expected_reports.append(("warning", args[1], args[0]))
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
        if line.startswith("warning: "):
            add_report("warning", line)
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


def compile_source(test):
    with open("tmp.bnj", "w") as f:
        f.write(test.source)

    compiler_path = find_executable("banjo-compiler")

    if is_windows:
        result = run_process([
            compiler_path,
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
            compiler_path,
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
            compiler_path,
            "--type", "executable",
            "--arch", "aarch64",
            "--os", "macos",
            "--opt-level", "0",
            "--path", ".",
            "--no-color",
        ])

    os.remove("tmp.bnj")
    return result


def run_executable(test):
    compile_source(test)

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
    elif is_linux or is_macos:
        if not os.path.exists("main.o"):
            return ProcessResult("", "", 1)

        subprocess.run(["clang", "-fuse-ld=lld", "-otest", "main.o"])
        os.remove("main.o")

        if not os.path.exists("test"):
            return ProcessResult("", "", 1)

        result = run_process(["./test"])
        os.remove("test")

    return result


if __name__ == "__main__":
    run_tests(
        directory="compilation2",
        file_name_extension=".bnj",
        runner=run_test,
        skipped_tests=SKIPPED_TESTS,
    )
