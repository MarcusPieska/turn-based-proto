#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os

from generate import ASSESSOR_SPECS, PREREQ_TYPES

#================================================================================================================================#
#=> - Constants -
#================================================================================================================================#

READABLE_PREFIX = "RESULTS_READABLE_"
OUTPUT_NAME = "RESULTS_READABLE_ALL"

SECTION_ORDER = []
for name, section, tok, cls in PREREQ_TYPES:
    SECTION_ORDER.append(section)

#================================================================================================================================#
#=> - Parse -
#================================================================================================================================#

def parse_readable_file (path):
    sections = {}
    cur_section = None
    with open(path, "r") as ptr:
        for raw in ptr:
            line = raw.rstrip("\n")
            stripped = line.strip()
            if stripped == "":
                continue
            if stripped in SECTION_ORDER:
                cur_section = stripped
                if cur_section not in sections:
                    sections[cur_section] = {}
                continue
            if cur_section is None:
                continue
            idx = line.find(">")
            if idx < 0:
                continue
            left = line[:idx].strip()
            right = line[idx + 1:].strip()
            items = []
            if right != "":
                for part in right.split(","):
                    item = part.strip()
                    if item != "":
                        items.append(item)
            sections[cur_section][left] = items
    return sections

def merge_item_lists (dst, src):
    seen = set(dst)
    for item in src:
        if item not in seen:
            dst.append(item)
            seen.add(item)

def merge_sections (merged, part):
    for section, rows in part.items():
        if section not in merged:
            merged[section] = {}
        for prereq, items in rows.items():
            if prereq not in merged[section]:
                merged[section][prereq] = []
            merge_item_lists(merged[section][prereq], items)

#================================================================================================================================#
#=> - Write -
#================================================================================================================================#

def format_section (out, section, rows):
    if len(rows) == 0:
        return
    names = list(rows.keys())
    pad = 0
    for nm in names:
        ln = len(nm)
        if ln > pad:
            pad = ln
    pad = pad + 1
    out.write("%s\n\n" % section)
    for prereq in sorted(names):
        items = rows[prereq]
        if len(items) == 0:
            continue
        rhs = ", ".join(items)
        out.write("%-*s> %s\n" % (pad, prereq, rhs))
    out.write("\n")

def write_merged (path, merged):
    with open(path, "w") as out:
        first = True
        for section in SECTION_ORDER:
            if section not in merged:
                continue
            if len(merged[section]) == 0:
                continue
            if not first:
                out.write("\n")
            first = False
            format_section(out, section, merged[section])

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(this_dir)

    merged = {}
    used = 0
    for output_prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        macro = output_prefix.upper()
        path = READABLE_PREFIX + macro
        if not os.path.isfile(path):
            print("WARN: missing", path)
            continue
        part = parse_readable_file(path)
        merge_sections(merged, part)
        used += 1
        print("read:", path)

    if used == 0:
        print("ERROR: no readable result files found")
        sys.exit(1)

    write_merged(OUTPUT_NAME, merged)
    print("wrote:", OUTPUT_NAME)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
