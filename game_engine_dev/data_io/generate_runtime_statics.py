#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True

from generater_commons import (
    data_header,
    data_struct,
    generate_from_templates,
    get_entries,
    get_map_specs,
    get_req_test_stems,
    join_tag_lines,
    lines_comp_clean_maps,
    lines_comp_clean_parser_test_suite,
    lines_comp_clean_parsers,
    lines_comp_compile_parser_test_suite,
    lines_comp_link_maps,
    lines_comp_link_parser_test_suite,
    lines_comp_link_parsers,
    map_class_name,
    static_data_class,
    static_data_key,
)

entries = get_entries()
req_test_stems = get_req_test_stems()
map_specs = get_map_specs()

from generate_item_effects import ie_scope_enum

SCOPE_ENUMS = [s for s in ie_scope_enum if s != "NONE"]

EFFECTOR_COMMON = ["effect_scoper_common", "effect_rev_mapper"]

#================================================================================================================================#
#=> - RuntimeStatics codegen -
#================================================================================================================================#

def scope_stem (scope_enum):
    return scope_enum.lower()

def scope_class (scope_enum):
    return scope_enum.capitalize() + "Effector"

def lines_runtime_header_includes ():
    return ['#include "%s"' % data_header(stem) for stem in entries]

def lines_runtime_header_map_includes ():
    return ['#include "%s.h"' % map_base for map_base, row_stem, col_stem in map_specs]

def lines_runtime_header_effector_includes ():
    return ['#include "gen_effector/%s_effector.h"' % scope_stem(s) for s in SCOPE_ENUMS]

def lines_runtime_header_members ():
    lines = ["%s m_%s;" % (static_data_class(stem), stem) for stem in entries]
    lines.append("")
    for map_base, row_stem, col_stem in map_specs:
        lines.append("%s m_%s;" % (map_class_name(map_base), map_base))
    lines.append("")
    for s in SCOPE_ENUMS:
        lines.append("%s m_%s_fx;" % (scope_class(s), scope_stem(s)))
    return lines

def lines_runtime_header_accessors ():
    lines = []
    for stem in entries:
        cls = static_data_class(stem)
        lines.append("%s& %s ();" % (cls, stem))
        lines.append("const %s& %s () const;" % (cls, stem))
    for map_base, row_stem, col_stem in map_specs:
        cls = map_class_name(map_base)
        lines.append("%s& %s ();" % (cls, map_base))
        lines.append("const %s& %s () const;" % (cls, map_base))
    for s in SCOPE_ENUMS:
        stem = scope_stem(s)
        cls = scope_class(s)
        lines.append("%s& %s_fx ();" % (cls, stem))
        lines.append("const %s& %s_fx () const;" % (cls, stem))
    return lines

def lines_runtime_cpp_map_includes ():
    lines = []
    seen = set()
    for map_base, row_stem, col_stem in map_specs:
        hdr = "%s.h" % map_base
        if hdr not in seen:
            seen.add(hdr)
            lines.append("#include \"%s\"" % hdr)
    return lines

def lines_runtime_cpp_effector_includes ():
    lines = ['#include "gen_effector/effect_rev_mapper.h"']
    for s in SCOPE_ENUMS:
        lines.append('#include "gen_effector/%s_effector.h"' % scope_stem(s))
    return lines

def lines_runtime_cpp_load_items_set ():
    lines = []
    for stem in entries:
        lines.append("m_%s.set_items(const_cast<%s*>(p.get_%s_data()), p.get_%s_count());" % (
            stem, data_struct(stem), stem, stem))
        lines.append("m_%s.load_names_from(p.get_%s_name_parser(), p.get_%s_count());" % (stem, stem, stem))
    return lines

def lines_runtime_cpp_load_items_take ():
    return ["m_%s.take_ownership();" % stem for stem in entries]

def lines_runtime_cpp_load_maps ():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("m_%s.set_map(p.get_%s_bank(), p.get_%s_count(), p.get_%s_count());" % (
            map_base, map_base, row_stem, col_stem))
        lines.append("m_%s.take_ownership();" % map_base)
    lines.append("p.release_map_banks();")
    return lines

def lines_runtime_cpp_load_effectors ():
    lines = []
    lines.append("u16 flat_fx_n = 0;")
    lines.append("EffectMapStruct* flat_fx = EffectRevMapper::build_flat_list(p, &flat_fx_n);")
    for s in SCOPE_ENUMS:
        lines.append("m_%s_fx.parse(flat_fx, flat_fx_n);" % scope_stem(s))
        lines.append("m_%s_fx.take_ownership();" % scope_stem(s))
    lines.append("EffectRevMapper::release_flat_list(flat_fx);")
    return lines

def lines_runtime_cpp_accessors ():
    blocks = []
    for stem in entries:
        cls = static_data_class(stem)
        blocks.append("%s& RuntimeStatics::%s () {\n    return m_%s;\n}" % (cls, stem, stem))
        blocks.append("const %s& RuntimeStatics::%s () const {\n    return m_%s;\n}" % (cls, stem, stem))
    for map_base, row_stem, col_stem in map_specs:
        cls = map_class_name(map_base)
        blocks.append("%s& RuntimeStatics::%s () {\n    return m_%s;\n}" % (cls, map_base, map_base))
        blocks.append("const %s& RuntimeStatics::%s () const {\n    return m_%s;\n}" % (cls, map_base, map_base))
    for s in SCOPE_ENUMS:
        stem = scope_stem(s)
        cls = scope_class(s)
        blocks.append("%s& RuntimeStatics::%s_fx () {\n    return m_%s_fx;\n}" % (cls, stem, stem))
        blocks.append("const %s& RuntimeStatics::%s_fx () const {\n    return m_%s_fx;\n}" % (cls, stem, stem))
    return blocks

def lines_lib_comp_compile_maps ():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("g++ $INC $CXXFLAGS -c ../static_state/%s.cpp -o %s.o" % (map_base, map_base))
        lines.append("g++ $INC $CXXFLAGS -c %s_parsing.cpp -o %s_parsing.o" % (map_base, map_base))
    return lines

def lines_lib_comp_compile_parsers ():
    return ["g++ $INC $CXXFLAGS -c %s_parser.cpp -o %s_parser.o" % (stem, stem) for stem in entries]

def lines_lib_comp_compile_holders ():
    lines = ["g++ $INC $CXXFLAGS -c ../misc/static_string_pool.cpp -o static_string_pool.o"]
    for stem in entries:
        lines.append("g++ $INC $CXXFLAGS -c ../static_state/%s_static_data.cpp -o %s_static_data.o" % (stem, stem))
        lines.append("g++ $INC $CXXFLAGS -c ../static_state/%s_static_data_load.cpp -o %s_static_data_load.o" % (stem, stem))
    return lines

def lines_comp_link_static_holders ():
    lines = []
    for stem in entries:
        lines.append("%s_static_data.o \\" % stem)
        lines.append("%s_static_data_load.o \\" % stem)
    lines.append("static_string_pool.o \\")
    return lines

def lines_comp_compile_runtime_statics_bundle ():
    return [
        "g++ $INC $CXXFLAGS -c ../static_state/config_settings_static.cpp -o config_settings_static.o",
        "g++ $INC $CXXFLAGS -c ../static_state/config_settings_parse.cpp -o config_settings_parse.o",
        "g++ $INC $CXXFLAGS -c runtime_statics.cpp -o runtime_statics.o",
    ]

def lines_comp_link_runtime_statics_bundle ():
    return [
        "config_settings_static.o \\",
        "config_settings_parse.o \\",
        "runtime_statics.o \\",
    ]

def lines_lib_comp_compile_effectors ():
    lines = []
    for name in EFFECTOR_COMMON:
        lines.append("g++ $INC $CXXFLAGS -c ../gen_effector/%s.cpp -o %s.o" % (name, name))
    for s in SCOPE_ENUMS:
        stem = scope_stem(s)
        lines.append("g++ $INC $CXXFLAGS -c ../gen_effector/%s_effector.cpp -o %s_effector.o" % (stem, stem))
    return lines

def lines_comp_link_effectors ():
    lines = []
    for name in EFFECTOR_COMMON:
        lines.append("%s.o \\" % name)
    for s in SCOPE_ENUMS:
        lines.append("%s_effector.o \\" % scope_stem(s))
    return lines

def lines_comp_clean_effectors ():
    lines = []
    for name in EFFECTOR_COMMON:
        lines.append("%s.o \\" % name)
    for s in SCOPE_ENUMS:
        lines.append("%s_effector.o \\" % scope_stem(s))
    return lines

def lines_tester_comp_compile_effectors ():
    return ["g++ $INC $CXXFLAGS -c ../gen_effector/%s_effector.cpp -o %s_effector.o" % (scope_stem(s), scope_stem(s)) for s in SCOPE_ENUMS]

def lines_tester_comp_link_effectors ():
    return ["%s_effector.o \\" % scope_stem(s) for s in SCOPE_ENUMS]

def lines_tester_comp_clean_effectors ():
    return ["%s_effector.o \\" % scope_stem(s) for s in SCOPE_ENUMS]

def lines_loader_tester_print_counts ():
    return ['print_u16_member("%s", s.%s().get_item_count());' % (stem, stem) for stem in entries]

def lines_loader_tester_load_count_tests ():
    return ['note_result(s.%s().get_item_count() > 0, "%s holder has items");' % (stem, stem) for stem in entries]

def map_count_getters (map_base, row_stem, col_stem):
    if map_base == "unit_type_action_map":
        return "get_unit_type_count", "get_action_count"
    if map_base == "civ_bld_discount_map":
        return "get_civ_trait_count", "get_building_count"
    return "get_%s_count" % row_stem, "get_%s_count" % col_stem

def lines_loader_tester_map_smoke_tests ():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        row_fn, col_fn = map_count_getters(map_base, row_stem, col_stem)
        lines.append('note_result(s.%s().%s() > 0, "%s row count");' % (map_base, row_fn, map_base))
        lines.append('note_result(s.%s().%s() > 0, "%s col count");' % (map_base, col_fn, map_base))
        lines.append('note_result(s.%s().%s() == s.%s().get_item_count(), "%s row match");' % (
            map_base, row_fn, row_stem, map_base))
        lines.append('note_result(s.%s().%s() == s.%s().get_item_count(), "%s col match");' % (
            map_base, col_fn, col_stem, map_base))
    return lines

FX_SOURCE_STEMS = ["building", "small_wonder", "tech", "wonder"]

def lines_loader_tester_effector_smoke_tests ():
    lines = []
    for s in SCOPE_ENUMS:
        stem = scope_stem(s)
        lines.append('note_result(s.%s_fx().get_count() == 0 || s.%s_fx().get_rows() != nullptr, "%s_fx rows when non-empty");' % (
            stem, stem, stem))
    lines.append('note_result(s.civ_fx().get_count() > 0, "civ_fx has rows");')
    lines.append("u16 total_fx = 0;")
    for stem in FX_SOURCE_STEMS:
        key = static_data_key(stem)
        lines.append("for (u16 i = 0; i < s.%s().get_item_count(); ++i) {" % stem)
        lines.append("    const ItemEffectStruct* slots = s.%s().get_item(%s::from_raw(i)).effects.items;" % (stem, key))
        lines.append("    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {")
        lines.append("        if (slots[j].type != 0) { ++total_fx; }")
        lines.append("    }")
        lines.append("}")
    lines.append("u16 scoped_fx = 0;")
    for s in SCOPE_ENUMS:
        lines.append("scoped_fx += s.%s_fx().get_count();" % scope_stem(s))
    lines.append('note_result(total_fx == 0 || scoped_fx * 10 >= total_fx * 9, "effector scope sum >= 90% of total effects");')
    return lines

def lines_loader_tester_req_test_blocks ():
    blocks = []
    for i, stem in enumerate(req_test_stems):
        key = static_data_key(stem)
        lines = []
        if i == 0:
            lines.append("bool result = true;")
        else:
            lines.append("result = true;")
        lines.append("for (u16 i = 0; i < s.%s().get_item_count(); ++i) {" % stem)
        lines.append("    if (!are_reqs_in_bounds(s, s.%s().get_item(%s::from_raw(i)).reqs, \"%s\", i)) {" % (
            stem, key, stem))
        lines.append("        result = false;")
        lines.append("        break;")
        lines.append("    }")
        lines.append("}")
        lines.append("note_result(result, \"%s req indices in bounds\");" % data_struct(stem))
        if i < len(req_test_stems) - 1:
            lines.append("")
        blocks.append("\n    ".join(lines))
    return blocks

def build_loader_sub_pairs ():
    sub_pairs = []
    sub_pairs.append(("[RUNTIME_LOADER_TESTER_PRINT_COUNTS_TAG]", join_tag_lines(lines_loader_tester_print_counts())))
    sub_pairs.append(("[RUNTIME_LOADER_TESTER_LOAD_COUNT_TESTS_TAG]", join_tag_lines(lines_loader_tester_load_count_tests(), "\n    ")))
    sub_pairs.append(("[RUNTIME_LOADER_TESTER_MAP_SMOKE_TESTS_TAG]", join_tag_lines(lines_loader_tester_map_smoke_tests(), "\n    ")))
    sub_pairs.append(("[RUNTIME_LOADER_TESTER_EFFECTOR_SMOKE_TESTS_TAG]", join_tag_lines(lines_loader_tester_effector_smoke_tests(), "\n    ")))
    sub_pairs.append(("[RUNTIME_LOADER_TESTER_REQ_TESTS_TAG]", join_tag_lines(lines_loader_tester_req_test_blocks(), "\n    ")))
    return sub_pairs

def build_runtime_sub_pairs ():
    sub_pairs = []
    sub_pairs.append(("[RUNTIME_STATICS_HEADER_INCLUDES_TAG]", join_tag_lines(
        lines_runtime_header_includes() + lines_runtime_header_map_includes() + lines_runtime_header_effector_includes(), "\n")))
    sub_pairs.append(("[RUNTIME_STATICS_HEADER_MEMBERS_TAG]", join_tag_lines(lines_runtime_header_members())))
    sub_pairs.append(("[RUNTIME_STATICS_HEADER_ACCESSORS_TAG]", join_tag_lines(lines_runtime_header_accessors(), "\n\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_MAP_INCLUDES_TAG]", join_tag_lines(lines_runtime_cpp_map_includes(), "\n")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_EFFECTOR_INCLUDES_TAG]", join_tag_lines(lines_runtime_cpp_effector_includes(), "\n")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_LOAD_ITEMS_SET_TAG]", join_tag_lines(lines_runtime_cpp_load_items_set(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_LOAD_EFFECTORS_TAG]", join_tag_lines(lines_runtime_cpp_load_effectors(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_LOAD_ITEMS_TAKE_TAG]", join_tag_lines(lines_runtime_cpp_load_items_take(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_LOAD_MAPS_TAG]", join_tag_lines(lines_runtime_cpp_load_maps(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_ACCESSORS_TAG]", join_tag_lines(lines_runtime_cpp_accessors(), "\n\n")))
    return sub_pairs

def build_runtime_statics_comp_sub_pairs ():
    sub_pairs = []
    sub_pairs.append(("[RUNTIME_STATICS_COMP_COMPILE_PARSER_SUITE_TAG]", join_tag_lines(lines_comp_compile_parser_test_suite(), "\n")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_COMPILE_EFFECTORS_TAG]", join_tag_lines(lines_tester_comp_compile_effectors(), "\n")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_PARSER_SUITE_TAG]", join_tag_lines(lines_comp_link_parser_test_suite(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_EFFECTORS_TAG]", join_tag_lines(lines_tester_comp_link_effectors(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_PARSER_SUITE_TAG]", join_tag_lines(lines_comp_clean_parser_test_suite(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_EFFECTORS_TAG]", join_tag_lines(lines_tester_comp_clean_effectors(), "\n    ")))
    return sub_pairs

def build_loader_comp_sub_pairs ():
    sub_pairs = build_loader_sub_pairs()
    sub_pairs.append(("[RUNTIME_STATICS_COMP_COMPILE_EFFECTORS_TAG]", join_tag_lines(lines_tester_comp_compile_effectors(), "\n")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_EFFECTORS_TAG]", join_tag_lines(lines_tester_comp_link_effectors(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_EFFECTORS_TAG]", join_tag_lines(lines_tester_comp_clean_effectors(), "\n    ")))
    return sub_pairs

def build_lib_sub_pairs ():
    sub_pairs = []
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_MAPS_TAG]", join_tag_lines(lines_comp_link_maps(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_PARSERS_TAG]", join_tag_lines(lines_comp_link_parsers(), "\n    ")))
    holder_objs = []
    for stem in entries:
        holder_objs.append("%s_static_data.o \\" % stem)
        holder_objs.append("%s_static_data_load.o \\" % stem)
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_HOLDERS_TAG]", join_tag_lines(holder_objs + ["static_string_pool.o \\"], "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_MAPS_TAG]", join_tag_lines(lines_comp_clean_maps(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_PARSERS_TAG]", join_tag_lines(lines_comp_clean_parsers(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_HOLDERS_TAG]", join_tag_lines(holder_objs + ["static_string_pool.o \\"], "\n    ")))
    sub_pairs.append(("[RUNTIME_STATIC_LOADER_LIB_COMP_COMPILE_MAPS_TAG]", join_tag_lines(lines_lib_comp_compile_maps(), "\n")))
    sub_pairs.append(("[RUNTIME_STATIC_LOADER_LIB_COMP_COMPILE_PARSERS_TAG]", join_tag_lines(lines_lib_comp_compile_parsers(), "\n")))
    sub_pairs.append(("[RUNTIME_STATIC_LOADER_LIB_COMP_COMPILE_HOLDERS_TAG]", join_tag_lines(lines_lib_comp_compile_holders(), "\n")))
    sub_pairs.append(("[RUNTIME_STATIC_LOADER_LIB_COMP_COMPILE_EFFECTORS_TAG]", join_tag_lines(lines_lib_comp_compile_effectors(), "\n")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_EFFECTORS_TAG]", join_tag_lines(lines_comp_link_effectors(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_EFFECTORS_TAG]", join_tag_lines(lines_comp_clean_effectors(), "\n    ")))
    return sub_pairs

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))
    runtime_statics_files = [
        ("runtime_statics.h", "runtime_statics.h"),
        ("runtime_statics.cpp", "runtime_statics.cpp"),
        ("runtime_statics_tester.cpp", "runtime_statics_tester.cpp"),
        ("runtime_statics_comp", "runtime_statics_comp"),
        ("runtime_static_loader_lib_api.h", "runtime_static_loader_lib_api.h"),
        ("runtime_static_loader_lib_api.cpp", "runtime_static_loader_lib_api.cpp"),
        ("runtime_static_loader_lib_comp", "runtime_static_loader_lib_comp"),
    ]
    for template_name, output_name in runtime_statics_files:
        if output_name == "runtime_statics_tester.cpp":
            generate_from_templates(this_dir, template_name, output_name, build_loader_sub_pairs())
        elif output_name == "runtime_static_loader_lib_comp":
            generate_from_templates(this_dir, template_name, output_name, build_lib_sub_pairs())
        elif output_name == "runtime_statics_comp":
            generate_from_templates(this_dir, template_name, output_name, build_runtime_statics_comp_sub_pairs())
        else:
            generate_from_templates(this_dir, template_name, output_name, build_runtime_sub_pairs())

    loader_files = [
        ("runtime_static_loader.h", "runtime_static_loader.h"),
        ("runtime_static_loader.cpp", "runtime_static_loader.cpp"),
        ("runtime_static_loader_tester.cpp", "runtime_static_loader_tester.cpp"),
        ("runtime_static_loader_comp", "runtime_static_loader_comp"),
    ]
    for template_name, output_name in loader_files:
        if output_name == "runtime_static_loader_comp":
            generate_from_templates(this_dir, template_name, output_name, build_loader_comp_sub_pairs())
        else:
            generate_from_templates(this_dir, template_name, output_name, build_loader_sub_pairs())

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
