#================================================================================================================================#
#=> - generate_config_settings.py -
#================================================================================================================================#
#
#  Reads config_settings.spec and emits GameConfigSettings code from templates
#  into ../static_state/:
#    TEMPLATE_config_settings_static.h / .cpp
#    TEMPLATE_config_settings_parse.h / .cpp
#    TEMPLATE_config_settings_comp (tagless copy)
#    TEMPLATE_config_settings_tester.cpp
#
#  Spec line format (config_settings.spec):
#    CONFIG_KEY : CARDINALITY : REGISTRY
#
#  Parse dispatch: template keeps [CONFIG_PARSE_DISPATCH_TAG] at line indent (4 spaces).
#  Unrolled lines have no leading indent; join_tag_lines adds "\n    " or "\n        " between them.
#
#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - Registry -
#================================================================================================================================#

TYPE_FAMILIES = {}
TYPE_FAMILIES[("SINGLE", "U16")] = {
    "value_type": "u16",
    "comp": "eq_u16",
    "assign": "assign_u16",
    "clr_line": "d->{member} = 0;",
    "parse_fn": "parse_u16",
    "parse_tmp": "u16 tmp = 0;",
    "parse_call": "parse_u16(v, &tmp)",
    "parse_fail": 'std::printf("ERROR: config could not parse u16 for \'%s\': \'%s\'\\n", k, v);',
}
TYPE_FAMILIES[("LIST", "UNIT")] = {
    "value_type": "ConfigListUnit",
    "comp": "eq_list_unit",
    "assign": "assign_list_unit",
    "clr_line": "clr_list_unit(&d->{member});",
    "parse_fn": "parse_list_unit",
    "parse_tmp": "ConfigListUnit tmp = {};",
    "parse_call": "parse_list_unit(v, &tmp, m_cbs.unit_name_to_idx)",
    "parse_fail": 'std::printf("ERROR: config could not parse list for \'%s\': \'%s\'\\n", k, v);',
}

METHOD_OVERRIDES = {}
METHOD_OVERRIDES["PATH_MP_TURN"] = "mov_pt_per_turn"

MEMBER_OVERRIDES = {}
MEMBER_OVERRIDES["PATH_MP_TURN"] = "m_mov_pt_per_turn"

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def join_tag_lines(lines, indent_between="\n    "):
    return indent_between.join(lines)

def join_blocks(blocks, sep="\n\n"):
    return sep.join(blocks)

def default_method_name(config_key):
    if config_key in METHOD_OVERRIDES:
        return METHOD_OVERRIDES[config_key]
    return config_key.lower()

def default_member_name(config_key):
    if config_key in MEMBER_OVERRIDES:
        return MEMBER_OVERRIDES[config_key]
    return "m_" + config_key.lower()

def lookup_type_family(cardinality, registry):
    key = (cardinality, registry)
    if key not in TYPE_FAMILIES:
        raise ValueError("unknown type family: %s : %s" % (cardinality, registry))
    return TYPE_FAMILIES[key]

def parse_spec_line(line):
    line = line.strip()
    if line == "" or line.startswith("#"):
        return None
    parts = [p.strip() for p in line.split(":")]
    if len(parts) != 3:
        raise ValueError("expected KEY : CARDINALITY : REGISTRY, got: %s" % line)
    return {
        "key": parts[0],
        "cardinality": parts[1],
        "registry": parts[2],
    }

def load_spec_entries(spec_path):
    entries = []
    with open(spec_path, "r") as ptr:
        for raw in ptr:
            entry = parse_spec_line(raw)
            if entry is not None:
                entries.append(entry)
    if len(entries) == 0:
        raise ValueError("no config settings entries in spec: %s" % spec_path)
    return entries

def enrich_entry(entry):
    fam = lookup_type_family(entry["cardinality"], entry["registry"])
    method = default_method_name(entry["key"])
    member = default_member_name(entry["key"])
    out = {}
    out.update(entry)
    out["method"] = method
    out["member"] = member
    out["value_type"] = fam["value_type"]
    out["comp"] = fam["comp"]
    out["assign"] = fam["assign"]
    out["clr_line"] = fam["clr_line"].format(member=member)
    out["parse_fn"] = fam["parse_fn"]
    out["parse_tmp"] = fam["parse_tmp"]
    out["parse_call"] = fam["parse_call"]
    out["parse_fail"] = fam["parse_fail"]
    out["setter"] = "out->set_%s(tmp);" % method
    return out

def line_data_member(entry):
    return "%s %s;" % (entry["value_type"], entry["member"])

def line_getter_decl(entry):
    return "%s get_%s () const;" % (entry["value_type"], entry["method"])

def line_setter_decl(entry):
    return "void set_%s (%s v);" % (entry["method"], entry["value_type"])

def block_getter_impl(entry):
    lines = []
    lines.append("%s GameConfigSettings::get_%s () const {" % (entry["value_type"], entry["method"]))
    lines.append("    return cur().%s;" % entry["member"])
    lines.append("}")
    return "\n".join(lines)

def block_setter_impl(entry):
    m = entry["member"]
    lines = []
    lines.append("void GameConfigSettings::set_%s (%s v) {" % (entry["method"], entry["value_type"]))
    lines.append("    if (%s(v, m_defaults.%s)) {" % (entry["comp"], m))
    lines.append("        if (!m_is_default) {")
    lines.append("            %s(&m_cfg->%s, v);" % (entry["assign"], m))
    lines.append("        }")
    lines.append("        return;")
    lines.append("    }")
    lines.append("    ensure_ovr();")
    lines.append("    %s(&m_cfg->%s, v);" % (entry["assign"], m))
    lines.append("}")
    return "\n".join(lines)

def block_accessor_impls(entry):
    return join_blocks([block_getter_impl(entry), block_setter_impl(entry)])

def block_parse_branch(entry, is_first):
    if is_first:
        head = "if (std::strcmp(k, \"%s\") == 0) {" % entry["key"]
    else:
        head = "} else if (std::strcmp(k, \"%s\") == 0) {" % entry["key"]
    inner_parts = []
    inner_parts.append(entry["parse_tmp"])
    inner_parts.append("if (!%s) {" % entry["parse_call"])
    if entry["parse_fail"] is not None:
        inner_parts.append("    %s" % entry["parse_fail"])
    inner_parts.append("    return false;")
    inner_parts.append("}")
    inner_parts.append(entry["setter"])
    inner_parts.append("return true;")
    inner = join_tag_lines(inner_parts, "\n        ")
    return join_tag_lines([head, inner], "\n        ")

def unroll_parse_dispatch(entries):
    if len(entries) == 0:
        return ""
    parts = []
    for i, e in enumerate(entries):
        parts.append(block_parse_branch(e, i == 0))
    parts.append("}")
    return join_tag_lines(parts, "\n    ")

def unroll_data_members(entries):
    return join_tag_lines([line_data_member(e) for e in entries])

def unroll_accessor_decls(entries):
    blocks = []
    for e in entries:
        blocks.append(join_tag_lines([line_getter_decl(e), line_setter_decl(e)]))
    return "\n\n    ".join(blocks)

def unroll_clr_data_body(entries):
    return join_tag_lines([e["clr_line"] for e in entries])

def unroll_accessor_impls(entries):
    blocks = [block_accessor_impls(e) for e in entries]
    return join_blocks(blocks)

def block_print_u16(entry):
    return 'std::printf("  %s: %%u\\n", cfg.get_%s());' % (entry["key"], entry["method"])

def block_print_list_unit(entry):
    lines = []
    lines.append("{")
    lines.append("    ConfigListUnit lst = cfg.get_%s();" % entry["method"])
    lines.append('    std::printf("  %s:");' % entry["key"])
    lines.append("    for (u16 i = 0; i < lst.n; ++i) {")
    lines.append('        std::printf(" %s", unit_idx_to_name(lst.keys[i].value()));')
    lines.append("    }")
    lines.append('    std::printf("\\n");')
    lines.append("}")
    return join_tag_lines(lines, "\n    ")

def block_print_field(entry):
    if entry["cardinality"] == "SINGLE":
        return block_print_u16(entry)
    if entry["cardinality"] == "LIST":
        return block_print_list_unit(entry)
    raise ValueError("unknown cardinality for print: %s" % entry["cardinality"])

def unroll_print_body(entries):
    return join_tag_lines([block_print_field(e) for e in entries])

def apply_substitutions(content, pairs):
    for old_string, new_string in pairs:
        content = content.replace(old_string, new_string)
    return content

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

def get_static_state_dir():
    this_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.normpath(os.path.join(this_dir, "..", "static_state"))

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))
    static_state_dir = get_static_state_dir()
    os.makedirs(static_state_dir, exist_ok=True)
    spec_path = os.path.join(this_dir, "config_settings.spec")
    entries = [enrich_entry(e) for e in load_spec_entries(spec_path)]

    static_pairs = []
    static_pairs.append(("[CONFIG_DATA_MEMBERS_TAG]", unroll_data_members(entries)))
    static_pairs.append(("[CONFIG_ACCESSOR_DECLS_TAG]", unroll_accessor_decls(entries)))
    static_pairs.append(("[CONFIG_CLR_DATA_BODY_TAG]", unroll_clr_data_body(entries)))
    static_pairs.append(("[CONFIG_ACCESSOR_IMPLS_TAG]", unroll_accessor_impls(entries)))

    parse_pairs = []
    parse_pairs.append(("[CONFIG_PARSE_DISPATCH_TAG]", unroll_parse_dispatch(entries)))

    tester_pairs = []
    tester_pairs.append(("[CONFIG_PRINT_BODY_TAG]", unroll_print_body(entries)))

    outputs = []
    outputs.append(("config_settings_static.h", static_pairs))
    outputs.append(("config_settings_static.cpp", static_pairs))
    outputs.append(("config_settings_parse.h", parse_pairs))
    outputs.append(("config_settings_parse.cpp", parse_pairs))
    outputs.append(("config_settings_tester.cpp", tester_pairs))

    for output_name, pairs in outputs:
        template_path = os.path.join(this_dir, "TEMPLATE_" + output_name)
        out_path = os.path.join(static_state_dir, output_name)
        with open(template_path, "r") as ptr:
            content = ptr.read()
        content = apply_substitutions(content, pairs)
        with open(out_path, "w") as ptr:
            ptr.write(content)
        print("wrote", out_path)

    comp_template_path = os.path.join(this_dir, "TEMPLATE_config_settings_comp")
    comp_out_path = os.path.join(static_state_dir, "config_settings_comp")
    with open(comp_template_path, "r") as ptr:
        comp_content = ptr.read()
    with open(comp_out_path, "w") as ptr:
        ptr.write(comp_content)
    os.chmod(comp_out_path, 0o755)
    print("wrote", comp_out_path)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
