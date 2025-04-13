import subprocess
from framework import TestResult, run_tests, find_executable


def run_test(test, conditions):
    util_path = find_executable("banjo-test-util")
    
    input_source = test.sections["input"].strip()
    output_source = test.sections["output"].strip()

    result = subprocess.run(
        [util_path, "ssa"],
        stdout=subprocess.PIPE,
        text=True,
        input=input_source,
    )

    # with open("input", "w") as f:
    #     f.write(input_source)

    # with open("output", "w") as f:
    #     f.write(output_source)

    # with open("output.actual", "w") as f:
    #     f.write(result.stdout.strip())

    if result.stdout.strip() == output_source:
        return TestResult(True)
    else:
        return TestResult(False, "ssa does not match")


if __name__ == "__main__":
    run_tests(
        directory="optimizer",
        file_name_extension=".test",
        runner=run_test,
    )
