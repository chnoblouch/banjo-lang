import subprocess
from framework import TestResult, run_tests, find_executable


def run_test(test, conditions):
    pass_name = arch = test.name.split(".")[0]
    util_path = find_executable("banjo-test-util")
    
    input_source = test.sections["input"].strip()
    output_source = test.sections["output"].strip()

    result = subprocess.run(
        [util_path, "ssa", pass_name],
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

    actual_output_source = result.stdout.strip()

    if actual_output_source == output_source:
        return TestResult(True)
    else:
        return TestResult(False, "ssa mismatch", output_source, actual_output_source)


if __name__ == "__main__":
    run_tests(
        directory="optimizer",
        file_name_extension=".test",
        runner=run_test,
    )
