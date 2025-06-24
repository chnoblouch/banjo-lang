import sys
import zipfile
import io
import urllib.request


BASE_URL = "https://banjo-storage.s3.amazonaws.com"


def install_package(package, destination):
    print(f"  Downloading {package}.zip...")

    url = f"{BASE_URL}/package/{package}.zip"
    response = urllib.request.urlopen(url)

    if response.getcode() != 200:
        sys.exit(1)

    print(f"  Extracting {package}.zip...")

    file_bytes = response.read()
    zip_file = zipfile.ZipFile(io.BytesIO(file_bytes), "r")
    zip_file.extractall(destination)
        

if __name__ == "__main__":
    assert len(sys.argv) == 3
    
    try:
        install_package(sys.argv[1], sys.argv[2])
    except Exception:
        sys.exit(1)
