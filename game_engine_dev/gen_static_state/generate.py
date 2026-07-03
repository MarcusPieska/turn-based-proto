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

def get_template_dir():
    return os.path.dirname(os.path.abspath(__file__))

def get_static_state_dir():
    return os.path.normpath(os.path.join(get_template_dir(), "..", "static_state"))

def get_static_variable_state_dir():
    return os.path.normpath(os.path.join(get_template_dir(), "..", "static_variable_state"))

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

    template_dir = get_template_dir()
    static_state_dir = get_static_state_dir()
    static_variable_state_dir = get_static_variable_state_dir()
    os.makedirs(static_state_dir, exist_ok=True)
    os.makedirs(static_variable_state_dir, exist_ok=True)

    substitution_pairs = []
    substitution_pairs.append(("[CLASS_NAME]", class_name + "StaticData"))
    substitution_pairs.append(("[DATA_KEY]", class_name + "StaticDataKey"))
    substitution_pairs.append(("[MACRO_PREFIX]", output_prefix.upper()))
    substitution_pairs.append(("[MEMBER_TAG]", output_prefix.lower()))
    substitution_pairs.append(("[DATA_STRUCT]", class_name + "StaticDataStruct"))
    substitution_pairs.append(("[BIT_ARRAY_CLASS]", class_name + "BitArray"))
    substitution_pairs.append(("[STRUCT_MEMBERS]", struct_members))
    substitution_pairs.append(("[STRUCT_MEMBER1]", struct_member1))
    substitution_pairs.append(("[STRUCT_MEMBER2]", struct_member2))

    template_files = []
    template_files.append("PREFIX_static_data.cpp")
    template_files.append("PREFIX_static_data_load.cpp")
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
        with open(os.path.join(template_dir, template_file), "r") as ptr:
            output_file_contents.append(ptr.read())

    for i, content in enumerate(output_file_contents):
        for old_string, new_string in substitution_pairs:
            content = apply_substitution(content, old_string, new_string)
        output_file_contents[i] = content

    for output_file, content in zip(output_files, output_file_contents):
        out_path = os.path.join(static_state_dir, output_file)
        with open(out_path, "w") as ptr:
            ptr.write(content)
        if output_file.endswith("_static_data_comp"):
            os.chmod(out_path, 0o755)

    bit_array_templates = []
    bit_array_templates.append("PREFIX_bit_array.cpp")
    bit_array_templates.append("PREFIX_bit_array.h")
    bit_array_templates.append("PREFIX_bit_array_tester.cpp")
    bit_array_templates.append("PREFIX_bit_array_comp")

    bit_array_outputs = []
    for template_file in bit_array_templates:
        bit_array_outputs.append(get_output_filename(output_prefix, template_file))

    bit_array_contents = []
    for template_file in bit_array_templates:
        with open(os.path.join(template_dir, template_file), "r") as ptr:
            bit_array_contents.append(ptr.read())

    for i, content in enumerate(bit_array_contents):
        for old_string, new_string in substitution_pairs:
            content = apply_substitution(content, old_string, new_string)
        bit_array_contents[i] = content

    for output_file, content in zip(bit_array_outputs, bit_array_contents):
        out_path = os.path.join(static_variable_state_dir, output_file)
        with open(out_path, "w") as ptr:
            ptr.write(content)
        if output_file.endswith("_bit_array_comp"):
            os.chmod(out_path, 0o755)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
