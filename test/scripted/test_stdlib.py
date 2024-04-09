import subprocess
from pathlib import Path

subprocess.run(["banjo", "test"], cwd=Path(__file__).parent / "stdlib")