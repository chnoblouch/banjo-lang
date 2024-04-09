import os
import json
import zipfile
import io

from configuration import Configuration
from toolchain import Toolchain
import cloud_storage
import error
import output


def load(config: Configuration, toolchain: Toolchain):
    for package_name in config.packages:
        if is_local_package(package_name):
            root_dir = package_name
        else:
            root_dir = os.path.join("packages", package_name)

            if not os.path.exists(root_dir):
                install(package_name)

        config.paths.append(os.path.join(root_dir, "src/"))
        config.library_paths.append(os.path.join(root_dir, "lib", str(toolchain.target)))

        try:
            info_file = open(os.path.join(root_dir, "banjo.json"))
            json_root = json.load(info_file)
            info_file.close()

            if "targets" in json_root:
                if toolchain.target.triple in json_root["targets"]:
                    target_json = json_root["targets"][toolchain.target.triple]

                    if "libraries" in target_json:
                        config.libraries.extend(target_json["libraries"])
                    if "library_paths" in target_json:
                        config.library_paths.extend(
                            target_json["library_paths"])
                    if "macos.frameworks" in target_json:
                        config.macos_frameworks.extend(
                            target_json["macos.frameworks"])
            if "libraries" in json_root:
                config.libraries.extend(json_root["libraries"])
            if "library_paths" in json_root:
                config.library_paths.extend(json_root["library_paths"])
            if "win.resources" in json_root:
                config.linked_win_resources.extend(json_root["win.resources"])
        except json.JSONDecodeError:
            print(f"Failed to load banjo.json in package \"{package_name}\"")


def install(package):
    if not output.quiet:
        print(f"Installing package '{package}'...")
        print(f"  Downloading {package}.zip...")

    response = cloud_storage.request(f"package/{package}.zip")

    if response.getcode() == 200:
        if not output.quiet:
            print(f"  Extracting {package}.zip...")

        file_bytes = response.read()
        zip_file = zipfile.ZipFile(io.BytesIO(file_bytes), "r")
        zip_file.extractall(f"packages")
    else:
        error.report_fatal("Failed to download package")


def is_local_package(package) -> bool:
    return "/" in package or os.sep in package
