import subprocess
import os
from pathlib import Path

TARGETS = [
    ("x86_64", "windows", "msvc", "x86_64-windows-msvc"),
    ("x86_64", "windows", "gnu", "x86_64-windows-gnu"),
    ("x86_64", "linux", "gnu", "x86_64-linux-gnu"),
    ("x86_64", "macos", None, "x86_64-macos-darwin"),
    ("aarch64", "linux", "gnu", "aarch64-linux-gnu"),
    ("aarch64", "macos", None, "aarch64-macos-darwin")
]

if __name__ == "__main__":
    for target in TARGETS:
        target_arch, target_os, target_env, clang_target = target
        target_name = f"{target_arch}-{target_os}-{target_env}" if target_env else f"{target_arch}-{target_os}"
        is_windows = target_os == "windows"

        print(f"Building runtime for {target_name}...")

        object_file_name = "runtime.obj" if is_windows else "runtime.o"
        library_name = target_name + (".lib" if is_windows else ".a")
        library_path = Path(__file__).parents[1] / "bin" / library_name

        subprocess.run(["clang", "-c", "--target=" + clang_target, "-o", object_file_name, "runtime.c"])
        
        if is_windows:
            subprocess.run(["llvm-lib", object_file_name, "/OUT:" + str(library_path)])
        else:
            subprocess.run(["llvm-ar", "cr", str(library_path), object_file_name])
            
        os.remove(object_file_name)
