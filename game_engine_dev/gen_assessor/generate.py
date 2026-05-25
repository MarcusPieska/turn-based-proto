#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
import importlib.util

#================================================================================================================================#
#=> - Parser specs (single source: gen_data_io/generate.py) -
#================================================================================================================================#

def _load_gen_data_io_generate ():
    base = os.path.dirname(os.path.abspath(__file__))
    path = os.path.normpath(os.path.join(base, "..", "gen_data_io", "generate.py"))
    mod_name = "gen_data_io_generate"
    spec = importlib.util.spec_from_file_location(mod_name, path)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod

PARSER_SPECS = _load_gen_data_io_generate().PARSER_SPECS

#================================================================================================================================#
#=> - Prereq types (names match parsers / static_state / item_reqs) -
#================================================================================================================================#

# Order is codegen order (switch cases, AssessorCtx members); put common types first.
PREREQ_NAMES = ["tech", "resource", "building", "city_flag", "civ"]

def prereq_section (name):
    if name == "city_flag":
        return "FLAGS"
    return name.upper() + "S"

def prereq_tok (name):
    if name == "city_flag":
        return "flag"
    return name

def prereq_class (name):
    parts = name.split("_")
    return "".join(p.capitalize() for p in parts)

def item_req_type_enum (name):
    suffix = name.upper()
    if name == "city_flag":
        suffix = "FLAG"
    return "ITEM_REQ_TYPE_" + suffix

def derive_prereq_types ():
    rows = []
    for name in PREREQ_NAMES:
        rows.append((name, prereq_section(name), prereq_tok(name), prereq_class(name)))
    return rows

PREREQ_TYPES = derive_prereq_types()

#================================================================================================================================#
#=> - Assessor specs (types with ItemReqsStruct) -
#================================================================================================================================#

def has_item_reqs (prefix, parsing_instructions):
    if prefix == "city_flag":
        return True
    if parsing_instructions is None:
        return False
    return "ItemReqsStruct" in parsing_instructions

def derive_assessor_specs ():
    specs = []
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        if has_item_reqs(prefix, parsing_instructions):
            specs.append((prefix, class_name, parsing_instructions))
    return specs

ASSESSOR_SPECS = derive_assessor_specs()

GAME_CONFIG_PREFIX = "../game_config."

def game_config_plural (prefix):
    if prefix == "tech":
        return "techs"
    return prefix + "s"

def derive_game_config_paths ():
    paths = {}
    for prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        paths[prefix] = GAME_CONFIG_PREFIX + game_config_plural(prefix)
    return paths

GAME_CONFIG_PATHS = derive_game_config_paths()

#================================================================================================================================#
#=> - Shared prereq codegen -
#================================================================================================================================#

def prereq_types_excluding (skip_name):
    out = []
    for row in PREREQ_TYPES:
        if row[0] != skip_name:
            out.append(row)
    return out

def join_tag (lines, cont="    "):
    if cont == "":
        return "\n".join(lines)
    return ("\n" + cont).join(lines)

def derive_inferred_reqs_arrays ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append("u16 m_%s[BRUTE_MAX_PER_TYPE];" % name)
    return join_tag(lines)

def derive_inferred_reqs_counts ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append("u8 m_%s_n;" % name)
    return join_tag(lines)

def derive_enables_map_slots ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append("EnSlot* m_%s;" % name)
    return join_tag(lines)

def derive_enables_map_counts ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append("u32 m_%s_count;" % name)
    return join_tag(lines)

def derive_set_all_params ():
    parts = []
    for name, section, tok, cls in PREREQ_TYPES:
        parts.append("BitArrayCL& %s" % name)
    parts.append("const StaticParsingManager& mgr")
    parts.append("bool building_all")
    return ", ".join(parts)

def derive_set_all_call (mgr_expr="mgr", building_all_expr="building_for_ablation"):
    parts = []
    for name, section, tok, cls in PREREQ_TYPES:
        parts.append(name)
    parts.append(mgr_expr)
    parts.append(building_all_expr)
    return ", ".join(parts)

def derive_set_all_bits ():
    lines = []
    for name, section, tok, cls in prereq_types_excluding("building"):
        lines.append("u32 %s_count = mgr.get_%s_count();" % (name, name))
        lines.append("for (u32 i = 0; i < %s_count; ++i) {" % name)
        lines.append("    %s.set_bit(i);" % name)
        lines.append("}")
    lines.append("if (building_all) {")
    lines.append("    u32 building_count = mgr.get_building_count();")
    lines.append("    for (u32 i = 0; i < building_count; ++i) {")
    lines.append("        building.set_bit(i);")
    lines.append("    }")
    lines.append("}")
    return join_tag(lines)

def derive_free_en_map ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append("delete[] en.m_%s;" % name)
        lines.append("en.m_%s = nullptr;" % name)
    return join_tag(lines)

def derive_make_en_map_args ():
    parts = []
    for name, section, tok, cls in PREREQ_TYPES:
        parts.append("u32 %s_count" % name)
    return ", ".join(parts)

def derive_make_en_map_init ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append("en.m_%s_count = %s_count;" % (name, name))
        lines.append("en.m_%s = new EnSlot[%s_count];" % (name, name))
        lines.append("for (u32 i = 0; i < %s_count; ++i) {" % name)
        lines.append("    en.m_%s[i] = {};" % name)
        lines.append("}")
    return join_tag(lines)

def derive_make_en_map_call ():
    parts = []
    for name, section, tok, cls in PREREQ_TYPES:
        parts.append("%s_count" % name)
    return ", ".join(parts)

def derive_run_local_counts ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append("u32 %s_count = mgr.get_%s_count();" % (name, name))
    return join_tag(lines)

def derive_run_bitarray_decl ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append("BitArrayCL %s(%s_count);" % (name, name))
    return join_tag(lines)

def derive_run_ctx_snap ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append("ctx.m_%s = &%s;" % (name, name))
    return join_tag(lines, "        ")

def derive_ablation_loop_lines (name):
    lines = []
    lines.append("for (u32 %s_i = 0; %s_i < %s_count; ++%s_i) {" % (name, name, name, name))
    lines.append("    set_base();")
    lines.append("    %s.clear_bit(%s_i);" % (name, name))
    lines.append("    snap(ctx);")
    lines.append("    BitArrayCL* cur = GeneralAssessor::assess(cfg.m_item_count, cfg.m_reqs, ctx);")
    lines.append("    for (u16 i = 0; i < cfg.m_item_count; ++i) {")
    lines.append("        if (baseline->get_bit(i) == 1 && cur->get_bit(i) != 1) {")
    lines.append("            add_u16(inferred[i].m_%s, &inferred[i].m_%s_n, (u16)%s_i);" % (name, name, name))
    lines.append("            add_en(en.m_%s[%s_i], i);" % (name, name))
    lines.append("        }")
    lines.append("    }")
    lines.append("    delete cur;")
    lines.append("}")
    return lines

def derive_ablation_loops ():
    lines = []
    for name, section, tok, cls in prereq_types_excluding("building"):
        lines.extend(derive_ablation_loop_lines(name))
    return join_tag(lines)

def derive_building_isolate ():
    lines = []
    lines.append("if (cfg.m_building_isolate) {")
    lines.append("    for (u32 b = 0; b < building_count; ++b) {")
    lines.append("        set_all(%s);" % derive_set_all_call("mgr", "true"))
    lines.append("        for (u32 i = 0; i < building_count; ++i) {")
    lines.append("            building.set_bit(i);")
    lines.append("        }")
    lines.append("        building.clear_bit(b);")
    lines.append("        snap(ctx);")
    lines.append("        BitArrayCL* bl = GeneralAssessor::assess(cfg.m_item_count, cfg.m_reqs, ctx);")
    lines.append("        for (u32 bb = 0; bb < building_count; ++bb) {")
    lines.append("            if (bb == b) {")
    lines.append("                continue;")
    lines.append("            }")
    lines.append("            building.clear_bit(bb);")
    lines.append("            snap(ctx);")
    lines.append("            BitArrayCL* cur = GeneralAssessor::assess(cfg.m_item_count, cfg.m_reqs, ctx);")
    lines.append("            if (bl->get_bit(b) == 1 && cur->get_bit(b) != 1) {")
    lines.append("                add_u16(inferred[b].m_building, &inferred[b].m_building_n, (u16)bb);")
    lines.append("                add_en(en.m_building[bb], b);")
    lines.append("            }")
    lines.append("            delete cur;")
    lines.append("            building.set_bit(bb);")
    lines.append("        }")
    lines.append("        delete bl;")
    lines.append("    }")
    lines.append("} else if (cfg.m_init_building_all) {")
    for ln in derive_ablation_loop_lines("building"):
        lines.append("    " + ln)
    lines.append("}")
    return join_tag(lines)

def derive_prereq_nm_funcs ():
    blocks = []
    for name, section, tok, cls in PREREQ_TYPES:
        struct_name = cls + "StaticDataStruct"
        fn = []
        fn.append("static cstr %s_nm (const StaticParsingManager& mgr, u16 idx) {" % name)
        fn.append("    const %s* a = mgr.get_%s_data();" % (struct_name, name))
        fn.append("    if (a == nullptr || idx >= mgr.get_%s_count()) {" % name)
        fn.append('        return "";')
        fn.append("    }")
        fn.append("    return a[idx].name.c_str();")
        fn.append("}")
        blocks.append("\n".join(fn))
    return "\n\n".join(blocks)

def derive_emit_reqs_loops ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        struct_name = cls + "StaticDataStruct"
        arg = "1" if name == "building" else "0"
        lines.append("const %s* %s_items = mgr.get_%s_data();" % (struct_name, name, name))
        lines.append("for (u8 i = 0; i < ir.m_%s_n; ++i) {" % name)
        lines.append("    u16 ix = ir.m_%s[i];" % name)
        lines.append("    if (%s_items != nullptr && ix < mgr.get_%s_count()) {" % (name, name))
        lines.append('        emit_req_tok(out, "%s", %s_items[ix].name.c_str(), %s);' % (tok, name, arg))
        lines.append("    }")
        lines.append("}")
    return join_tag(lines)

def derive_write_readable_sections ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append('pr_en_section(out, "%s", en.m_%s, en.m_%s_count, mgr, %s_nm, cfg);' % (section, name, name, name))
    return join_tag(lines)

def derive_assessor_ctx_members ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        lines.append("const BitArrayCL* m_%s;" % name)
    return join_tag(lines)

def derive_chk_switch_cases ():
    lines = []
    for name, section, tok, cls in PREREQ_TYPES:
        req_tp = item_req_type_enum(name)
        lines.append("case %s:" % req_tp)
        lines.append("    if (!chk_bit(ctx.m_%s, ix)) {" % name)
        lines.append("        return false;")
        lines.append("    }")
        lines.append("    break;")
    return join_tag(lines, "            ")

def derive_shared_substitution_pairs ():
    pairs = []
    pairs.append(("[INFERRED_REQS_ARRAYS_TAG]", derive_inferred_reqs_arrays()))
    pairs.append(("[INFERRED_REQS_COUNTS_TAG]", derive_inferred_reqs_counts()))
    pairs.append(("[ENABLES_MAP_SLOTS_TAG]", derive_enables_map_slots()))
    pairs.append(("[ENABLES_MAP_COUNTS_TAG]", derive_enables_map_counts()))
    pairs.append(("[SET_ALL_PARAMS_TAG]", derive_set_all_params()))
    pairs.append(("[SET_ALL_CALL_TAG]", derive_set_all_call()))
    pairs.append(("[SET_ALL_BITS_TAG]", derive_set_all_bits()))
    pairs.append(("[FREE_EN_MAP_TAG]", derive_free_en_map()))
    pairs.append(("[MAKE_EN_MAP_ARGS_TAG]", derive_make_en_map_args()))
    pairs.append(("[MAKE_EN_MAP_INIT_TAG]", derive_make_en_map_init()))
    pairs.append(("[MAKE_EN_MAP_CALL_TAG]", derive_make_en_map_call()))
    pairs.append(("[RUN_LOCAL_COUNTS_TAG]", derive_run_local_counts()))
    pairs.append(("[RUN_BITARRAY_DECL_TAG]", derive_run_bitarray_decl()))
    pairs.append(("[RUN_CTX_SNAP_TAG]", derive_run_ctx_snap()))
    pairs.append(("[ABLATION_LOOPS_TAG]", derive_ablation_loops()))
    pairs.append(("[BUILDING_ISOLATE_TAG]", derive_building_isolate()))
    pairs.append(("[PREREQ_NM_FUNCS_TAG]", derive_prereq_nm_funcs()))
    pairs.append(("[EMIT_REQS_LOOPS_TAG]", derive_emit_reqs_loops()))
    pairs.append(("[WRITE_READABLE_SECTIONS_TAG]", derive_write_readable_sections()))
    pairs.append(("[ASSESSOR_CTX_MEMBERS_TAG]", derive_assessor_ctx_members()))
    pairs.append(("[CHK_SWITCH_CASES_TAG]", derive_chk_switch_cases()))
    return pairs

def template_outputs ():
    outputs = []
    outputs.append(("TEMPLATE_assessor_brute_shared.h", "assessor_brute_shared.h"))
    outputs.append(("TEMPLATE_assessor_brute_shared.cpp", "assessor_brute_shared.cpp"))
    outputs.append(("TEMPLATE_assessor_brute_write.h", "assessor_brute_write.h"))
    outputs.append(("TEMPLATE_assessor_brute_write.cpp", "assessor_brute_write.cpp"))
    outputs.append(("TEMPLATE_general_assessor.h", "general_assessor.h"))
    outputs.append(("TEMPLATE_general_assessor.cpp", "general_assessor.cpp"))
    return outputs

def generate_template_files ():
    this_dir = os.path.dirname(os.path.abspath(__file__))
    pairs = derive_shared_substitution_pairs()
    for template_name, output_name in template_outputs():
        template_path = os.path.join(this_dir, template_name)
        out_path = os.path.join(this_dir, output_name)
        with open(template_path, "r") as ptr:
            content = ptr.read()
        for old_string, new_string in pairs:
            content = apply_substitution(content, old_string, new_string)
        with open(out_path, "w") as ptr:
            ptr.write(content)

def generate_shared_prereq_files ():
    generate_template_files()

#================================================================================================================================#
#=> - Per-assessor tester codegen -
#================================================================================================================================#

def derive_cost_emit_for_spec (prefix, class_name, parsing_instructions):
    struct_name = class_name + "StaticDataStruct"
    lines = []
    if prefix == "resource":
        lines.append("const ResourceStaticDataStruct* items = mgr.get_resource_data();")
        lines.append("if (items != nullptr) {")
        lines.append('    std::fprintf(out, " : %3u : %3u : %3u", items[idx].food, items[idx].shields, items[idx].commerce);')
        lines.append("}")
    elif prefix == "city_flag":
        pass
    elif parsing_instructions and "cost,1,u32" in parsing_instructions:
        lines.append("const %s* items = mgr.get_%s_data();" % (struct_name, prefix))
        lines.append("if (items != nullptr) {")
        lines.append('    std::fprintf(out, " : %5u", items[idx].cost);')
        lines.append("}")
    return join_tag(lines)

def derive_building_flags (prefix):
    if prefix == "building":
        return "false", "true"
    return "true", "false"

def derive_comp_compile_holders ():
    lines = []
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        lines.append("g++ $INC -c ../static_state/%s_static_data.cpp -o %s_static_data.o" % (prefix, prefix))
    return lines

def derive_comp_link_holders ():
    return ["    %s_static_data.o \\" % prefix for prefix, class_name, parsing_instructions in PARSER_SPECS]

def derive_comp_clean_holders ():
    return ["    %s_static_data.o \\" % prefix for prefix, class_name, parsing_instructions in PARSER_SPECS]

def derive_spm_compile ():
    dio = "../data_io"
    lines = []
    lines.append("g++ $INC -c %s/../static_state/static_bit_bank.cpp -o static_bit_bank.o" % dio)
    lines.append("g++ $INC -c %s/../static_state/unit_type_action_map.cpp -o unit_type_action_map.o" % dio)
    lines.append("g++ $INC -c %s/unit_type_action_map_parsing.cpp -o unit_type_action_map_parsing.o" % dio)
    lines.append("g++ $INC -c %s/../static_state/civ_bld_discount_map.cpp -o civ_bld_discount_map.o" % dio)
    lines.append("g++ $INC -c %s/civ_bld_discount_map_parsing.cpp -o civ_bld_discount_map_parsing.o" % dio)
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        lines.append("g++ $INC -c %s/%s_parser.cpp -o %s_parser.o" % (dio, prefix, prefix))
    return lines

def derive_spm_link ():
    objs = []
    objs.append("static_bit_bank.o")
    objs.append("unit_type_action_map.o")
    objs.append("unit_type_action_map_parsing.o")
    objs.append("civ_bld_discount_map.o")
    objs.append("civ_bld_discount_map_parsing.o")
    for prefix, class_name, parsing_instructions in PARSER_SPECS:
        objs.append("%s_parser.o" % prefix)
    return objs

def derive_spm_link_lines (objs):
    return "\n".join(["    %s \\" % o for o in objs])

def derive_spm_clean_lines (objs):
    return "\n".join(["    %s \\" % o for o in objs])

def get_output_filename (output_prefix, template_file):
    return template_file.replace("PREFIX", output_prefix)

def apply_substitution (content, old_string, new_string):
    return content.replace(old_string, new_string)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":

    generate_template_files()

    if len(sys.argv) != 3:
        print("Usage: python generate.py <output prefix> <class name>")
        sys.exit(1)

    output_prefix = sys.argv[1]
    class_name = sys.argv[2]
    struct_name = class_name + "StaticDataStruct"
    parsing_instructions = None
    for prefix, cn, pi in PARSER_SPECS:
        if prefix == output_prefix:
            parsing_instructions = pi
            break

    init_building_all, building_isolate = derive_building_flags(output_prefix)
    cost_emit = derive_cost_emit_for_spec(output_prefix, class_name, parsing_instructions)

    substitution_pairs = []
    substitution_pairs.append(("[CLASS_TAG]", class_name))
    substitution_pairs.append(("[FILE_TAG]", output_prefix))
    substitution_pairs.append(("[MACRO_TAG]", output_prefix.upper()))
    substitution_pairs.append(("[STRUCT_TAG]", struct_name))
    substitution_pairs.append(("[COST_EMIT_TAG]", cost_emit))
    substitution_pairs.append(("[INIT_BUILDING_ALL_TAG]", init_building_all))
    substitution_pairs.append(("[BUILDING_ISOLATE_TAG]", building_isolate))
    substitution_pairs.append(("[DEP_COMP_COMPILE_HOLDERS_TAG]", "\n".join(derive_comp_compile_holders())))
    substitution_pairs.append(("[DEP_COMP_LINK_HOLDERS_TAG]", "\n    ".join(derive_comp_link_holders())))
    substitution_pairs.append(("[DEP_COMP_CLEAN_HOLDERS_TAG]", "\n    ".join(derive_comp_clean_holders())))
    spm_objs = derive_spm_link()
    substitution_pairs.append(("[DEP_SPM_COMPILE_TAG]", "\n".join(derive_spm_compile())))
    substitution_pairs.append(("[DEP_SPM_LINK_TAG]", derive_spm_link_lines(spm_objs)))
    substitution_pairs.append(("[DEP_SPM_CLEAN_TAG]", derive_spm_clean_lines(spm_objs)))

    template_files = []
    template_files.append("PREFIX_assessor_tester_brute.cpp")
    template_files.append("PREFIX_assessor_tester.cpp")
    template_files.append("PREFIX_assessor_tester_comp")

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
        if output_file.endswith("_assessor_tester_comp"):
            os.chmod(output_file, 0o755)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
