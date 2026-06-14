#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - Helper functions -
#================================================================================================================================#

def die(msg):
    sys.stderr.write(msg + "\n")
    sys.exit(1)

def snake_to_pascal(snake):
    parts = [p for p in snake.split("_") if p]
    if not parts:
        return ""
    return "".join(p[:1].upper() + p[1:].lower() for p in parts)

def snake_to_macro(snake):
    return snake.upper()

def apply_substitutions(content, pairs):
    for old, new in pairs:
        content = content.replace(old, new)
    return content

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    if len(sys.argv) < 2 or len(sys.argv) > 3:
        die("Usage: python boilerplate_adjustor.py <file_tag> [prefix]")
    file_tag = sys.argv[1].strip()
    if not file_tag or file_tag != sys.argv[1]:
        die("Error: file_tag must be non-empty and have no leading/trailing whitespace.")

    script_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(script_dir)

    main_class = snake_to_pascal(file_tag)
    if not main_class:
        die("Error: could not derive class name from file_tag (use segments like land_water_overlay).")
    macro_tag = snake_to_macro(file_tag)

    prefix = "adjust_"
    out_h = prefix + file_tag + ".h"
    out_cpp = prefix + file_tag + ".cpp"
    out_tester = prefix + file_tag + "_tester.cpp"
    out_comp = prefix + file_tag + "_comp"
    outputs = [out_h, out_cpp, out_tester, out_comp]

    for path in outputs:
        if os.path.lexists(path):
            die("Error: output file already exists, aborting: " + path)

    tpl_h = "BOILERPLATE_ADJUSTOR.h"
    tpl_cpp = "BOILERPLATE_ADJUSTOR.cpp"
    tpl_tester = "BOILERPLATE_ADJUSTOR_tester.cpp"
    tpl_comp = "BOILERPLATE_ADJUSTOR_comp"
    templates = [tpl_h, tpl_cpp, tpl_tester, tpl_comp]

    for path in templates:
        if not os.path.isfile(path):
            die("Error: template missing: " + path)

    pairs = [
        ("[FILE_PREFIX_TAG]", file_tag),
        ("[file_prefix_tag]", file_tag),
        ("[MAIN_CLASS_TAG]", main_class),
        ("[MACRO_TAG]", macro_tag),
    ]

    contents = []
    for path in templates:
        with open(path, "r", encoding="utf-8") as f:
            contents.append(f.read())

    for i, raw in enumerate(contents):
        contents[i] = apply_substitutions(raw, pairs)

    for path, body in zip(outputs, contents):
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            f.write(body)
        if path.endswith("_comp"):
            os.chmod(path, 0o755)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
