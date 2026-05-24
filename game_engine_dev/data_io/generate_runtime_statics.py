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

#================================================================================================================================#
#=> - RuntimeStatics codegen -
#================================================================================================================================#

def lines_runtime_header_includes ():
    return ['#include "%s"' % data_header(stem) for stem in entries]

def lines_runtime_header_map_includes ():
    return ['#include "%s.h"' % map_base for map_base, row_stem, col_stem in map_specs]

def lines_runtime_header_members ():
    lines = ["%s m_%s;" % (static_data_class(stem), stem) for stem in entries]
    for map_base, row_stem, col_stem in map_specs:
        lines.append("%s m_%s;" % (map_class_name(map_base), map_base))
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

def lines_runtime_cpp_load_items ():
    lines = []
    for stem in entries:
        lines.append("m_%s.set_items(const_cast<%s*>(p.get_%s_data()), p.get_%s_count());" % (
            stem, data_struct(stem), stem, stem))
        lines.append("m_%s.take_ownership();" % stem)
    return lines

def lines_runtime_cpp_load_maps ():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("m_%s.set_map(p.get_%s_bank(), p.get_%s_count(), p.get_%s_count());" % (
            map_base, map_base, row_stem, col_stem))
        lines.append("m_%s.take_ownership();" % map_base)
    lines.append("p.release_map_banks();")
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
    return ["g++ $INC $CXXFLAGS -c ../static_state/%s_static_data.cpp -o %s_static_data.o" % (stem, stem) for stem in entries]

def lines_loader_tester_print_counts ():
    return ['print_u16_member("%s", statics.%s().get_item_count());' % (stem, stem) for stem in entries]

def lines_loader_tester_load_count_tests ():
    return ['note_result(statics.%s().get_item_count() > 0, "%s holder has items");' % (stem, stem) for stem in entries]

def lines_loader_tester_req_test_blocks ():
    blocks = []
    for i, stem in enumerate(req_test_stems):
        key = static_data_key(stem)
        lines = []
        if i == 0:
            lines.append("bool result = true;")
        else:
            lines.append("result = true;")
        lines.append("for (u16 i = 0; i < statics.%s().get_item_count(); ++i) {" % stem)
        lines.append("    if (!are_reqs_in_bounds(statics, statics.%s().get_item(%s::from_raw(i)).reqs, \"%s\", i)) {" % (
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
    sub_pairs.append(("[RUNTIME_LOADER_TESTER_REQ_TESTS_TAG]", join_tag_lines(lines_loader_tester_req_test_blocks(), "\n    ")))
    return sub_pairs

def build_runtime_sub_pairs ():
    sub_pairs = []
    sub_pairs.append(("[RUNTIME_STATICS_HEADER_INCLUDES_TAG]", join_tag_lines(
        lines_runtime_header_includes() + lines_runtime_header_map_includes(), "\n")))
    sub_pairs.append(("[RUNTIME_STATICS_HEADER_MEMBERS_TAG]", join_tag_lines(lines_runtime_header_members())))
    sub_pairs.append(("[RUNTIME_STATICS_HEADER_ACCESSORS_TAG]", join_tag_lines(lines_runtime_header_accessors(), "\n\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_MAP_INCLUDES_TAG]", join_tag_lines(lines_runtime_cpp_map_includes(), "\n")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_LOAD_ITEMS_TAG]", join_tag_lines(lines_runtime_cpp_load_items(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_LOAD_MAPS_TAG]", join_tag_lines(lines_runtime_cpp_load_maps(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_CPP_ACCESSORS_TAG]", join_tag_lines(lines_runtime_cpp_accessors(), "\n\n")))
    return sub_pairs

def build_runtime_statics_comp_sub_pairs ():
    sub_pairs = []
    sub_pairs.append(("[RUNTIME_STATICS_COMP_COMPILE_PARSER_SUITE_TAG]", join_tag_lines(lines_comp_compile_parser_test_suite(), "\n")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_PARSER_SUITE_TAG]", join_tag_lines(lines_comp_link_parser_test_suite(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_PARSER_SUITE_TAG]", join_tag_lines(lines_comp_clean_parser_test_suite(), "\n    ")))
    return sub_pairs

def build_lib_sub_pairs ():
    sub_pairs = []
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_MAPS_TAG]", join_tag_lines(lines_comp_link_maps(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_PARSERS_TAG]", join_tag_lines(lines_comp_link_parsers(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_LINK_HOLDERS_TAG]", join_tag_lines(["%s_static_data.o \\" % stem for stem in entries], "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_MAPS_TAG]", join_tag_lines(lines_comp_clean_maps(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_PARSERS_TAG]", join_tag_lines(lines_comp_clean_parsers(), "\n    ")))
    sub_pairs.append(("[RUNTIME_STATICS_COMP_CLEAN_HOLDERS_TAG]", join_tag_lines(["%s_static_data.o \\" % stem for stem in entries], "\n    ")))
    sub_pairs.append(("[RUNTIME_STATIC_LOADER_LIB_COMP_COMPILE_MAPS_TAG]", join_tag_lines(lines_lib_comp_compile_maps(), "\n")))
    sub_pairs.append(("[RUNTIME_STATIC_LOADER_LIB_COMP_COMPILE_PARSERS_TAG]", join_tag_lines(lines_lib_comp_compile_parsers(), "\n")))
    sub_pairs.append(("[RUNTIME_STATIC_LOADER_LIB_COMP_COMPILE_HOLDERS_TAG]", join_tag_lines(lines_lib_comp_compile_holders(), "\n")))
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
        generate_from_templates(this_dir, template_name, output_name, build_loader_sub_pairs())

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
