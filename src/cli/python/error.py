import sys


def report_fatal(message):
    print("error:", message, file=sys.stderr)
    exit(1)