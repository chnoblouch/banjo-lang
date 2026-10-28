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
    os_name = platform.system().lower()
    
    benchmarks_dir = Path(__file__).parent
    banjo_dir = benchmarks_dir / "banjo" / name
    cpp_dir = benchmarks_dir / "cpp" 
    
    print("\n--- Debug Build ---\n")
    subprocess.run(["banjo", "build", "--opt-level", "0"], cwd=banjo_dir)
    subprocess.run(["clang++", "-O0", f"./{name}.cpp"], cwd=cpp_dir)

    banjo_binary = next((banjo_dir / "out").iterdir()) / name
    cpp_binary = cpp_dir / ("a.exe" if os_name == "windows" else "a.out")
    run_benchmark(banjo_binary, cpp_binary)

    print("\n--- Optimized Build ---\n")
    subprocess.run(["banjo", "build", "--opt-level", "2"], cwd=banjo_dir)
    subprocess.run(["clang++", "-O3", f"./{name}.cpp"], cwd=cpp_dir)
    run_benchmark(banjo_binary, cpp_binary)

    shutil.rmtree(banjo_dir / "out")
    os.remove(cpp_binary)