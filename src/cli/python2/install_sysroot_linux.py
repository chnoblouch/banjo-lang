import sys
import zipfile
import io
import urllib.request


BASE_URL = "https://banjo-storage.s3.amazonaws.com"


def install_sysroot(arch, destination):
    sysroot_name = f"sysroot-{arch}-linux-gnu"
    zip_file_name = f"{sysroot_name}.zip"

    print(f"    Downloading {zip_file_name}...")

    url = f"{BASE_URL}/toolchain/{zip_file_name}"
    response = urllib.request.urlopen(url)

    if response.getcode() != 200:
        sys.exit(1)

    print(f"    Extracting {zip_file_name}...")

    file_bytes = response.read()
    zip_file = zipfile.ZipFile(io.BytesIO(file_bytes), "r")
    zip_file.extractall(destination)


if __name__ == "__main__":
    assert len(sys.argv) == 3

    try:
        install_sysroot(sys.argv[1], sys.argv[2])
    except Exception:
        sys.exit(1)
