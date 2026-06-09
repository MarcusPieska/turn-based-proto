#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
import subprocess

#================================================================================================================================#
#=> - Globals -
#================================================================================================================================#

substitution_pairs = []
substitution_pairs.append("[CLASS_NAME_PREFIX]")
substitution_pairs.append("[MACRO_PREFIX]")
substitution_pairs.append("[MEMBER_TAG]")

template_files = []
template_files.append("PREFIX_vector.cpp")
template_files.append("PREFIX_vector.h")
template_files.append("PREFIX_vector_key.h")
template_files.append("PREFIX_vector_tester.cpp")
template_files.append("PREFIX_visual_tester.cpp")
template_files.append("PREFIX_vector_comp")

#================================================================================================================================#
#=> - Helper functions -
#================================================================================================================================#

def get_output_filename(output_prefix, template_file):
    return template_file.replace("PREFIX", output_prefix)

def build_substitution_pairs(output_prefix, class_name):
    pairs = []
    pairs.append((substitution_pairs[0], class_name))
    pairs.append((substitution_pairs[1], output_prefix.upper()))
    pairs.append((substitution_pairs[2], output_prefix.lower()))
    return pairs

def substitute_make_substitutions(content, old_string, new_string):
    return content.replace(old_string, new_string)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":

    if len(sys.argv) != 3:
        print("Usage: python generate_dyn_state.py <output prefix> <class name prefix>")
        sys.exit(1)
    
    output_prefix = sys.argv[1]
    class_name = sys.argv[2]

    pairs = build_substitution_pairs(output_prefix, class_name)

    output_files = []
    for template_file in template_files:
        output_files.append(get_output_filename(output_prefix, template_file))
    
    output_file_contents = []
    for template_file in template_files:
        with open(template_file, "r") as ptr:
            output_file_contents.append(ptr.read())

    for i, content in enumerate(output_file_contents):
        for old_string, new_string in pairs:
            content = substitute_make_substitutions(content, old_string, new_string)
        output_file_contents[i] = content

    for output_file, content in zip(output_files, output_file_contents):
        with open(output_file, "w") as ptr:
            ptr.write(content)
        if output_file.endswith("_vector_comp"):
            os.chmod(output_file, 0o755)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#

