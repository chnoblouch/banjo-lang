# verbose = False
# quiet = False

# def print_step(*args, **kwargs):
#     if not quiet:
#         print(*args, **kwargs)


# def print_command(name, command):
#     if verbose:
#         print(f"[{name}]", " ".join(command), flush=True)

verbose = False
quiet = False

def print_step(*args, **kwargs):
    if not quiet:
        if args[0] != "Running..." and not args[0].startswith("Build finished"):
            print("\x1b[2m\x1b[2K\r" + " ".join(args) + "\x1b[0m\r", **kwargs, end="")
        else:
            print("\x1b[2K\r", end="")


def print_command(name, command):
    if verbose:
        print(f"[{name}]", " ".join(command), flush=True)
