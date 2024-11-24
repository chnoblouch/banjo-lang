single_line = True
verbose = False
quiet = False

def print_step(*args, **kwargs):
    if quiet:
        return

    if single_line:
        print("\x1b[2m\x1b[2K\r" + " ".join(args) + "\x1b[0m\r", **kwargs, end="")
    else:
        print(*args, **kwargs)


def print_final_step(*args, **kwargs):
    if quiet:
        return

    if single_line:
        print("\x1b[2K\r", end="")
    else:
        print(*args, *kwargs)


def print_command(name, command):
    if verbose:
        print(f"[{name}]", " ".join(command), flush=True)
