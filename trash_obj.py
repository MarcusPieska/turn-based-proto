#!/usr/bin/env python3

import os
import shutil
import sys

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
TRASH_DIR = "/home/w/Projects/trash"


def main():
    os.makedirs(TRASH_DIR, exist_ok=True)
    moved = 0
    for root, _dirs, files in os.walk(BASE_DIR):
        for name in files:
            if not name.endswith(".o"):
                continue
            src = os.path.join(root, name)
            dst = os.path.join(TRASH_DIR, name)
            if os.path.lexists(dst):
                os.remove(dst)
            shutil.move(src, dst)
            moved += 1
            print(os.path.relpath(src, BASE_DIR))
    print("moved %u object file(s) to %s" % (moved, TRASH_DIR), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())
