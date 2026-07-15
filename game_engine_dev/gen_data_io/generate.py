#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data_io"))
from generater_commons import path_from_stem

#================================================================================================================================#
#=> - Parser specs and ref-parser metadata -
#================================================================================================================================#

PARSER_SPECS = []
PARSER_SPECS.append(("building", "Building", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))
PARSER_SPECS.append(("city_flag", "CityFlag", ""))
PARSER_SPECS.append(("civ", "Civ", "traits,1,CivTraitStruct"))
PARSER_SPECS.append(("civ_trait", "CivTrait", ""))
PARSER_SPECS.append(("mvt_cost", "MvtCost", "cost,1,u16"))
PARSER_SPECS.append(("resource", "Resource", "food,1,u16:shields,2,u16:commerce,3,u16:reqs,4,ItemReqsStruct:res_dist_idx,0,ResDistIdx"))
PARSER_SPECS.append(("res_dist", "ResDist", "plc,1,ResPlacement"))
PARSER_SPECS.append(("small_wonder", "SmallWonder", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))
PARSER_SPECS.append(("tech", "Tech", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStructOpt"))
PARSER_SPECS.append(("unit", "Unit", "type,1,UnitType:cost,2,u32:attack,3,u16:defense,4,u16:mvt_pts,5,u16:sight,6,u16:reqs,7,ItemReqsStruct"))
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
        path_fn = "get_path_to_" + path_from_stem(prefix)
        deps.append((prefix, path_fn, "%s_name_to_idx" % prefix, derive_req_type(prefix)))
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
    elif output_type == "ItemEffectsStructOpt":
        return "parse_item_effects_optional"
    elif output_type == "CivTraitStruct":
        return "parse_civ_traits"
    elif output_type == "ResPlacement":
        return "parse_res_placement"
    else:
        return None
        
def derive_parsing_lines(parsing_instructions):
    parsing_lines = []
    if parsing_instructions is None or len(parsing_instructions) == 0:
        return ["// No parsing instructions provided"]
    for instruction in parsing_instructions.split(":"):
        member, idx, data_type = [part.strip() for part in instruction.strip().split(",")]
        if data_type == "ResPlacement":
            parsing_lines.append("parsed_data[i].has_plc = parse_res_placement(line_items, parsed_data[i].plc) ? 1 : 0;")
            continue
        if data_type == "ResDistIdx":
            parsing_lines.append("parsed_data[i].res_dist_idx = m_name_to_idx_cbs.res_dist_name_to_idx(get_names().get_string_content(i));")
            continue
        function_name = get_function_name_from_output_type(data_type)
        parsing_lines.append("parsed_data[i].%s = %s(line_items, %d);" %(member, function_name, int(idx)))
    return parsing_lines

def derive_member_print_lines(parsing_instructions):
    lines = []
    if parsing_instructions is None or len(parsing_instructions) == 0:
        return ["// No parsing instructions provided"]
    for instruction in parsing_instructions.split(":"):
        mem, idx, data_type = [part.strip() for part in instruction.strip().split(",")]
        if data_type in ["u16", "UnitType", "ResDistIdx"]:
            lines.append('pr_u16("%s", item.%s);' % (mem, mem))
        elif data_type == "u32":
            lines.append('pr_u32("%s", item.%s);' % (mem, mem))
        elif data_type == "ItemReqsStruct":
            lines.append('pr_reqs("%s", item.%s);' % (mem, mem))
        elif data_type in ["ItemEffectsStruct", "ItemEffectsStructOpt"]:
            lines.append('pr_fx("%s", item.%s);' % (mem, mem))
        elif data_type == "CivTraitStruct":
            lines.append('pr_traits("%s", item.%s);' % (mem, mem))
        elif data_type == "ResPlacement":
            lines.append('pr_plc(item.has_plc, item.plc);')
    return lines

def derive_extra_includes(parsing_instructions):
    if parsing_instructions and "ResPlacement" in parsing_instructions:
        return '#include "res_placement.h"'
    return ""

def derive_extra_pr_decl(parsing_instructions):
    if parsing_instructions and "ResPlacement" in parsing_instructions:
        return "void pr_plc (u8 has_plc, const ResPlacement& plc);"
    return ""

def derive_extra_pr_impl(class_name, parsing_instructions):
    if not parsing_instructions or "ResPlacement" not in parsing_instructions:
        return ""
    return (
        "void %sParserTester::pr_plc (u8 has_plc, const ResPlacement& plc) {\n"
        "    fprintf(out(), \"  has_plc: %%u\\n\", has_plc);\n"
        "    if (!has_plc) return;\n"
        "    fprintf(out(), \"  res_wt: %%u\\n\", plc.m_res_wt);\n"
        "    fprintf(out(), \"  quad_n: %%u\\n\", plc.m_quad_n);\n"
        "    for (u32 qi = 0; qi < plc.m_quad_n; ++qi) {\n"
        "        fprintf(out(), \"  quad[%%u]: terr=%%u clim=%%u ov=%%u wt=%%u\\n\", qi,\n"
        "            plc.m_quads[qi].m_terr, plc.m_quads[qi].m_clim, plc.m_quads[qi].m_ov, plc.m_quads[qi].m_wt);\n"
        "    }\n"
        "}" % class_name
    )

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
        lines.append("ld_sm(%s_items, paths.%s());" % (prefix, path_fn))
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
        lines.append('        fprintf(out(), "    [%%u] type=%%u %%s (%%u)", j, type, m_%s_sd->get_name(%s::from_raw(idx)), idx);' % (prefix, key_name))
        lines.append("    } else if (m_%s_psr != NULL) {" % prefix)
        lines.append('        fprintf(out(), "    [%%u] type=%%u %%s (%%u)", j, type, m_%s_psr->idx_to_name(idx), idx);' % prefix)
        lines.append("    }")
    return lines

def derive_comp_compile_holders():
    lines = ["g++ $INC -c ../misc/static_string_pool.cpp -o static_string_pool.o"]
    lines.extend(["g++ $INC -c ../static_state/%s_static_data.cpp -o %s_static_data.o" % (prefix, prefix) for prefix, class_name, parsing_instructions in PARSER_SPECS])
    return lines

def derive_comp_link_holders():
    lines = ["static_string_pool.o \\"]
    lines.extend(["%s_static_data.o \\" % prefix for prefix, class_name, parsing_instructions in PARSER_SPECS])
    return lines

def derive_comp_clean_holders():
    lines = ["static_string_pool.o \\"]
    lines.extend(["%s_static_data.o \\" % prefix for prefix, class_name, parsing_instructions in PARSER_SPECS])
    return lines

def get_output_filename(output_prefix, template_file):
    return template_file.replace("PREFIX", output_prefix)

def get_output_dir():
    return os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data_io"))

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
    substitution_pairs.append(("[PATH_STEM]", path_from_stem(output_prefix)))
    substitution_pairs.append(("[MACRO_TAG]", output_prefix.upper()))
    substitution_pairs.append(("[STRUCT_TAG]", struct_name))
    substitution_pairs.append(("[PARSE_TAG]", "\n        ".join(derive_parsing_lines(parsing_instructions))))
    substitution_pairs.append(("[MEMBER_PRINT_TAG]", "\n    ".join(derive_member_print_lines(parsing_instructions))))
    substitution_pairs.append(("[EXTRA_INCLUDES_TAG]", derive_extra_includes(parsing_instructions)))
    substitution_pairs.append(("[EXTRA_PR_DECL_TAG]", derive_extra_pr_decl(parsing_instructions)))
    substitution_pairs.append(("[EXTRA_PR_IMPL_TAG]", derive_extra_pr_impl(class_name, parsing_instructions)))
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
        out_path = os.path.join(get_output_dir(), output_file)
        with open(out_path, "w") as ptr:
            ptr.write(content)
        if output_file.endswith("_parser_comp"):
            os.chmod(out_path, 0o755)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
