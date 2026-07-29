import sys
import io
import json
import os
import shutil
import urllib.request
from pathlib import Path


BASE_URL = "https://marinohimself.ch/banjo/storage"
PRETTY = True


def install_sysroot(destination):
    sysroot_name = f"sysroot-aarch64-macos"
    spec_file_name = f"sysroot-macos-api.json"

    print(f"    Downloading {spec_file_name}...")

    url = f"{BASE_URL}/toolchain/{spec_file_name}"
    response = urllib.request.urlopen(url)

    if response.getcode() != 200:
        sys.exit(1)

    print(f"    Generating macOS sysroot...")
    generate_sysroot(response.read(), str(Path(destination) / sysroot_name))


def generate_sysroot(spec_bytes, destination):
    spec = json.load(io.BytesIO(spec_bytes))

    for library_spec in spec["libraries"]:
        write_tapi(library_spec, Path(destination + "/" + library_spec["install_path"]))

    for framework_spec in spec["frameworks"]:
        name = framework_spec["name"]
    
        tapi_path = Path(destination + "/" + framework_spec["install_path"] + ".tbd")
        write_tapi(framework_spec, tapi_path)

        for sub_spec in framework_spec["frameworks"]:
            write_tapi(sub_spec, Path(destination + "/" + sub_spec["install_path"] + ".tbd"))

        for sub_spec in framework_spec["libraries"]:
            write_tapi(sub_spec, Path(destination + "/" + sub_spec["install_path"]))

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

    if PRETTY:
        tapi_file.write("--- !tapi-tbd\n")
        tapi_file.write("tbd-version: 4\n")
        tapi_file.write("targets: [ arm64-macos ]\n")
        tapi_file.write(f"install-name: 'install_name'\n")
        
        tapi_file.write("\nreexported-libraries:\n")
        tapi_file.write("  - targets: [ arm64-macos ]\n")
    
        tapi_file.write(f"    libraries:\n")
        for reexport in reexports:
            tapi_file.write(f"      - {reexport}\n")
            
        tapi_file.write("\nexports:\n")
        tapi_file.write("  - targets: [ arm64-macos ]\n")
        
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
        tapi_file.write("targets: [ arm64-macos ]\n")
        tapi_file.write(f"install-name: '{install_path}'\n")
        
        tapi_file.write("reexported-libraries: [ { ")
        tapi_file.write("targets: [ arm64-macos ], ")
        tapi_file.write(f"libraries: [ {', '.join(reexports)} ]")
        tapi_file.write(" } ]\n")
            
        tapi_file.write("exports: [ { ")
        tapi_file.write("targets: [ arm64-macos ], ")
        tapi_file.write(f"symbols: [ {', '.join(symbols)} ], ")
        tapi_file.write(f"objc-classes: [ {', '.join(objc_classes)} ], ")
        tapi_file.write(f"objc-ivars: [ {', '.join(objc_ivars)} ]")
        tapi_file.write(" } ]\n")

        tapi_file.write("...\n")
    
    tapi_file.close()


if __name__ == "__main__":
    assert len(sys.argv) == 2

    try:
        install_sysroot(sys.argv[1])
    except Exception:
        sys.exit(1)
