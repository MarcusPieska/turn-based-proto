#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - Parse config stems -
#================================================================================================================================#

entries = []

entries.append("building") 
entries.append("city_flag")
entries.append("civ")
entries.append("civ_trait")
entries.append("resource")
entries.append("small_wonder")
entries.append("tech")
entries.append("unit")
entries.append("unit_action")
entries.append("unit_type")
entries.append("wonder")

req_test_stems = []
req_test_stems.append("tech")
req_test_stems.append("resource")
req_test_stems.append("city_flag")
req_test_stems.append("building")
req_test_stems.append("unit")
req_test_stems.append("wonder")
req_test_stems.append("small_wonder")

map_specs = []
map_specs.append(("unit_type_action_map", "unit_type", "unit_action"))
map_specs.append(("civ_bld_discount_map", "civ_trait", "building"))

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def join_tag_lines(lines, indent_between="\n    "):
    return indent_between.join(lines)

def to_pascal(snake):
    return "".join([part.capitalize() for part in snake.split("_")])

def parser_header(stem):
    return "%s_parser.h" % stem

def data_header(stem):
    return "%s_static_data.h" % stem

def parser_class(stem):
    return "%sParser" % to_pascal(stem)

def data_struct(stem):
    return "%sStaticDataStruct" % to_pascal(stem)

def path_from_stem(stem):
    return stem + "s"

def lines_header_parser_includes():
    return ['#include "%s"' % parser_header(stem) for stem in entries]

def lines_header_data_includes():
    return ['#include "%s"' % data_header(stem) for stem in entries]

def lines_header_typed_getter_blocks():
    lines = []
    for stem in entries:
        lines.append("const %s* get_%s_data () const;" % (data_struct(stem), stem))
        lines.append("u16 get_%s_count () const;" % stem)
    return lines

def lines_header_raw_getter_blocks():
    return []

def lines_header_readers():
    return ["StringManager m_%s_items;" % stem for stem in entries]

def lines_header_name_parsers():
    return ["DataParserBase* m_%s_name_parser;" % stem for stem in entries]

def lines_header_data_ptrs():
    return ["%s* m_%s_data;" % (data_struct(stem), stem) for stem in entries]

def lines_cpp_callback_parsers():
    return ["const DataParserBase* g_%s_name_parser = nullptr;" % stem for stem in entries]

def lines_cpp_callback_func_blocks():
    blocks = []
    for stem in entries:
        lines = []
        lines.append("u16 cb_%s_name_to_idx (cstr name) {" % stem)
        lines.append("    return g_%s_name_parser->name_to_idx(name);" % stem)
        lines.append("}")
        blocks.append("\n".join(lines))
    return blocks

def lines_cpp_reader_inits():
    return ["m_%s_items()," % stem for stem in entries]

def lines_cpp_name_parser_inits():
    return ["m_%s_name_parser(nullptr)," % stem for stem in entries]

def lines_cpp_data_ptr_inits():
    lines = []
    for i, stem in enumerate(entries):
        suffix = "," if i < len(entries) - 1 else ""
        lines.append("m_%s_data(nullptr)%s" % (stem, suffix))
    return lines

def lines_cpp_typed_getter_blocks():
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

def lines_cpp_raw_getter_blocks():
    return []

def lines_cpp_callback_assign_parsers():
    return ["g_%s_name_parser = m_%s_name_parser;" % (stem, stem) for stem in entries]

def lines_cpp_callback_assign_cbs():
    return ["m_name_to_idx_cbs.%s_name_to_idx = cb_%s_name_to_idx;" % (stem, stem) for stem in entries]

def lines_cpp_local_parsers():
    return ["%s %s_parser(m_%s_items, m_name_to_idx_cbs);" % (parser_class(stem), stem, stem) for stem in entries]

def lines_cpp_load_items():
    lines = [
        "m_effect_items.split_string_by_char(0, '\\n');",
        "m_effect_items.cull_empty_strings();",
    ]
    for stem in entries:
        lines.append("m_%s_items.load_file_content(m_paths.get_path_to_%s().c_str());" % (stem, path_from_stem(stem)))
        lines.append("m_%s_items.split_string_by_char(0, '\\n');" % stem)
        lines.append("m_%s_items.cull_empty_strings();" % stem)
    return lines

def lines_cpp_init_name_parsers():
    return ["m_%s_name_parser = new DataParserBase(m_%s_items, NameToIdxCbs());" % (stem, stem) for stem in entries]

def lines_cpp_delete_name_parsers():
    return ["delete m_%s_name_parser;" % stem for stem in entries]

def lines_cpp_parse_assignments():
    return ["m_%s_data = %s_parser.parse_data_dependencies();" % (stem, stem) for stem in entries]

def lines_tester_print_counts():
    return ['print_u16_member("%s", parser.get_%s_count());' % (stem, stem) for stem in entries]

def lines_tester_req_test_blocks():
    lines = []
    for i, stem in enumerate(req_test_stems):
        pre = "bool result = " if i == 0 else "result = "
        lines.append("%stest_dataset_req_bounds(parser, parser.get_%s_data(), parser.get_%s_count(), \"%s\");" % (
            pre, stem, stem, stem))
        lines.append("note_result(result, \"%s req indices in bounds\");" % data_struct(stem))
        if i < len(req_test_stems) - 1:
            lines.append("")
    return lines

def lines_comp_compile_parsers():
    return ["g++ $INC -c %s_parser.cpp -o %s_parser.o" % (stem, stem) for stem in entries]

def lines_comp_link_parsers():
    return ["%s_parser.o \\" % stem for stem in entries]

def lines_comp_clean_parsers():
    return ["%s_parser.o \\" % stem for stem in entries]

def map_class_name(map_base):
    return "".join([part.capitalize() for part in map_base.split("_")])

def map_col_getter_method_suffix(col_stem):
    if col_stem == "unit_action":
        return "action"
    return col_stem

def lines_header_map_bank_ptrs():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("StaticBitBank* m_%s_bank;" % map_base)
    return lines

def lines_cpp_map_includes():
    lines = []
    lines.append("#include \"static_bit_bank.h\"")
    seen = set()
    for map_base, row_stem, col_stem in map_specs:
        for hdr in (
            parser_header(row_stem),
            parser_header(col_stem),
            "%s.h" % map_base,
            "%s_parsing.h" % map_base,
        ):
            if hdr not in seen:
                seen.add(hdr)
                lines.append("#include \"%s\"" % hdr)
    return lines

def lines_cpp_map_bank_inits():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("m_%s_bank(nullptr)," % map_base)
    return lines

def lines_cpp_delete_map_banks():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("delete m_%s_bank;" % map_base)
    return lines

def lines_cpp_build_maps():
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
        lines.append("%s::set_map(%s, %s, %s);" % (cls, bank, r_n, c_n))
    return lines

def lines_header_map_getter_blocks():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("StaticBitBank* get_%s_bank () const;" % map_base)
    return lines

def lines_cpp_map_getter_blocks():
    blocks = []
    for map_base, row_stem, col_stem in map_specs:
        sub = []
        sub.append("StaticBitBank* StaticParsingManager::get_%s_bank () const {" % map_base)
        sub.append("    return m_%s_bank;" % map_base)
        sub.append("}")
        blocks.append("\n".join(sub))
    return blocks

def lines_comp_compile_maps():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("g++ $INC -c ../static_state/%s.cpp -o %s.o" % (map_base, map_base))
        lines.append("g++ $INC -c %s_parsing.cpp -o %s_parsing.o" % (map_base, map_base))
    return lines

def lines_comp_link_maps():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("%s.o \\" % map_base)
        lines.append("%s_parsing.o \\" % map_base)
    return lines

def lines_comp_clean_maps():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("%s.o \\" % map_base)
        lines.append("%s_parsing.o \\" % map_base)
    return lines

def lines_tester_map_includes():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        lines.append("#include \"%s.h\"" % map_base)
    return lines

def lines_tester_map_tests():
    lines = []
    for map_base, row_stem, col_stem in map_specs:
        cls = map_class_name(map_base)
        col_getter = map_col_getter_method_suffix(col_stem)
        bank_fn = "get_%s_bank" % map_base
        lines.append("note_result(%s::get_%s_count() == parser.get_%s_count(), \"%s map row count\");" % (
            cls, row_stem, row_stem, map_base))
        lines.append("note_result(%s::get_%s_count() == parser.get_%s_count(), \"%s map col count\");" % (
            cls, col_getter, col_stem, map_base))
        lines.append("note_result(parser.%s()->get_array_count() == parser.get_%s_count(), \"%s bank array_count\");" % (
            bank_fn, row_stem, map_base))
        lines.append("note_result(parser.%s()->get_array_size() == parser.get_%s_count(), \"%s bank array_size\");" % (
            bank_fn, col_stem, map_base))
    lines.append("if (print_level >= 1) {")
    for map_base, row_stem, col_stem in map_specs:
        bank_fn = "get_%s_bank" % map_base
        lines.append("    printf(\" %s bank: array_count=%%u array_size=%%u\\n\", parser.%s()->get_array_count(), parser.%s()->get_array_size());" % (
            map_base, bank_fn, bank_fn))
    lines.append("}")
    return lines

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))
    sub_pairs = []

    sub_pairs.append(("[STATIC_PARSE_HEADER_PARSER_INCLUDES_TAG]", join_tag_lines(lines_header_parser_includes(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_HEADER_DATA_INCLUDES_TAG]", join_tag_lines(lines_header_data_includes(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_HEADER_TYPED_GETTERS_TAG]", join_tag_lines(lines_header_typed_getter_blocks(), "\n\n    ")))
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
    sub_pairs.append(("[STATIC_PARSE_CPP_RAW_GETTERS_TAG]", join_tag_lines(lines_cpp_raw_getter_blocks(), "\n\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_MAP_GETTERS_TAG]", join_tag_lines(lines_cpp_map_getter_blocks(), "\n\n")))
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

    file_prefix = "TEMPLATE_"
    files = []
    files.append("static_parsing_manager.h")
    files.append("static_parsing_manager.cpp")
    files.append("static_parsing_manager_tester.cpp")
    files.append("static_parsing_manager_comp")

    for output_path in files:
        template_path = os.path.join(this_dir, file_prefix + output_path)
        out_path = os.path.join(this_dir, output_path)
        with open(template_path, "r") as ptr:
            content = ptr.read()
        for old_string, new_string in sub_pairs:
            content = content.replace(old_string, new_string)
        with open(out_path, "w") as ptr:
            ptr.write(content)
        if output_path.endswith("_comp"):
            os.chmod(out_path, 0o755)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
