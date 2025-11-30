import subprocess
from framework import TestResult, run_tests, find_executable


def run_test(test, conditions):
    util_path = find_executable("banjo-test-util")
    
    input_source = test.sections["input"].strip()
    output_source = test.sections["output"].strip()

    result = subprocess.run(
        [util_path, "format", input_source],
        stdout=subprocess.PIPE,
        text=True,
    )

    actual_output_source = result.stdout.strip()

    if actual_output_source == output_source:
        return TestResult(True)
    else:
        return TestResult(False, "formatting mismatch", output_source, actual_output_source)


if __name__ == "__main__":
    run_tests(
        directory="formatter",
        file_name_extension=".test",
        runner=run_test,
    )
