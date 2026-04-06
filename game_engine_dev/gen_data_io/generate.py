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
    if output_type == "u32":
        return "parse_u32"
    elif output_type == "ItemReqsStruct":
        return "parse_item_reqs"
    elif output_type == "ItemEffectsStruct":
        return "parse_item_effects"
    else:
        return None

def derive_parsing_lines(parsing_instructions):
    parsing_lines = []
    for instruction in parsing_instructions.split(":"):
        member, idx, data_type = [part.strip() for part in instruction.strip().split(",")]
        function_name = get_function_name_from_output_type(data_type)
        parsing_lines.append("parsed_data[i].%s = %s(line_items, %d);" %(member, function_name, int(idx)))
    return parsing_lines

def derive_print_lines(parsing_instructions):
    lines = []
    for instruction in parsing_instructions.split(":"):
        mem, idx, data_type = [part.strip() for part in instruction.strip().split(",")]
        if data_type == "u32":
            lines.append('printf("  %s: %%u\\n", static_cast<unsigned int>(item.%s));' % (mem, mem))
        elif data_type == "ItemReqsStruct":
            lines.append('printf("  %s:\\n");' % mem)
            lines.append("for (u32 j = 0; j < MAX_PREREQ_COUNT; ++j) {")
            lines.append("    if (item.%s.types[j] == ITEM_REQ_TYPE_NONE) {" % mem)
            lines.append("        continue;")
            lines.append("    }")
            lines.append("    const u16 idx = item.%s.indices[j];" % mem)
            lines.append("    const u8 type = item.%s.types[j];" % mem)
            lines.append("    const std::string req_name = req_idx_to_name(type, idx);")
            lines.append('    printf("    [%u] type=%u %s (%u)", j, type, req_name.c_str(), idx);')
            lines.append("    if (item.%s.added_args[j] != 0) {" % mem)
            lines.append('        printf(" arg=%%u", item.%s.added_args[j]);' % mem)
            lines.append("    }")
            lines.append('    printf("\\n");')
            lines.append("}")
        elif data_type == "ItemEffectsStruct" and 0:
            lines.append('printf("  %s:\\n");' % mem)
            lines.append("for (u32 j = 0; j < MAX_EFFECT_COUNT; ++j) {")
            lines.append('    printf("    [%%u] type=%%u idx=%%u\\n", j, item.%s.types[j], item.%s.indices[j]);' % (mem, mem))
            lines.append("}")
    return lines

def get_output_filename(output_prefix, template_file):
    return template_file.replace("PREFIX", output_prefix)

def apply_substitution(content, old_string, new_string):
    return content.replace(old_string, new_string)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":

    if len(sys.argv) != 5:
        print("Usage: python generate.py <output prefix> <class name> <struct name> <parsing instructions>")
        sys.exit(1)
    
    output_prefix = sys.argv[1]
    class_name = sys.argv[2]
    struct_name = sys.argv[3]
    parsing_instructions = sys.argv[4]
    substitution_pairs = []
    substitution_pairs.append(("[CLASS_TAG]", class_name))
    substitution_pairs.append(("[FILE_TAG]", output_prefix.lower()))
    substitution_pairs.append(("[STRUCT_TAG]", struct_name))
    substitution_pairs.append(("[PARSE_TAG]", "\n        ".join(derive_parsing_lines(parsing_instructions))))
    substitution_pairs.append(("[PRINT_TAG]", "\n    ".join(derive_print_lines(parsing_instructions))))

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
