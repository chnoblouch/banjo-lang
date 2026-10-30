import sys
import subprocess
import platform
import shutil
import os
from pathlib import Path


def run_benchmark(banjo_binary, cpp_binary):
    subprocess.run([
        "hyperfine",
        "-w10", "-m20", "-usecond",
        "-nbanjo",
        "-nc++",
        f"./{banjo_binary}",
        f"./{cpp_binary}",
    ])


if __name__ == "__main__":
    name = sys.argv[1]
    os.chdir(Path(__file__).parent / name)

    banjo_source = f"./{name}.bnj"
    cpp_source = f"./{name}.cpp"

    if platform.system().lower() == "windows":
        banjo_binary = f"{name}.exe"
        cpp_binary = f"{name}-cpp.exe"
    else:
        banjo_binary = name
        cpp_binary = f"{name}-cpp"

    print("\n--- Debug Build ---\n")
    subprocess.run(["banjo", "build", "--opt-level", "0", "-q", banjo_source])
    subprocess.run(["clang++", f"-o{cpp_binary}", "-O0", cpp_source])
    run_benchmark(banjo_binary, cpp_binary)

    print("\n--- Optimized Build ---\n")
    subprocess.run(["banjo", "build", "--opt-level", "2", "-q", banjo_source])
    subprocess.run(["clang++", f"-o{cpp_binary}", "-O3", cpp_source])
    run_benchmark(banjo_binary, cpp_binary)

    os.remove(banjo_binary)
    os.remove(cpp_binary)