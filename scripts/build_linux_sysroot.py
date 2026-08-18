# Dependencies: 
#   - gcc
#   - make
#   - gawk
#   - bison
#   - flex
#   - sed
#   - texinfo
#   - libgmp
#   - libmpfr
#   - libmpc 

import sys
import os
import platform
import tarfile
import subprocess
import shutil
import urllib.request
from pathlib import Path
from io import BytesIO


GLIBC_LIBRARIES = [
    "crt1.o",
    "crti.o",
    "crtn.o",
    ("libc.so", 6),
    ("libm.so", 6),
    ("libdl.so", 2),
    ("libpthread.so", 0),
    "libc_nonshared.a",
]

GCC_LIBRARIES = [
    "crtbegin.o",
    "crtend.o",
    "crtfastmath.o",
    "libgcc.a",
]


if __name__ == "__main__":
    # Defaults are based on libraries available on Ubuntu 24.04
    glibc_version = sys.argv[1] if len(sys.argv) > 1 else "2.39"
    gcc_version = sys.argv[2] if len(sys.argv) > 2 else "13.4.0"

    print(f"Building glibc version {glibc_version}")
    print(f"Building libgcc version {gcc_version}")

    if platform.machine().lower() in ("x86_64", "amd64"):
        arch = "x86_64"
        target = "x86_64-pc-linux-gnu"
    elif platform.machine().lower() in ("aarch64", "arm64"):
        arch = "aarch64"
        target = "aarch64-unknown-linux-gnu"

    root_path = Path("linux-sysroot-build").absolute()    
    root_path.mkdir(exist_ok=True)
    os.chdir(root_path)

    glibc_source_path = Path(f"glibc-{glibc_version}").absolute()
    glibc_build_path = Path("glibc-build").absolute()
    gcc_source_path = Path(f"gcc-{gcc_version}").absolute()
    gcc_build_path = Path("gcc-build").absolute()

    glibc_install_path = glibc_build_path / "install"
    gcc_install_path = gcc_build_path / "install"

    if not glibc_source_path.is_dir():
        with urllib.request.urlopen(f"https://ftp.gnu.org/gnu/glibc/glibc-{glibc_version}.tar.xz") as f:
            print("Downloading glibc...")

            with tarfile.open(name=None, fileobj=BytesIO(f.read())) as t:
                print("Extracting glibc...")
                t.extractall()

    if not gcc_source_path.is_dir():
        with urllib.request.urlopen(f"https://ftp.fu-berlin.de/unix/languages/gcc/releases/gcc-{gcc_version}/gcc-{gcc_version}.tar.xz") as f:
            print("Downloading gcc...")

            with tarfile.open(name=None, fileobj=BytesIO(f.read())) as t:
                print("Extracting gcc...")
                t.extractall()

    glibc_build_path.mkdir(exist_ok=True)
    gcc_build_path.mkdir(exist_ok=True)
    glibc_install_path.mkdir(exist_ok=True)
    gcc_install_path.mkdir(exist_ok=True)

    subprocess.run(
        [
            f"{glibc_source_path}/configure",
            f"--prefix={glibc_install_path}",
            f"--libdir={glibc_install_path}/lib",
            f"--build={target}",
            f"--host={target}",
            "--disable-werror",
            "--disable-mathvec",
        ],
        cwd=glibc_build_path,
    )

    subprocess.run(["make", f"-C{glibc_build_path}", "-j8", "install"])

    subprocess.run(
        [
            f"{gcc_source_path}/configure",
            f"--prefix={gcc_install_path}",
            f"--libdir={gcc_install_path}/lib",
            "--enable-languages=c,c++",
            "--disable-multilib",
        ],
        cwd=gcc_build_path,
    )

    subprocess.run(["make", "-j8", "all-gcc"], cwd=gcc_build_path)
    subprocess.run(["make", "-j8", "all-target-libgcc"], cwd=gcc_build_path)
    subprocess.run(["make", "-j8", "install-target-libgcc"], cwd=gcc_build_path)

    out_dir = Path(f"sysroot-{arch}-linux-gnu").absolute()
    glibc_licenses_dir = out_dir / "licenses" / "glibc"
    gcc_licenses_dir = out_dir / "licenses" / "gcc"

    if out_dir.exists():
        shutil.rmtree(out_dir)

    out_dir.mkdir()
    glibc_licenses_dir.mkdir(parents=True)
    gcc_licenses_dir.mkdir(parents=True)

    glibc_libraries = GLIBC_LIBRARIES

    if arch == "x86_64":
        glibc_libraries.append("ld-linux-x86-64.so.2")
    if arch == "aarch64":
        glibc_libraries.append("ld-linux-aarch64.so.1")

    for file in glibc_libraries:
        if type(file) is str:
            shutil.copy(glibc_install_path / "lib" / file, out_dir / file)
        elif type(file) is tuple:
            file, version = file
            shutil.copy(glibc_install_path / "lib" / (file + f".{version}"), out_dir / file)

    for file in GCC_LIBRARIES:
        gcc_lib_dir = gcc_install_path / "lib" / "gcc" / target / gcc_version
        shutil.copy(gcc_lib_dir / file, out_dir / file)

    shutil.copy(gcc_install_path / "lib64" / "libgcc_s.so.1", out_dir / "libgcc_s.so")

    for file in ["COPYINGv2", "COPYING.LESSERv2", "COPYINGv3", "COPYING.LIB", "LICENSES"]:
        src = glibc_source_path / file

        if Path(src).is_file():
            shutil.copy(src, glibc_licenses_dir / file)

    for file in ["COPYING", "COPYING.LIB", "COPYING.RUNTIME", "COPYING3", "COPYING3.LIB"]:
        shutil.copy(gcc_source_path / file, gcc_licenses_dir / file)
