from pathlib import Path
import sys
import subprocess


if __name__ == "__main__":
    working_dir = Path(__file__).parent
    source_path = working_dir / "source.s"
    object_path = working_dir / "object.o"
    binary_path = working_dir / "binary.bin"

    source = sys.argv[1]

    with open(source_path, "w") as f:
        f.write(".text\n")
        f.write(source)
    
    subprocess.run([
        "clang",
        "-c",
        "-target", "aarch64-linux",
        f"-o{object_path}",
        f"{source_path}",
    ])

    source_path.unlink()

    subprocess.run([
        "llvm-objcopy",
        "-Obinary",
        "--only-section=.text",
        f"{object_path}",
        f"{binary_path}",
    ])

    object_path.unlink()

    with open(binary_path, "rb") as f:
        encoded = f.read()
        print(encoded.hex().upper())

    binary_path.unlink()
