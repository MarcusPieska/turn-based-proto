#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True
sys.path.append(os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data_io")))

from generate_item_effects import ie_scope_enum

#================================================================================================================================#
#=> - Scope specs (from item_effects generator) -
#================================================================================================================================#

SCOPE_ENUMS = [s for s in ie_scope_enum if s != "NONE"]

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def scope_stem (scope_enum):
    return scope_enum.lower()

def scope_class (scope_enum):
    return scope_enum.capitalize() + "Effector"

def scope_guard (scope_enum):
    return scope_enum + "_EFFECTOR_H"

def apply_substitution (content, old_string, new_string):
    return content.replace(old_string, new_string)

GENERATED_FILES = []

def write_generated_file (this_dir, output_name, content):
    out_path = os.path.join(this_dir, output_name)
    with open(out_path, "w") as ptr:
        ptr.write(content)
    GENERATED_FILES.append(output_name)

def get_output_filename (output_prefix, template_file):
    return template_file.replace("PREFIX", output_prefix)

def generate_from_template (this_dir, template_name, output_name, sub_pairs):
    template_path = os.path.join(this_dir, template_name)
    with open(template_path, "r") as ptr:
        content = ptr.read()
    for old_string, new_string in sub_pairs:
        content = apply_substitution(content, old_string, new_string)
    write_generated_file(this_dir, output_name, content)

def generate_effect_map (this_dir):
    generate_from_template(this_dir, "TEMPLATE_effect_map.h", "effect_map.h", [])

def generate_scope_effector (this_dir, scope_enum):
    stem = scope_stem(scope_enum)
    sub_pairs = []
    sub_pairs.append(("[FILE_TAG]", stem))
    sub_pairs.append(("[SCOPE_ENUM]", scope_enum))
    sub_pairs.append(("[SCOPE_CLASS]", scope_class(scope_enum)))
    sub_pairs.append(("[SCOPE_GUARD]", scope_guard(scope_enum)))
    template_files = []
    template_files.append("PREFIX_effector.h")
    template_files.append("PREFIX_effector.cpp")
    for template_file in template_files:
        output_name = get_output_filename(stem, template_file)
        generate_from_template(this_dir, template_file, output_name, sub_pairs)

def generate_all_files ():
    this_dir = os.path.dirname(os.path.abspath(__file__))
    GENERATED_FILES.clear()
    generate_effect_map(this_dir)
    for scope_enum in SCOPE_ENUMS:
        generate_scope_effector(this_dir, scope_enum)
    return list(GENERATED_FILES)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    files = generate_all_files()
    for name in files:
        print("generated:", name)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
