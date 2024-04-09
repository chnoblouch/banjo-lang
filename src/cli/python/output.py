verbose = False
quiet = False

def print_step(*args, **kwargs):
    if not quiet:
        print(*args, **kwargs)


def print_command(name, command):
    if verbose:
        print(f"[{name}]", " ".join(command), flush=True)
