import shutil
import platform
import subprocess


def locate_exec(name):
    if type(name) == str:
        path = shutil.which(name)
    elif type(name) == list:
        path = None
        for cmd in name:
            path = shutil.which(cmd)
            if path:
                break
    
    if not path:
        return None

    if platform.system() == "Windows" and path.endswith(".EXE"):
        path = path[:-4]
    return path


def run_detection_tool(command):
    completed_process = subprocess.run(command, check=True, stdout=subprocess.PIPE)
    if completed_process.returncode != 0:
        return None

    return completed_process.stdout.decode().strip()