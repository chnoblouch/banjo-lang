from toolchain import Toolchain
from target import Target, get_toolchains_dir
from toolchain_utils import locate_exec, run_detection_tool
from linker import DarwinLinker
import cloud_storage
import output

import platform
from pathlib import Path
import json
import shutil
import os


debug = False


class MacosToolchain(Toolchain):

    def __init__(self, target: Target):
        super().__init__(target)
        
        self.keys = {
            "linker_path": "Linker",
            "sysroot": "Sysroot",
            "extra_args": "Extra args"
        }

        self.linker_path = None
        self.sysroot = None
        self.extra_args = []
    
    def detect(self):
        output.print_step("Locating macOS toolchain...")
        
        self.linker_path = run_detection_tool(["xcodebuild", "-find", "ld"])
        if self.linker_path:
            output.print_step(f"  Found Xcode linker: {self.linker_path}")
        else:
            output.print_step("  Failed to find Xcode linker")
            return
    
        self.sysroot = run_detection_tool(["xcrun", "-sdk", "macosx", "-show-sdk-path"])
        if self.sysroot:
            output.print_step(f"  Found macOS SDK: {self.sysroot}")
        else:
            output.print_step("  Failed to find macOS SDK")
            return

    def install(self):
        print("Installing macOS toolchain...")

        self.linker_path = locate_exec("lld")
        if self.linker_path:
            output.print_step(f"  Found LLD: {self.linker_path}")
            self.extra_args = ["-flavor", "ld64"]
        else:
            output.print_step("  Failed to find LLD")
            return

        sysroot_path = self.get_sysroot_path()
        self.sysroot = str(sysroot_path)

        if sysroot_path.is_dir():
            output.print_step(f"  Found existing sysroot: {self.sysroot}")
            return
        
        output.print_step("  Downloading macOS API definition...")
        api_definition_path = get_toolchains_dir() / "sysroot_macos.json"
        cloud_storage.download("toolchain/sysroot_macos.json", api_definition_path)

        output.print_step("  Generating macOS sysroot...")
        generate_sysroot(api_definition_path, sysroot_path)
        os.remove(api_definition_path)

    def setup(self):
        if platform.system() == "Darwin":
            self.detect()
        else:
            self.install()

    def get_sysroot_path(self):
        return get_toolchains_dir() / "sysroot-macos"
    
    def get_linker(self):
        return DarwinLinker(self.linker_path, self.sysroot, self.extra_args)


def generate_sysroot(api_definition_path, out_dir):
    spec_file = open(api_definition_path, "r")
    spec = json.load(spec_file)
    spec_file.close()

    for library_spec in spec["libraries"]:
        write_tapi(library_spec, Path(str(out_dir) + library_spec["install_path"]))

    for framework_spec in spec["frameworks"]:
        name = framework_spec["name"]
    
        tapi_path = Path(str(out_dir) + framework_spec["install_path"] + ".tbd")
        write_tapi(framework_spec, tapi_path)

        for sub_spec in framework_spec["frameworks"]:
            write_tapi(sub_spec, Path(str(out_dir) + sub_spec["install_path"] + ".tbd"))

        for sub_spec in framework_spec["libraries"]:
            write_tapi(sub_spec, Path(str(out_dir) + sub_spec["install_path"]))

        shutil.copy(tapi_path, tapi_path.parents[2] / f"{name}.tbd")


def write_tapi(spec, path):
    if not path.parent.exists():
        path.parent.mkdir(parents=True)

    tapi_file = open(path, "w", newline="")
    
    install_path = spec["install_path"]
    reexports = spec["reexports"]
    symbols = spec["symbols"]
    objc_classes = spec["objc_classes"]
    objc_ivars = spec["objc_ivars"]
        
    if debug:
        tapi_file.write("--- !tapi-tbd\n")
        tapi_file.write("tbd-version: 4\n")
        tapi_file.write("targets: [ x86_64-macos, arm64-macos ]\n")
        tapi_file.write(f"install-name: 'install_name'\n")
        
        tapi_file.write("\nreexported-libraries:\n")
        tapi_file.write("  - targets: [ x86_64-macos, arm64-macos ]\n")
    
        tapi_file.write(f"    libraries:\n")
        for reexport in reexports:
            tapi_file.write(f"      - {reexport}\n")
            
        tapi_file.write("\nexports:\n")
        tapi_file.write("  - targets: [ x86_64-macos, arm64-macos ]\n")
        
        tapi_file.write("\n    symbols:\n")
        for symbol in symbols:
            tapi_file.write(f"      - {symbol}\n")

        tapi_file.write("\n    objc-classes:\n")
        for symbol in objc_classes:
            tapi_file.write(f"      - {symbol}\n")

        tapi_file.write("\n    objc-ivars:\n")
        for symbol in objc_ivars:
            tapi_file.write(f"      - {symbol}\n")

        tapi_file.write("...\n")
    else:
        tapi_file.write("--- !tapi-tbd\n")
        tapi_file.write("tbd-version: 4\n")
        tapi_file.write("targets: [ x86_64-macos, arm64-macos ]\n")
        tapi_file.write(f"install-name: '{install_path}'\n")
        
        tapi_file.write("reexported-libraries: [ { ")
        tapi_file.write("targets: [ x86_64-macos, arm64-macos ], ")
        tapi_file.write(f"libraries: [ {', '.join(reexports)} ]")
        tapi_file.write(" } ]\n")
            
        tapi_file.write("exports: [ { ")
        tapi_file.write("targets: [ x86_64-macos, arm64-macos ], ")
        tapi_file.write(f"symbols: [ {', '.join(symbols)} ], ")
        tapi_file.write(f"objc-classes: [ {', '.join(objc_classes)} ], ")
        tapi_file.write(f"objc-ivars: [ {', '.join(objc_ivars)} ]")
        tapi_file.write(" } ]\n")

        tapi_file.write("...\n")
    
    tapi_file.close()
