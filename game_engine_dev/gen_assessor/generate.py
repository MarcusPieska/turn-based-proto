#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
import shutil
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
    lines.append("    BitArrayCL* cur = cfg.m_assess(cfg.m_item_count, ctx);")
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
    lines.append("        BitArrayCL* bl = cfg.m_assess(cfg.m_item_count, ctx);")
    lines.append("        for (u32 bb = 0; bb < building_count; ++bb) {")
    lines.append("            if (bb == b) {")
    lines.append("                continue;")
    lines.append("            }")
    lines.append("            building.clear_bit(bb);")
    lines.append("            snap(ctx);")
    lines.append("            BitArrayCL* cur = cfg.m_assess(cfg.m_item_count, ctx);")
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

def derive_assess_struct_fwd ():
    lines = []
    for prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        struct_name = class_name + "StaticDataStruct"
        lines.append("struct %s;" % struct_name)
    return join_tag(lines, "")

def derive_assess_declarations ():
    lines = []
    for prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        struct_name = class_name + "StaticDataStruct"
        lines.append("static BitArrayCL* assess_%s (u16 item_count, const %s* items, const AssessorCtx& ctx);" % (prefix, struct_name))
    return join_tag(lines)

def derive_assess_includes ():
    lines = []
    for prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        lines.append('#include "%s_static_data.h"' % prefix)
    return join_tag(lines, "")

def derive_assess_implementations ():
    blocks = []
    for prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        struct_name = class_name + "StaticDataStruct"
        fn = []
        fn.append("BitArrayCL* GeneralAssessor::assess_%s (u16 item_count, const %s* items, const AssessorCtx& ctx) {" % (prefix, struct_name))
        fn.append("    BitArrayCL* result = new BitArrayCL(item_count);")
        fn.append("    if (items == nullptr) {")
        fn.append("        return result;")
        fn.append("    }")
        fn.append("    for (u16 i = 0; i < item_count; ++i) {")
        fn.append("        if (chk(items[i].reqs, ctx)) {")
        fn.append("            result->set_bit(i);")
        fn.append("        }")
        fn.append("    }")
        fn.append("    return result;")
        fn.append("}")
        blocks.append("\n".join(fn))
    return "\n\n".join(blocks)

def derive_assessor_static_includes ():
    lines = []
    for prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        lines.append('#include "%s_static_data.h"' % prefix)
    return join_tag(lines, "")

def derive_assessor_brute_block (prefix, class_name, parsing_instructions):
    struct_name = class_name + "StaticDataStruct"
    macro = prefix.upper()
    init_building_all, building_isolate = derive_building_flags(prefix)
    cost_emit = derive_cost_emit_for_spec(prefix, class_name, parsing_instructions)
    lines = []
    lines.append("static const %s* s_%s_items;" % (struct_name, prefix))
    lines.append("")
    lines.append("static BitArrayCL* s_assess_%s (u16 n, const AssessorCtx& ctx) {" % prefix)
    lines.append("    return GeneralAssessor::assess_%s(n, s_%s_items, ctx);" % (prefix, prefix))
    lines.append("}")
    lines.append("")
    lines.append("static u16 get_%s_cnt (const StaticParsingManager& mgr) {" % prefix)
    lines.append("    return mgr.get_%s_count();" % prefix)
    lines.append("}")
    lines.append("")
    lines.append("static const char* get_%s_nm (const StaticParsingManager& mgr, u16 idx) {" % prefix)
    lines.append("    const %s* items = mgr.get_%s_data();" % (struct_name, prefix))
    lines.append("    if (items == nullptr || idx >= get_%s_cnt(mgr)) {" % prefix)
    lines.append('        return "";')
    lines.append("    }")
    lines.append("    return items[idx].name.c_str();")
    lines.append("}")
    lines.append("")
    lines.append("struct EmitUd_%s {" % macro)
    lines.append("    const StaticParsingManager* m_mgr;")
    lines.append("};")
    lines.append("")
    lines.append("static void emit_ln_%s (FILE* out, u16 idx, const InferredReqs& ir, void* ud) {" % prefix)
    lines.append("    EmitUd_%s* eu = static_cast<EmitUd_%s*>(ud);" % (macro, macro))
    lines.append("    const StaticParsingManager& mgr = *eu->m_mgr;")
    lines.append("    char nm[64];")
    lines.append("    AssessorBruteWrite::pad_name(nm, 64, get_%s_nm(mgr, idx));" % prefix)
    lines.append('    std::fprintf(out, "%s", nm);')
    if cost_emit:
        lines.append(cost_emit)
    lines.append("    AssessorBruteWrite::emit_reqs(out, ir, mgr);")
    lines.append('    std::fprintf(out, "\\n");')
    lines.append("}")
    lines.append("")
    lines.append("int run_%s_assessor_brute () {" % prefix)
    lines.append("    StaticParsingManager mgr(\"../\");")
    lines.append("    u16 n = get_%s_cnt(mgr);" % prefix)
    lines.append("    if (n == 0) {")
    lines.append("        return 0;")
    lines.append("    }")
    lines.append("    s_%s_items = mgr.get_%s_data();" % (prefix, prefix))
    lines.append("    if (s_%s_items == nullptr) {" % prefix)
    lines.append("        return 0;")
    lines.append("    }")
    lines.append("    static const char* s_nms[4096];")
    lines.append("    for (u16 i = 0; i < n && i < 4096; ++i) {")
    lines.append("        s_nms[i] = get_%s_nm(mgr, i);" % prefix)
    lines.append("    }")
    lines.append("    EmitUd_%s eu = { &mgr };" % macro)
    lines.append("    BruteRunCfg cfg = {};")
    lines.append("    cfg.m_item_count = n;")
    lines.append("    cfg.m_assess = s_assess_%s;" % prefix)
    lines.append("    cfg.m_names = s_nms;")
    lines.append('    cfg.m_results_to_match_path = "RESULTS_TO_MATCH_%s";' % macro)
    lines.append('    cfg.m_results_readable_path = "RESULTS_READABLE_%s";' % macro)
    lines.append("    cfg.m_init_building_all = %s;" % init_building_all)
    lines.append("    cfg.m_building_isolate = %s;" % building_isolate)
    lines.append("    cfg.m_emit_line = emit_ln_%s;" % prefix)
    lines.append("    cfg.m_emit_ud = &eu;")
    lines.append("    return AssessorBrute::run(mgr, cfg);")
    lines.append("}")
    return "\n".join(lines)

def derive_assessor_brute_blocks ():
    blocks = []
    for prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        blocks.append(derive_assessor_brute_block(prefix, class_name, parsing_instructions))
    return "\n\n".join(blocks)

def derive_assessor_run_decl ():
    lines = []
    for prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        lines.append("int run_%s_assessor_brute ();" % prefix)
    return join_tag(lines, "")

def derive_assessor_main_body ():
    lines = []
    lines.append("int rc = 0;")
    for prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        lines.append("rc |= run_%s_assessor_brute();" % prefix)
    lines.append("return rc;")
    return join_tag(lines, "    ")

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
    pairs.append(("[ASSESS_STRUCT_FWD_TAG]", derive_assess_struct_fwd()))
    pairs.append(("[ASSESS_DECLARATIONS_TAG]", derive_assess_declarations()))
    pairs.append(("[ASSESS_INCLUDES_TAG]", derive_assess_includes()))
    pairs.append(("[ASSESS_IMPLEMENTATIONS_TAG]", derive_assess_implementations()))
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
        with open(template_path, "r") as ptr:
            content = ptr.read()
        for old_string, new_string in pairs:
            content = apply_substitution(content, old_string, new_string)
        write_generated_file(this_dir, output_name, content)

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

def apply_substitution (content, old_string, new_string):
    return content.replace(old_string, new_string)

GENERATED_FILES = []

def clear_generated_files ():
    GENERATED_FILES.clear()

def write_generated_file (this_dir, output_name, content):
    out_path = os.path.join(this_dir, output_name)
    with open(out_path, "w") as ptr:
        ptr.write(content)
    if output_name.endswith("_comp"):
        os.chmod(out_path, 0o755)
    GENERATED_FILES.append(output_name)

def get_output_filename (output_prefix, template_file):
    return template_file.replace("PREFIX", output_prefix)

def generate_per_type_tester_files ():
    this_dir = os.path.dirname(os.path.abspath(__file__))
    spm_objs = derive_spm_link()
    template_files = []
    template_files.append("PREFIX_assessor_tester_brute.cpp")
    template_files.append("PREFIX_assessor_tester.cpp")
    template_files.append("PREFIX_assessor_tester_comp")
    for output_prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        struct_name = class_name + "StaticDataStruct"
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
        substitution_pairs.append(("[DEP_SPM_COMPILE_TAG]", "\n".join(derive_spm_compile())))
        substitution_pairs.append(("[DEP_SPM_LINK_TAG]", derive_spm_link_lines(spm_objs)))
        substitution_pairs.append(("[DEP_SPM_CLEAN_TAG]", derive_spm_clean_lines(spm_objs)))
        for template_file in template_files:
            template_path = os.path.join(this_dir, template_file)
            output_name = get_output_filename(output_prefix, template_file)
            with open(template_path, "r") as ptr:
                content = ptr.read()
            for old_string, new_string in substitution_pairs:
                content = apply_substitution(content, old_string, new_string)
            write_generated_file(this_dir, output_name, content)

def generate_tester_files ():
    this_dir = os.path.dirname(os.path.abspath(__file__))
    substitution_pairs = []
    substitution_pairs.append(("[ASSESSOR_STATIC_INCLUDES_TAG]", derive_assessor_static_includes()))
    substitution_pairs.append(("[ASSESSOR_BRUTE_BLOCKS_TAG]", derive_assessor_brute_blocks()))
    substitution_pairs.append(("[ASSESSOR_RUN_DECL_TAG]", derive_assessor_run_decl()))
    substitution_pairs.append(("[ASSESSOR_MAIN_BODY_TAG]", derive_assessor_main_body()))
    substitution_pairs.append(("[DEP_COMP_COMPILE_HOLDERS_TAG]", "\n".join(derive_comp_compile_holders())))
    substitution_pairs.append(("[DEP_COMP_LINK_HOLDERS_TAG]", "\n    ".join(derive_comp_link_holders())))
    substitution_pairs.append(("[DEP_COMP_CLEAN_HOLDERS_TAG]", "\n    ".join(derive_comp_clean_holders())))
    spm_objs = derive_spm_link()
    substitution_pairs.append(("[DEP_SPM_COMPILE_TAG]", "\n".join(derive_spm_compile())))
    substitution_pairs.append(("[DEP_SPM_LINK_TAG]", derive_spm_link_lines(spm_objs)))
    substitution_pairs.append(("[DEP_SPM_CLEAN_TAG]", derive_spm_clean_lines(spm_objs)))
    template_files = []
    template_files.append(("TEMPLATE_assessor_tester_brute.cpp", "assessor_tester_brute.cpp"))
    template_files.append(("TEMPLATE_assessor_tester.cpp", "assessor_tester.cpp"))
    template_files.append(("TEMPLATE_assessor_tester_comp", "assessor_tester_comp"))
    for template_name, output_name in template_files:
        template_path = os.path.join(this_dir, template_name)
        with open(template_path, "r") as ptr:
            content = ptr.read()
        for old_string, new_string in substitution_pairs:
            content = apply_substitution(content, old_string, new_string)
        write_generated_file(this_dir, output_name, content)
    generate_per_type_tester_files()

def generate_all_files ():
    clear_generated_files()
    generate_template_files()
    generate_tester_files()
    return list(GENERATED_FILES)

def remove_executables (dir_path):
    if not os.path.isdir(dir_path):
        return
    for name in os.listdir(dir_path):
        path = os.path.join(dir_path, name)
        if not os.path.isfile(path):
            continue
        if name == "assessor_tester" or name.endswith("_assessor_tester"):
            os.remove(path)
            print("  removed:", path)

def migrate_to_city (this_dir, generated_files=None):
    city_dir = os.path.normpath(os.path.join(this_dir, "..", "city"))
    os.makedirs(city_dir, exist_ok=True)
    remove_executables(this_dir)
    remove_executables(city_dir)
    names = generated_files if generated_files is not None else GENERATED_FILES
    for name in names:
        src = os.path.join(this_dir, name)
        if os.path.isfile(src):
            dst = os.path.join(city_dir, name)
            shutil.move(src, dst)
            print("  moved:", name, "->", city_dir)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    generate_all_files()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
