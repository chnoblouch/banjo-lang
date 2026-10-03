import subprocess
from framework import TestResult, run_tests, find_executable


def run_test(test, conditions):
    arch = test.name.split(".")[0]
    util_path = find_executable("banjo-test-util")
    
    ssa = test.sections["ssa"].strip()
    mcode = test.sections["mcode"].strip()

    result = subprocess.run(
        [util_path, "codegen", arch],
        stdout=subprocess.PIPE,
        text=True,
        input=ssa,
    )

    actual_mcode = result.stdout.strip()

    if actual_mcode == mcode:
        return TestResult(True)
    else:
        return TestResult(False, "invalid codegen", mcode, actual_mcode)


if __name__ == "__main__":
    run_tests(
        directory="codegen",
        file_name_extension=".test",
        runner=run_test,
    )
