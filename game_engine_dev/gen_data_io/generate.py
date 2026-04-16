#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os

#================================================================================================================================#
#=> - Helper functions -
#================================================================================================================================#

def get_function_name_from_output_type(output_type):
    if output_type =="u16":
        return "parse_u16"
    elif output_type == "u32":
        return "parse_u32"
    elif output_type == "UnitType":
        return "parse_unit_type"
    elif output_type == "ItemReqsStruct":
        return "parse_item_reqs"
    elif output_type == "ItemEffectsStruct":
        return "parse_item_effects"
    elif output_type == "CivTraitStruct":
        return "parse_civ_traits"
    else:
        return None
        
def derive_parsing_lines(parsing_instructions):
    parsing_lines = []
    if parsing_instructions is None or len(parsing_instructions) == 0:
        return ["// No parsing instructions provided"]
    for instruction in parsing_instructions.split(":"):
        member, idx, data_type = [part.strip() for part in instruction.strip().split(",")]
        function_name = get_function_name_from_output_type(data_type)
        parsing_lines.append("parsed_data[i].%s = %s(line_items, %d);" %(member, function_name, int(idx)))
    return parsing_lines

def derive_member_print_lines(parsing_instructions):
    lines = []
    if parsing_instructions is None or len(parsing_instructions) == 0:
        return ["// No parsing instructions provided"]
    for instruction in parsing_instructions.split(":"):
        mem, idx, data_type = [part.strip() for part in instruction.strip().split(",")]
        if data_type in ["u16", "UnitType"]:
            lines.append('print_u16_member("%s", item.%s);' % (mem, mem))
        elif data_type == "u32":
            lines.append('print_u32_member("%s", item.%s);' % (mem, mem))
        elif data_type == "ItemReqsStruct":
            lines.append('print_reqs_member("%s", item.%s);' % (mem, mem))
        elif data_type == "ItemEffectsStruct":
            lines.append('print_effects_member("%s", item.%s);' % (mem, mem))
        elif data_type == "CivTraitStruct":
            lines.append('print_civ_traits_member("%s", item.%s);' % (mem, mem))
    return lines

def get_output_filename(output_prefix, template_file):
    return template_file.replace("PREFIX", output_prefix)

def apply_substitution(content, old_string, new_string):
    return content.replace(old_string, new_string)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":

    if len(sys.argv) not in [5, 4]:
        print("Usage: python generate.py <output prefix> <class name> <struct name> <parsing instructions (optional)>")
        sys.exit(1)
    
    output_prefix = sys.argv[1]
    class_name = sys.argv[2]
    struct_name = sys.argv[3]
    parsing_instructions = None
    if len(sys.argv) == 5:
        parsing_instructions = sys.argv[4]
    substitution_pairs = []
    substitution_pairs.append(("[CLASS_TAG]", class_name))
    substitution_pairs.append(("[FILE_TAG]", output_prefix.lower()))
    substitution_pairs.append(("[STRUCT_TAG]", struct_name))
    substitution_pairs.append(("[PARSE_TAG]", "\n        ".join(derive_parsing_lines(parsing_instructions))))
    substitution_pairs.append(("[MEMBER_PRINT_TAG]", "\n    ".join(derive_member_print_lines(parsing_instructions))))

    template_files = []
    template_files.append("PREFIX_parser.cpp")
    template_files.append("PREFIX_parser.h")
    template_files.append("PREFIX_parser_tester.cpp")
    template_files.append("PREFIX_parser_comp")

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
        if output_file.endswith("_parser_comp"):
            os.chmod(output_file, 0o755)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
