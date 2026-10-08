import sys
import subprocess
from pathlib import Path


if __name__ == "__main__":
    arch = sys.argv[1]
    filename = sys.argv[2]
    lines = []

    with open(filename) as f:
        for line in f.readlines():
            if line.startswith("# test:encoding"):
                encoding_line_index = len(lines)
            elif not line.startswith("# ") and line.strip():
                script = Path(__file__).parent / f"assemble_{arch}.py"
                child = subprocess.run(["python3", str(script), line], stdout=subprocess.PIPE, text=True)
                encoding = str(child.stdout).strip()
                lines[encoding_line_index] = f"# test:encoding \"{encoding}\"\n"

            lines.append(line)

    with open(filename, "w") as f:
        f.writelines(lines)
