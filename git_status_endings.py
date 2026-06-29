#!/usr/bin/env python3
#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
import subprocess

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def run_git_status(root_dir="."):
    try:
        proc = subprocess.run(
            ["git", "status", "--porcelain"],
            cwd=root_dir,
            capture_output=True,
            text=True,
        )
    except OSError as e:
        print("git status failed: %s" % e)
        return None
    if proc.returncode != 0:
        err = proc.stderr.strip() if proc.stderr else "unknown error"
        print("git status failed: %s" % err)
        return None
    return proc.stdout

def parse_status_paths(status_out):
    paths = []
    for line in status_out.splitlines():
        if len(line) < 4:
            continue
        body = line[3:].strip()
        if " -> " in body:
            body = body.split(" -> ", 1)[1].strip()
        if body:
            paths.append(body)
    return paths

def file_tag(path):
    path = path.rstrip("/")
    base = os.path.basename(path)
    if not base:
        base = path
    _root, ext = os.path.splitext(base)
    if ext:
        return ext
    parts = base.split("_")
    if parts and parts[-1]:
        return parts[-1]
    return base

def group_by_tag(paths):
    groups = {}
    for path in paths:
        tag = file_tag(path)
        if tag not in groups:
            groups[tag] = []
        groups[tag].append(path)
    return groups

def print_groups(groups):
    rows = []
    for tag, plist in groups.items():
        if len(plist) == 1:
            rows.append((plist[0], plist[0], 1))
        else:
            rows.append((tag, tag, len(plist)))
    rows.sort(key=lambda r: r[0].lower())
    for _sort_key, label, n in rows:
        print("  %s: %d" % (label, n))

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    root = "."
    if len(sys.argv) > 1:
        root = sys.argv[1]
    status_out = run_git_status(root)
    if status_out is None:
        sys.exit(1)
    paths = parse_status_paths(status_out)
    groups = group_by_tag(paths)
    print("git status endings (%s):" % root)
    if not paths:
        print("  (clean)")
    else:
        print_groups(groups)
        print("  Total: %d files" % len(paths))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
