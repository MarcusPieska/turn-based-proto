#!/usr/bin/env python3
#================================================================================================================================#
#=> - print_prereq_counts.py -
#================================================================================================================================#
#
#  For each tech T, list items that have tech(T) as a prereq:
#    - non-tech items (buildings/units/resources/civics/...)
#    - techs
#  Format:  Prereq : Item1, Item2, ...
#  Heaviest gates first.
#
#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import re
import sys
from collections import defaultdict

#================================================================================================================================#
#=> - Config -
#================================================================================================================================#

TECH_REQ_RE = re.compile(r"\btech\(([^)]*)\)")

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def script_dir():
    return os.path.dirname(os.path.abspath(__file__))

def config_files(root):
    out = []
    for name in sorted(os.listdir(root)):
        if not name.startswith("game_config."):
            continue
        path = os.path.join(root, name)
        if os.path.isfile(path):
            out.append(path)
    return out

def line_item_name(line):
    parts = line.split(":", 1)
    if len(parts) == 0:
        return None
    name = parts[0].strip()
    if name == "":
        return None
    return name

def tech_names_in_line(line):
    names = []
    for m in TECH_REQ_RE.finditer(line):
        name = m.group(1).strip()
        if name != "":
            names.append(name)
    return names

def collect_gates(paths):
    tech_unlocks = defaultdict(list)
    nontech_unlocks = defaultdict(list)
    for path in paths:
        base = os.path.basename(path)
        is_tech_file = (base == "game_config.techs")
        with open(path, "r", encoding="utf-8", errors="replace") as ptr:
            text = ptr.read()
        for line in text.splitlines():
            s = line.strip()
            if s == "" or s.startswith("#") or s.startswith("//"):
                continue
            item = line_item_name(s)
            if item is None:
                continue
            prereqs = tech_names_in_line(s)
            if len(prereqs) == 0:
                continue
            bucket = tech_unlocks if is_tech_file else nontech_unlocks
            for pre in prereqs:
                bucket[pre].append(item)
    return tech_unlocks, nontech_unlocks

def print_lists(title, unlocks):
    rows = sorted(unlocks.items(), key=lambda kv: (-len(kv[1]), kv[0]))
    total = sum(len(v) for v in unlocks.values())
    print("%s (%u techs, %u gated items):" % (title, len(rows), total))
    print("-" * 60)
    for pre, items in rows:
        print("%s : %s" % (pre, ", ".join(items)))

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

def main():
    root = script_dir()
    paths = config_files(root)
    if len(paths) == 0:
        print("ERROR: no game_config.* files found in %s" % root)
        return 1
    tech_u, nontech_u = collect_gates(paths)
    print_lists("Non-tech items gated behind each tech", nontech_u)
    print("")
    print_lists("Techs gated behind each tech", tech_u)
    return 0

if __name__ == "__main__":
    sys.exit(main())

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
