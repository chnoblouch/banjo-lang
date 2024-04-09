import platform
from pathlib import Path
import urllib.request
from zipfile import ZipFile
import os


ARCHITECTURES = {
    "x86_64": "x86_64",
    "amd64": "x86_64",
    "aarch64": "aarch64",
    "arm64": "aarch64",
}

OPERATING_SYSTEMS = {
    "windows": "windows",
    "linux": "linux",
    "darwin": "macos",
}

BASE_URL = "https://github.com/chnoblouch/banjo-lang/releases/latest/download"

def run():
    cur_arch = ARCHITECTURES[platform.machine().lower()]
    cur_os = OPERATING_SYSTEMS[platform.system().lower()]
    target = f"{cur_arch}-{cur_os}"

    print(f"\nPlatform: {target}")

    if cur_os != "windows":
        print("Shell: ", end="")

        if os.environ.get("SHELL") == "/bin/zsh":
            shell = "zsh"
            print("Zsh")
        else:
            shell = None
            print("Unknown (can't set PATH)")
        
    install_dir = Path.home() / ".banjo"
    bin_dir = install_dir / "bin"

    if not install_dir.exists():
        install_dir.mkdir()

    print(f"Install directory: {install_dir}")

    zip_name = f"banjo-{target}.zip"
    url = f"{BASE_URL}/{zip_name}"
    print(f"Download URL: {url}")

    print(f"\nDownloading...")

    zip_path = install_dir / zip_name
    urllib.request.urlretrieve(url, zip_path)

    print(f"Extracting...")

    zip_ref = ZipFile(zip_path, "r")

    for member in zip_ref.filelist:
        prefix = f"banjo-{target}/"
        if member.filename == prefix or not member.filename.startswith(prefix):
            continue

        member.filename = member.filename[len(prefix):]
        zip_ref.extract(member, install_dir)

    zip_ref.close()
    zip_path.unlink()

    print("Download finished!")

    if cur_os == "windows" or shell:
        print("\nThis script can add banjo to your PATH environment variable")
    
        if cur_os == "windows":
            print("This will append the bin directory to your HKEY_CURRENT_USER/Environment/Path registry key")
        elif shell == "bash":
            print("This will append an export command to your .bashrc")
        elif shell == "zsh":
            print("This will append an export command to your .zshrc")

        print("Do you want to add banjo to your PATH? [Y/n] ", end="")
        response = input()
    else:
        response = None

    if response in ("y", "Y", ""):
        if cur_os == "windows":
            modify_path_variable_windows(bin_dir)
        else:
            modify_path_variable_unix(bin_dir, shell)
        
        print("PATH environment variable updated!")
        print("\nRestart your shell or run this to start using banjo:")
    else:
        print("\nRun this to add banjo to your PATH for the current shell session:")

    if cur_os == "windows":
        print(f"$ENV:PATH=\"$ENV:PATH;{bin_dir}\"")
    else:
        print(f"export PATH=\"$PATH:{bin_dir}\"")

    print()


def modify_path_variable_windows(bin_dir):
    import winreg
    registry = winreg.ConnectRegistry(None, winreg.HKEY_CURRENT_USER)

    with winreg.OpenKey(registry, "Environment", 0, winreg.KEY_QUERY_VALUE | winreg.KEY_SET_VALUE) as key:
        value, _ = winreg.QueryValueEx(key, "Path")
        
        if value[:-1] != ";":
            value += ";"
        
        value += str(bin_dir)

        winreg.SetValueEx(key, "Path", 0, winreg.REG_SZ, value)


def modify_path_variable_unix(bin_dir, shell):
    if shell == "bash":
        rc_dir = Path.home() / ".bashrc"
    elif shell == "zsh":
        rc_dir = Path.home() / ".zshrc"

    with open(rc_dir, "a") as file:
        file.write(f"\nexport PATH=\"$PATH:{bin_dir}\"\n")


if __name__ == "__main__":
    run()
