#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os

#================================================================================================================================#
#=> - Helper functions -
#================================================================================================================================#

def get_output_filename(output_prefix, template_file):
    return template_file.replace("PREFIX", output_prefix)

def apply_substitution(content, old_string, new_string):
    return content.replace(old_string, new_string)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":

    if len(sys.argv) != 4:
        print("Usage: python generate.py <output prefix> <class name prefix> <user class name>")
        sys.exit(1)
    
    output_prefix = sys.argv[1]
    class_name = sys.argv[2]
    user_class_name = sys.argv[3]

    substitution_pairs = []
    substitution_pairs.append(("[CLASS_NAME_PREFIX]", class_name))
    substitution_pairs.append(("[MACRO_PREFIX]", output_prefix.upper()))
    substitution_pairs.append(("[MEMBER_TAG]", output_prefix.lower()))
    substitution_pairs.append(("[USER_CLASS_NAME]", user_class_name))

    template_files = []
    template_files.append("PREFIX_bit_bank.cpp")
    template_files.append("PREFIX_bit_bank.h")
    template_files.append("PREFIX_bit_bank_tester.cpp")
    template_files.append("PREFIX_bit_bank_major1_tester.cpp")
    template_files.append("PREFIX_bit_bank_major2_tester.cpp")
    template_files.append("PREFIX_bit_bank_visual1_tester.cpp")
    template_files.append("PREFIX_bit_bank_comp")

    output_files = []
    for template_file in template_files:
        output_files.append(get_output_filename(output_prefix, template_file))
    
    output_file_contents = []
    for template_file in template_files:
        with open(template_file, "r") as ptr:
            output_file_contents.append(ptr.read())

    for i, content in enumerate(output_file_contents):
        for old_string, new_string in substitution_pairs:
            content = apply_substitution(content, old_string, new_string)
        output_file_contents[i] = content

    for output_file, content in zip(output_files, output_file_contents):
        with open(output_file, "w") as ptr:
            ptr.write(content)
        if output_file.endswith("_bit_bank_comp"):
            os.chmod(output_file, 0o755)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
