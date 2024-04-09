import os
from pathlib import Path

from linker import UnixLinker
from toolchain import Toolchain


class LlvmAndroidToolchain(Toolchain):

    def __init__(self, target):
        super().__init__(target)

        self.keys = {
            "ndk_path": "Android NDK path"
        }

        self.ndk_path = None

    def validate(self):
        if not self.ndk_path:
            return "Android NDK path missing (ndk_path)"

        return None

    def get_linker(self):
        tools_path = f"{self.ndk_path}/toolchains/llvm/prebuilt/windows-x86_64/bin"
        sysroot = f"{self.ndk_path}/toolchains/llvm/prebuilt/windows-x86_64/sysroot"
        lib_dir = f"{sysroot}/usr/lib/{self.target.arch}-linux-android/30"
        gcc_dir = f"{self.ndk_path}/toolchains/llvm/prebuilt/windows-x86_64/lib/gcc/aarch64-linux-android/4.9.x"

        path = str(Path(tools_path) / "ld.lld")

        args = [
            "--sysroot", sysroot,
            "-L" + lib_dir, "-L" + gcc_dir,
            "-lc", "-lm", "-ldl", "-lgcc",
            "--dynamic-linker", "/system/bin/linker64"
        ]

        crt_files = [
            os.path.join(lib_dir, "crtbegin_dynamic.o"),
            os.path.join(lib_dir, "crtend_android.o")
        ]

        return UnixLinker(path, args, crt_files)
