#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os

#================================================================================================================================#
#=> - Parser specs and ref-parser metadata -
#================================================================================================================================#

PARSER_SPECS = []
PARSER_SPECS.append(("building", "Building", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))
PARSER_SPECS.append(("city_flag", "CityFlag", ""))
PARSER_SPECS.append(("civ", "Civ", "traits,1,CivTraitStruct"))
PARSER_SPECS.append(("civ_trait", "CivTrait", ""))
PARSER_SPECS.append(("resource", "Resource", "food,1,u16:shields,2,u16:commerce,3,u16:reqs,4,ItemReqsStruct"))
PARSER_SPECS.append(("small_wonder", "SmallWonder", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))
PARSER_SPECS.append(("tech", "Tech", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))
PARSER_SPECS.append(("unit", "Unit", "type,1,UnitType:cost,2,u32:attack,3,u16:defense,4,u16:mvt_pts,5,u16:reqs,6,ItemReqsStruct"))
PARSER_SPECS.append(("unit_action", "UnitAction", ""))
PARSER_SPECS.append(("unit_type", "UnitType", ""))
PARSER_SPECS.append(("wonder", "Wonder", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))

def derive_req_type(prefix):
    if prefix == "city_flag":
        return "ITEM_REQ_TYPE_FLAG"
    req_type = "ITEM_REQ_TYPE_" + prefix.upper()
    if req_type in ("ITEM_REQ_TYPE_BUILDING", "ITEM_REQ_TYPE_CIV", "ITEM_REQ_TYPE_RESOURCE", "ITEM_REQ_TYPE_TECH"):
        return req_type
    return None

def derive_ref_parser_deps():
    deps = []
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        deps.append((prefix, "get_path_to_%ss" % prefix, "%s_name_to_idx" % prefix, derive_req_type(prefix)))
    return deps

REF_PARSER_DEPS = derive_ref_parser_deps()

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
            lines.append('pr_u16("%s", item.%s);' % (mem, mem))
        elif data_type == "u32":
            lines.append('pr_u32("%s", item.%s);' % (mem, mem))
        elif data_type == "ItemReqsStruct":
            lines.append('pr_reqs("%s", item.%s);' % (mem, mem))
        elif data_type == "ItemEffectsStruct":
            lines.append('pr_fx("%s", item.%s);' % (mem, mem))
        elif data_type == "CivTraitStruct":
            lines.append('pr_traits("%s", item.%s);' % (mem, mem))
    return lines

def derive_dep_psr_members():
    lines = []
    for prefix, path_fn, cb_field, req_type in REF_PARSER_DEPS:
        lines.append("const DataParserBase* m_%s_psr;" % prefix)
    return lines

def derive_dep_n2i_decls():
    lines = []
    for prefix, path_fn, cb_field, req_type in REF_PARSER_DEPS:
        lines.append("static u16 st_%s_n2i (cstr name);" % prefix)
    return lines

def derive_dep_init():
    parts = []
    for prefix, path_fn, cb_field, req_type in REF_PARSER_DEPS:
        parts.append("m_%s_psr(NULL)" % prefix)
    return ",\n    ".join(parts)

def derive_dep_n2i_impl(class_tag):
    lines = []
    for prefix, path_fn, cb_field, req_type in REF_PARSER_DEPS:
        lines.append("u16 %sParserTester::st_%s_n2i (cstr name) {" % (class_tag, prefix))
        lines.append("    if (s_inst == NULL || s_inst->m_%s_psr == NULL) {" % prefix)
        lines.append("        return U16_KEY_NULL;")
        lines.append("    }")
        lines.append("    return s_inst->m_%s_psr->name_to_idx(name);" % prefix)
        lines.append("}")
        lines.append("")
    return lines

def derive_dep_cbs_wire():
    lines = []
    for prefix, path_fn, cb_field, req_type in REF_PARSER_DEPS:
        lines.append("cbs.%s = st_%s_n2i;" % (cb_field, prefix))
    return lines

def derive_dep_items_decl():
    lines = []
    for prefix, path_fn, cb_field, req_type in REF_PARSER_DEPS:
        lines.append("StringManager %s_items;" % prefix)
    return lines

def derive_dep_ld():
    lines = []
    for prefix, path_fn, cb_field, req_type in REF_PARSER_DEPS:
        lines.append("ld_sm(%s_items, paths.%s().c_str());" % (prefix, path_fn))
    return lines

def derive_dep_parser_decl():
    lines = []
    for prefix, path_fn, cb_field, req_type in REF_PARSER_DEPS:
        lines.append("DataParserBase %s_parser(%s_items, cbs);" % (prefix, prefix))
    return lines

def derive_dep_psr_assign():
    lines = []
    for prefix, path_fn, cb_field, req_type in REF_PARSER_DEPS:
        lines.append("m_%s_psr = &%s_parser;" % (prefix, prefix))
    return lines

def static_data_class_name(class_name):
    return class_name + "StaticData"

def static_data_key_name(class_name):
    return class_name + "StaticDataKey"

def derive_dep_sd_includes():
    lines = []
    seen = set()
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        hdr = "%s_static_data.h" % prefix
        if hdr in seen:
            continue
        seen.add(hdr)
        lines.append('#include "%s"' % hdr)
    return lines

def derive_dep_sd_members():
    lines = []
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        lines.append("const %s* m_%s_sd;" % (static_data_class_name(class_name), prefix))
    return lines

def derive_dep_sd_setters_decl():
    lines = []
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        sd = static_data_class_name(class_name)
        lines.append("void set_%s_sd (const %s* sd);" % (prefix, sd))
    return lines

def derive_dep_sd_setters_impl(class_tag):
    blocks = []
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        sd = static_data_class_name(class_name)
        blocks.append("void %sParserTester::set_%s_sd (const %s* sd) {" % (class_tag, prefix, sd))
        blocks.append("    m_%s_sd = sd;" % prefix)
        blocks.append("}")
    return blocks

def derive_dep_sd_init():
    parts = []
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        parts.append("m_%s_sd(NULL)" % prefix)
    return ",\n    ".join(parts)

def derive_dep_reqs_print():
    prefix_to_class = {}
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        prefix_to_class[prefix] = class_name
    lines = []
    first = True
    for prefix, path_fn, cb_field, req_type in REF_PARSER_DEPS:
        if req_type is None:
            continue
        class_name = prefix_to_class[prefix]
        key_name = static_data_key_name(class_name)
        if first:
            lines.append("if (type == %s) {" % req_type)
            first = False
        else:
            lines.append("} else if (type == %s) {" % req_type)
        lines.append("    if (m_%s_sd != NULL && idx < m_%s_sd->get_item_count()) {" % (prefix, prefix))
        lines.append('        fprintf(out(), "    [%%u] type=%%u %%s (%%u)", j, type, m_%s_sd->get_item(%s::from_raw(idx)).name.c_str(), idx);' % (prefix, key_name))
        lines.append("    } else if (m_%s_psr != NULL) {" % prefix)
        lines.append('        fprintf(out(), "    [%%u] type=%%u %%s (%%u)", j, type, m_%s_psr->idx_to_name(idx).c_str(), idx);' % prefix)
        lines.append("    }")
    return lines

def derive_comp_compile_holders():
    return ["g++ $INC -c ../static_state/%s_static_data.cpp -o %s_static_data.o" % (prefix, prefix) for prefix, class_name, parsing_instructions in PARSER_SPECS]

def derive_comp_link_holders():
    return ["%s_static_data.o \\" % prefix for prefix, class_name, parsing_instructions in PARSER_SPECS]

def derive_comp_clean_holders():
    return ["%s_static_data.o \\" % prefix for prefix, class_name, parsing_instructions in PARSER_SPECS]

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
    substitution_pairs.append(("[MACRO_TAG]", output_prefix.upper()))
    substitution_pairs.append(("[STRUCT_TAG]", struct_name))
    substitution_pairs.append(("[PARSE_TAG]", "\n        ".join(derive_parsing_lines(parsing_instructions))))
    substitution_pairs.append(("[MEMBER_PRINT_TAG]", "\n    ".join(derive_member_print_lines(parsing_instructions))))
    substitution_pairs.append(("[DEP_SD_INCLUDES_TAG]", "\n".join(derive_dep_sd_includes())))
    substitution_pairs.append(("[DEP_SD_SETTERS_DECL_TAG]", "\n    ".join(derive_dep_sd_setters_decl())))
    substitution_pairs.append(("[DEP_SD_SETTERS_IMPL_TAG]", "\n\n".join(derive_dep_sd_setters_impl(class_name))))
    substitution_pairs.append(("[DEP_SD_MEMBERS_TAG]", "\n    ".join(derive_dep_sd_members())))
    substitution_pairs.append(("[DEP_SD_INIT_TAG]", derive_dep_sd_init()))
    substitution_pairs.append(("[DEP_PSR_MEMBERS_TAG]", "\n    ".join(derive_dep_psr_members())))
    substitution_pairs.append(("[DEP_N2I_DECL_TAG]", "\n    ".join(derive_dep_n2i_decls())))
    substitution_pairs.append(("[DEP_INIT_TAG]", derive_dep_init()))
    substitution_pairs.append(("[DEP_N2I_IMPL_TAG]", "\n".join(derive_dep_n2i_impl(class_name))))
    substitution_pairs.append(("[DEP_CBS_WIRE_TAG]", "\n    ".join(derive_dep_cbs_wire())))
    substitution_pairs.append(("[DEP_ITEMS_DECL_TAG]", "\n    ".join(derive_dep_items_decl())))
    substitution_pairs.append(("[DEP_LD_TAG]", "\n    ".join(derive_dep_ld())))
    substitution_pairs.append(("[DEP_PARSER_DECL_TAG]", "\n    ".join(derive_dep_parser_decl())))
    substitution_pairs.append(("[DEP_PSR_ASSIGN_TAG]", "\n    ".join(derive_dep_psr_assign())))
    substitution_pairs.append(("[DEP_REQS_PRINT_TAG]", "\n        ".join(derive_dep_reqs_print())))
    substitution_pairs.append(("[DEP_COMP_COMPILE_HOLDERS_TAG]", "\n".join(derive_comp_compile_holders())))
    substitution_pairs.append(("[DEP_COMP_LINK_HOLDERS_TAG]", "\n    ".join(derive_comp_link_holders())))
    substitution_pairs.append(("[DEP_COMP_CLEAN_HOLDERS_TAG]", "\n    ".join(derive_comp_clean_holders())))

    template_files = []
    template_files.append("PREFIX_parser.cpp")
    template_files.append("PREFIX_parser.h")
    template_files.append("PREFIX_parser_tester.h")
    template_files.append("PREFIX_parser_tester.cpp")
    template_files.append("PREFIX_parser_tester_drv.cpp")
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
