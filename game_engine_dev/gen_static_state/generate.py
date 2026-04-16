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

    if len(sys.argv) != 6:
        print("Usage: python generate.py <output prefix> <class name> <struct members> <struct member 1> <struct member 2>")
        sys.exit(1)
    
    output_prefix = sys.argv[1]
    class_name = sys.argv[2]
    struct_member1 = sys.argv[4]
    struct_member2 = sys.argv[5]
    struct_members = []
    for member in sys.argv[3].split(";")[:-1]:
        struct_members.append("    " + member.strip() + ";")
    struct_members = "\n".join(struct_members)

    substitution_pairs = []
    substitution_pairs.append(("[CLASS_NAME]", class_name + "StaticData"))
    substitution_pairs.append(("[DATA_KEY]", class_name + "StaticDataKey"))
    substitution_pairs.append(("[MACRO_PREFIX]", output_prefix.upper()))
    substitution_pairs.append(("[MEMBER_TAG]", output_prefix.lower()))
    substitution_pairs.append(("[DATA_STRUCT]", class_name + "StaticDataStruct"))
    substitution_pairs.append(("[STRUCT_MEMBERS]", struct_members))
    substitution_pairs.append(("[STRUCT_MEMBER1]", struct_member1))
    substitution_pairs.append(("[STRUCT_MEMBER2]", struct_member2))

    template_files = []
    template_files.append("PREFIX_static_data.cpp")
    template_files.append("PREFIX_static_data.h")
    template_files.append("PREFIX_static_key.h")
    if struct_member1 != "None" and struct_member2 != "None":
        template_files.append("PREFIX_static_data_tester.cpp")
        template_files.append("PREFIX_static_data_comp")

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
        if output_file.endswith("_static_data_comp"):
            os.chmod(output_file, 0o755)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
