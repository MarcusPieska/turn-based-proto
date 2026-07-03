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
    lines_comp_clean_parsers,
    lines_comp_compile_maps,
    lines_comp_compile_parsers,
    lines_comp_link_maps,
    lines_comp_link_parsers,
    lines_tester_map_includes,
    lines_tester_map_tests,
    map_class_name,
    parser_class,
    parser_header,
    path_from_stem,
    to_pascal,
)

entries = get_entries()
req_test_stems = get_req_test_stems()
map_specs = get_map_specs()

#================================================================================================================================#
#=> - StaticParsingManager codegen -
#================================================================================================================================#

def lines_header_parser_includes ():
    return ['#include "%s"' % parser_header(stem) for stem in entries]

def lines_header_data_includes ():
    return ['#include "%s"' % data_header(stem) for stem in entries]

def lines_header_name_parser_getter_blocks ():
    return ["const DataParserBase& get_%s_name_parser () const;" % stem for stem in entries]

def lines_header_typed_getter_blocks ():
    lines = []
    for stem in entries:
        lines.append("const %s* get_%s_data () const;" % (data_struct(stem), stem))
        lines.append("u16 get_%s_count () const;" % stem)
    return lines

def lines_header_raw_getter_blocks ():
    return []

def lines_header_readers ():
    return ["StringManager m_%s_items;" % stem for stem in entries]

def lines_header_name_parsers ():
    return ["DataParserBase* m_%s_name_parser;" % stem for stem in entries]

def lines_header_data_ptrs ():
    return ["%s* m_%s_data;" % (data_struct(stem), stem) for stem in entries]

def lines_cpp_callback_parsers ():
    return ["const DataParserBase* g_%s_name_parser = nullptr;" % stem for stem in entries]

def lines_cpp_callback_func_blocks ():
    blocks = []
    for stem in entries:
        lines = []
        lines.append("u16 cb_%s_name_to_idx (cstr name) {" % stem)
        lines.append("    return g_%s_name_parser->name_to_idx(name);" % stem)
        lines.append("}")
        blocks.append("\n".join(lines))
    return blocks

def lines_cpp_reader_inits ():
    return ["m_%s_items()," % stem for stem in entries]

def lines_cpp_name_parser_inits ():
    return ["m_%s_name_parser(nullptr)," % stem for stem in entries]

def lines_cpp_data_ptr_inits ():
    lines = []
    for i, stem in enumerate(entries):
        suffix = "," if i < len(entries) - 1 else ""
        lines.append("m_%s_data(nullptr)%s" % (stem, suffix))
    return lines

def lines_cpp_typed_getter_blocks ():
    blocks = []
    for stem in entries:
        lines = []
        lines.append("const %s* StaticParsingManager::get_%s_data () const {" % (data_struct(stem), stem))
        lines.append("    return m_%s_data;" % stem)
        lines.append("}")
        lines.append("")
        lines.append("u16 StaticParsingManager::get_%s_count () const {" % stem)
        lines.append("    return safe_size_to_u16(m_%s_items.get_string_count());" % stem)
        lines.append("}")
        blocks.append("\n".join(lines))
    return blocks

def lines_cpp_name_parser_getter_blocks ():
    blocks = []
    for stem in entries:
        lines = []
        lines.append("const DataParserBase& StaticParsingManager::get_%s_name_parser () const {" % stem)
        lines.append("    return *m_%s_name_parser;" % stem)
        lines.append("}")
        blocks.append("\n".join(lines))
    return blocks

def lines_cpp_raw_getter_blocks ():
    return []

def lines_cpp_callback_assign_parsers ():
    return ["g_%s_name_parser = m_%s_name_parser;" % (stem, stem) for stem in entries]

def lines_cpp_callback_assign_cbs ():
    return ["m_name_to_idx_cbs.%s_name_to_idx = cb_%s_name_to_idx;" % (stem, stem) for stem in entries]

def lines_cpp_local_parsers ():
    return ["%s %s_parser(m_%s_items, m_name_to_idx_cbs);" % (parser_class(stem), stem, stem) for stem in entries]

def lines_cpp_load_items ():
    lines = [
        "m_effect_items.split_string_by_char(0, '\\n');",
        "m_effect_items.cull_empty_strings();",
    ]
    for stem in entries:
        lines.append("m_%s_items.load_file_content(m_paths.get_path_to_%s());" % (stem, path_from_stem(stem)))
        lines.append("m_%s_items.split_string_by_char(0, '\\n');" % stem)
        lines.append("m_%s_items.cull_empty_strings();" % stem)
    return lines

def lines_cpp_init_name_parsers ():
    return ["m_%s_name_parser = new DataParserBase(m_%s_items, NameToIdxCbs());" % (stem, stem) for stem in entries]

def lines_cpp_delete_name_parsers ():
    return ["delete m_%s_name_parser;" % stem for stem in entries]

def lines_cpp_parse_assignments ():
    return ["m_%s_data = %s_parser.parse_data_dependencies();" % (stem, stem) for stem in entries]

def lines_tester_print_counts ():
    return ['print_u16_member("%s", parser.get_%s_count());' % (stem, stem) for stem in entries]

def lines_tester_req_test_blocks ():
    lines = []
    for i, stem in enumerate(req_test_stems):
        pre = "bool result = " if i == 0 else "result = "
        lines.append("%stest_dataset_req_bounds(parser, parser.get_%s_data(), parser.get_%s_count(), \"%s\");" % (
            pre, stem, stem, stem))
        lines.append("note_result(result, \"%s req indices in bounds\");" % data_struct(stem))
        if i < len(req_test_stems) - 1:
            lines.append("")
    return lines

def lines_header_map_bank_ptrs ():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("StaticBitBank* m_%s_bank;" % map_base)
    return lines

def lines_cpp_map_includes ():
    lines = []
    lines.append("#include \"static_bit_bank.h\"")
    seen = set()
    for map_base, row_stem, col_stem in map_specs:
        for hdr in (
            parser_header(row_stem),
            parser_header(col_stem),
            "%s_parsing.h" % map_base,
        ):
            if hdr not in seen:
                seen.add(hdr)
                lines.append("#include \"%s\"" % hdr)
    return lines

def lines_cpp_map_bank_inits ():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("m_%s_bank(nullptr)," % map_base)
    return lines

def lines_cpp_delete_map_banks ():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("delete m_%s_bank;" % map_base)
    return lines

def lines_cpp_build_maps ():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        cls = map_class_name(map_base)
        parse_cls = cls + "Parsing"
        r_items = "m_%s_items" % row_stem
        r_n = "%s_n" % row_stem
        c_n = "%s_n" % col_stem
        bank = "m_%s_bank" % map_base
        pr = "%s_parser" % row_stem
        pc = "%s_parser" % col_stem
        lines.append("")
        lines.append("const u16 %s = safe_size_to_u16(%s.get_string_count());" % (r_n, r_items))
        lines.append("const u16 %s = safe_size_to_u16(m_%s_items.get_string_count());" % (c_n, col_stem))
        lines.append("%s = new StaticBitBank(%s, %s);" % (bank, r_n, c_n))
        lines.append("%s::load_cfg_map(*%s, %s, %s, %s);" % (parse_cls, bank, r_items, pr, pc))
    return lines

def lines_header_map_getter_blocks ():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("StaticBitBank* get_%s_bank () const;" % map_base)
    lines.append("void release_map_banks ();")
    return lines

def lines_cpp_release_map_banks ():
    lines = ["void StaticParsingManager::release_map_banks () {"]
    for map_base, row_stem, col_stem in map_specs:
        lines.append("    m_%s_bank = nullptr;" % map_base)
    lines.append("}")
    return lines

def lines_cpp_map_getter_blocks ():
    blocks = []
    for map_base, row_stem, col_stem in map_specs:
        sub = []
        sub.append("StaticBitBank* StaticParsingManager::get_%s_bank () const {" % map_base)
        sub.append("    return m_%s_bank;" % map_base)
        sub.append("}")
        blocks.append("\n".join(sub))
    return blocks

def build_static_parse_sub_pairs ():
    sub_pairs = []
    sub_pairs.append(("[STATIC_PARSE_HEADER_PARSER_INCLUDES_TAG]", join_tag_lines(lines_header_parser_includes(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_HEADER_DATA_INCLUDES_TAG]", join_tag_lines(lines_header_data_includes(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_HEADER_TYPED_GETTERS_TAG]", join_tag_lines(lines_header_typed_getter_blocks(), "\n\n    ")))
    sub_pairs.append(("[STATIC_PARSE_HEADER_NAME_PARSER_GETTERS_TAG]", join_tag_lines(lines_header_name_parser_getter_blocks(), "\n\n    ")))
    sub_pairs.append(("[STATIC_PARSE_HEADER_RAW_GETTERS_TAG]", join_tag_lines(lines_header_raw_getter_blocks(), "\n\n    ")))
    sub_pairs.append(("[STATIC_PARSE_HEADER_MAP_GETTERS_TAG]", join_tag_lines(lines_header_map_getter_blocks(), "\n\n    ")))
    sub_pairs.append(("[STATIC_PARSE_HEADER_READERS_TAG]", join_tag_lines(lines_header_readers())))
    sub_pairs.append(("[STATIC_PARSE_HEADER_NAME_PARSERS_TAG]", join_tag_lines(lines_header_name_parsers())))
    sub_pairs.append(("[STATIC_PARSE_HEADER_MAP_BANK_PTRS_TAG]", join_tag_lines(lines_header_map_bank_ptrs())))
    sub_pairs.append(("[STATIC_PARSE_HEADER_DATA_PTRS_TAG]", join_tag_lines(lines_header_data_ptrs())))
    sub_pairs.append(("[STATIC_PARSE_CPP_MAP_INCLUDES_TAG]", join_tag_lines(lines_cpp_map_includes(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_CALLBACK_PARSERS_TAG]", join_tag_lines(lines_cpp_callback_parsers(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_CALLBACK_FUNCS_TAG]", join_tag_lines(lines_cpp_callback_func_blocks(), "\n\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_READER_INITS_TAG]", join_tag_lines(lines_cpp_reader_inits(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_CPP_NAME_PARSER_INITS_TAG]", join_tag_lines(lines_cpp_name_parser_inits(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_CPP_MAP_BANK_INITS_TAG]", join_tag_lines(lines_cpp_map_bank_inits(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_CPP_DATA_PTR_INITS_TAG]", join_tag_lines(lines_cpp_data_ptr_inits(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_CPP_TYPED_GETTERS_TAG]", join_tag_lines(lines_cpp_typed_getter_blocks(), "\n\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_NAME_PARSER_GETTERS_TAG]", join_tag_lines(lines_cpp_name_parser_getter_blocks(), "\n\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_RAW_GETTERS_TAG]", join_tag_lines(lines_cpp_raw_getter_blocks(), "\n\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_MAP_GETTERS_TAG]", join_tag_lines(lines_cpp_map_getter_blocks(), "\n\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_RELEASE_MAP_BANKS_TAG]", join_tag_lines(lines_cpp_release_map_banks(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_CALLBACK_ASSIGN_PARSERS_TAG]", join_tag_lines(lines_cpp_callback_assign_parsers())))
    sub_pairs.append(("[STATIC_PARSE_CPP_CALLBACK_ASSIGN_CBS_TAG]", join_tag_lines(lines_cpp_callback_assign_cbs())))
    sub_pairs.append(("[STATIC_PARSE_CPP_CALLBACK_COUNT_TAG]", str(len(entries))))
    sub_pairs.append(("[STATIC_PARSE_CPP_LOCAL_PARSERS_TAG]", join_tag_lines(lines_cpp_local_parsers())))
    sub_pairs.append(("[STATIC_PARSE_CPP_PARSE_ASSIGNMENTS_TAG]", join_tag_lines(lines_cpp_parse_assignments())))
    sub_pairs.append(("[STATIC_PARSE_CPP_LOAD_ITEMS_TAG]", join_tag_lines(lines_cpp_load_items())))
    sub_pairs.append(("[STATIC_PARSE_CPP_INIT_NAME_PARSERS_TAG]", join_tag_lines(lines_cpp_init_name_parsers())))
    sub_pairs.append(("[STATIC_PARSE_CPP_BUILD_MAPS_TAG]", join_tag_lines(lines_cpp_build_maps())))
    sub_pairs.append(("[STATIC_PARSE_CPP_DELETE_MAP_BANKS_TAG]", join_tag_lines(lines_cpp_delete_map_banks())))
    sub_pairs.append(("[STATIC_PARSE_CPP_DELETE_NAME_PARSERS_TAG]", join_tag_lines(lines_cpp_delete_name_parsers())))
    sub_pairs.append(("[STATIC_PARSE_TESTER_PRINT_COUNTS_TAG]", join_tag_lines(lines_tester_print_counts())))
    sub_pairs.append(("[STATIC_PARSE_TESTER_REQ_TESTS_TAG]", join_tag_lines(lines_tester_req_test_blocks(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_TESTER_MAP_INCLUDES_TAG]", join_tag_lines(lines_tester_map_includes(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_TESTER_MAP_TESTS_TAG]", join_tag_lines(lines_tester_map_tests(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_COMP_COMPILE_MAPS_TAG]", join_tag_lines(lines_comp_compile_maps(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_COMP_COMPILE_PARSERS_TAG]", join_tag_lines(lines_comp_compile_parsers(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_COMP_LINK_MAPS_TAG]", join_tag_lines(lines_comp_link_maps(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_COMP_LINK_PARSERS_TAG]", join_tag_lines(lines_comp_link_parsers(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_COMP_CLEAN_MAPS_TAG]", join_tag_lines(lines_comp_clean_maps(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_COMP_CLEAN_PARSERS_TAG]", join_tag_lines(lines_comp_clean_parsers(), "\n    ")))
    return sub_pairs

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))
    static_parse_files = [
        ("static_parsing_manager.h", "static_parsing_manager.h"),
        ("static_parsing_manager.cpp", "static_parsing_manager.cpp"),
        ("static_parsing_manager_tester.cpp", "static_parsing_manager_tester.cpp"),
        ("static_parsing_manager_comp", "static_parsing_manager_comp"),
    ]
    for template_name, output_name in static_parse_files:
        generate_from_templates(this_dir, template_name, output_name, build_static_parse_sub_pairs())

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
