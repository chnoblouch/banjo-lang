import os
import json
import subprocess
import winreg
import re
from dataclasses import dataclass

from linker import Linker, WindowsLinker
from configuration import Configuration
from toolchain import Toolchain
import output


@dataclass
class VsInstallation:
    tools_path: str
    lib_path: str


_WINSDK_SUPER_KEYS = [winreg.HKEY_LOCAL_MACHINE, winreg.HKEY_CURRENT_USER]
_WINSDK_SUB_KEYS = [r"SOFTWARE\Wow6432Node", r"SOFTWARE"]


class MsvcLinker(WindowsLinker):

    def __init__(self):
        super().__init__("link")


class MsvcToolchain(Toolchain):

    def __init__(self, target):
        super().__init__(target)
        self.keys = {
            "tools": "MSVC tools",
            "lib": "Windows SDK"
        }

        self.installation = None

    def load(self, json_config):
        tools_dir = json_config["tools"]
        lib_dir = json_config["lib"]
        self.installation = VsInstallation(tools_dir, lib_dir)
        self.apply()

    def store(self):
        return {
            "tools": self.installation.tools_path,
            "lib": self.installation.lib_path
        }

    def detect(self):
        if "VCToolsInstallDir" in os.environ:
            output.print_step("Loading MSVC toolchain from environment...")

            tools_dir = os.environ["VCToolsInstallDir"].replace("\\", "/")
            win_sdk_dir = os.environ["WindowsSdkDir"].replace("\\", "/")
            win_sdk_ver = os.environ["WindowsSdkVersion"].replace("\\", "/")

            self.installation = VsInstallation(tools_dir, os.path.join(win_sdk_dir, "Lib", win_sdk_ver))
            self.apply()
            return

        output.print_step("Locating MSVC toolchain...")

        vswhere_path = _find_vswhere()
        if not vswhere_path:
            output.print_step("  Failed to locate vswhere!")
            return
        output.print_step("  Found vswhere:", vswhere_path)

        vs_path = _find_vs_path(vswhere_path)
        if not vs_path:
            output.print_step("  Failed to locate Visual Studio!")
            return
        output.print_step("  Found Visual Studio installation:", vs_path)

        msvc_version = _find_latest_msvc_version(vs_path)
        output.print_step("  Using MSVC version", msvc_version)

        winsdk_root_path = _find_winsdk_root_path()
        output.print_step("  Found Windows SDK:", winsdk_root_path)

        winsdk_version = _find_latest_winsdk_version(winsdk_root_path)
        output.print_step("  Using Windows SDK version", winsdk_version)

        tools_path = os.path.join(vs_path, "VC", "Tools", "MSVC", msvc_version)
        lib_path = os.path.join(winsdk_root_path, "Lib", winsdk_version)

        self.installation = VsInstallation(tools_path, lib_path)
        self.apply()

    def apply(self):
        if self.target.arch == "x86_64":
            win_architecture = "x64"
        elif self.target.arch == "aarch64":
            win_architecture = "arm64"
        else:
            win_architecture = "???"

        tools_path = os.path.join(self.installation.tools_path, "bin", "Hostx64", win_architecture)
        msvc_lib_path = os.path.join(self.installation.tools_path, "lib", win_architecture)
        um_lib_path = os.path.join(self.installation.lib_path, "um", win_architecture)
        ucrt_lib_path = os.path.join(self.installation.lib_path, "ucrt", win_architecture)

        os.environ["PATH"] = tools_path + os.pathsep + os.environ["PATH"]
        os.environ["LIB"] = os.pathsep.join([msvc_lib_path, um_lib_path, ucrt_lib_path])

    def get_linker(self):
        return MsvcLinker()


def _find_vswhere():
    if "ProgramFiles(x86)" in os.environ:
        program_files_path = os.environ["ProgramFiles(x86)"]
    elif "ProgramFiles" in os.environ:
        program_files_path = os.environ["ProgramFiles"]
    else:
        return None

    path = os.path.join(program_files_path, "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if not os.path.isfile(path):
        return None

    return path


def _find_vs_path(vswhere_path):
    def run_vswhere(args):
        command = [vswhere_path, "-format", "json"] + args
        result = subprocess.run(command, stdout=subprocess.PIPE)
        return json.loads(result.stdout.decode())
    
    # Look for a full Visual Studio installation.
    result = run_vswhere(["-latest"])
    if len(result) != 0:
        return result[0]["installationPath"]
    
    # Look for a Visual Studio Build Tools installation.
    result = run_vswhere(["-latest", "-products", "Microsoft.VisualStudio.Product.BuildTools"])
    if len(result) != 0:
        return result[0]["installationPath"]

    return None


def _find_latest_msvc_version(root_path):
    versions_path = os.path.join(root_path, "VC", "Tools", "MSVC")
    versions = os.listdir(versions_path)
    latest = _max_version(versions, 3)
    return latest


def _find_winsdk_root_path():
    for super_key in _WINSDK_SUPER_KEYS:
        for sub_key in _WINSDK_SUB_KEYS:
            full_sub_key = rf"{sub_key}\Microsoft\Microsoft SDKs\Windows\v10.0"
            access = winreg.KEY_READ | winreg.KEY_WOW64_64KEY

            try:
                key = winreg.OpenKey(super_key, full_sub_key, access=access)
                result = winreg.QueryValueEx(key, "InstallationFolder")[0]
                key.Close()
                return result
            except FileNotFoundError:
                output.print_step("  Registry key not found.")


def _find_latest_winsdk_version(winsdk_root_path):
    versions_path = os.path.join(winsdk_root_path, "Lib")
    versions = os.listdir(versions_path)
    latest = _max_version(versions, 4)
    return latest


def _max_version(versions, num_components):
    def get_components(version):
        pattern = r"\.".join(num_components * [r"(\d+)"])
        re_match = re.search(pattern, version)
        if not re_match:
            return None

        components = re_match.groups()
        return tuple(int(component) for component in components)

    versions = filter(lambda version: get_components(version) is not None, versions)
    return max(versions, key=get_components)
