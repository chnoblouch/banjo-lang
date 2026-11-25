import sys
import subprocess
from pathlib import Path

SCRIPTS = (
    "test_compilation.py",
    "test_codegen.py",
    "test_assembler.py",
    "test_optimizer.py",
)


if __name__ == "__main__":
    for script in SCRIPTS:
        path = Path(__file__).parent / script
        subprocess.run([sys.executable, str(path), *sys.argv[1:]])
