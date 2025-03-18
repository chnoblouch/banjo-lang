import subprocess
from framework import TestResult, run_tests, find_executable


def run_test(test, conditions):
    util_path = find_executable("banjo-test-util")
    result = subprocess.run([util_path, test.source], stdout=subprocess.PIPE, text=True)

    for condition, args in conditions:
        if condition != "encoding":
            continue

        expected = args[0]
        actual = result.stdout

        if expected == actual:
            return TestResult(True)
        else:
            return TestResult(False, "unexpected encoding", expected, actual)

    return TestResult(False, "no condition provided")


if __name__ == "__main__":
    run_tests(
        directory="assembler",
        file_name_extension=".test",
        runner=run_test,
    )
