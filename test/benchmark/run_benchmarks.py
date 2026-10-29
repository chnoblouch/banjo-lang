import sys
import subprocess
import platform
import shutil
import os
from pathlib import Path


def run_benchmark(banjo_binary, cpp_binary):
    subprocess.run([
        "hyperfine",
        "-w10", "-m50",
        banjo_binary,
        cpp_binary,
    ])


if __name__ == "__main__":
    name = sys.argv[1]

    benchmarks_dir = Path(__file__).parent
    banjo_dir = benchmarks_dir / "banjo"
    cpp_dir = benchmarks_dir / "cpp"

    banjo_source = f"./{name}.bnj"
    cpp_source = f"./{name}.cpp"

    if platform.system().lower() == "windows":
        banjo_binary = banjo_dir / f"{name}.exe"
        cpp_binary = cpp_dir / "a.exe"
    else:
        banjo_binary = banjo_dir / name
        cpp_binary = cpp_dir / "a.out"

    print("\n--- Debug Build ---\n")
    subprocess.run(["banjo", "build", "--opt-level", "0", banjo_source], cwd=banjo_dir)
    subprocess.run(["clang++", "-O0", cpp_source], cwd=cpp_dir)

    run_benchmark(banjo_binary, cpp_binary)

    print("\n--- Optimized Build ---\n")
    subprocess.run(["banjo", "build", "--opt-level", "2", banjo_source], cwd=banjo_dir)
    subprocess.run(["clang++", "-O3", cpp_source], cwd=cpp_dir)
    run_benchmark(banjo_binary, cpp_binary)

    os.remove(banjo_binary)
    os.remove(cpp_binary)