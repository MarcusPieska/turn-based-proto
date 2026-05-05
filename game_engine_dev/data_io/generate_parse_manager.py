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
        lines.append("const %s* get_%s_data () const;\n    u16 get_%s_count () const;" % (
            data_struct(stem), stem, stem))
    return lines

def lines_header_raw_getter_blocks():
    return []

def lines_header_readers():
    return ["DataReader m_%s_reader;" % stem for stem in entries]

def lines_header_name_parsers():
    return ["DataParserBase m_%s_name_parser;" % stem for stem in entries]

def lines_header_data_ptrs():
    return ["%s* m_%s_data;" % (data_struct(stem), stem) for stem in entries]

def lines_cpp_callback_parsers():
    return ["const DataParserBase* g_%s_name_parser = nullptr;" % stem for stem in entries]

def lines_cpp_callback_func_blocks():
    blocks = []
    for stem in entries:
        blocks.append("u16 cb_%s_name_to_idx (cstr name) {\n    return g_%s_name_parser->name_to_idx(name);\n}" % (
            stem, stem))
    return blocks

def lines_cpp_reader_inits():
    return ["m_%s_reader(m_paths.get_path_to_%s())," % (stem, path_from_stem(stem)) for stem in entries]

def lines_cpp_name_parser_inits():
    return ["m_%s_name_parser(m_%s_reader.get_raw_items(), NameToIdxCbs())," % (stem, stem) for stem in entries]

def lines_cpp_data_ptr_inits():
    lines = []
    for i, stem in enumerate(entries):
        suffix = "," if i < len(entries) - 1 else ""
        lines.append("m_%s_data(nullptr)%s" % (stem, suffix))
    return lines

def lines_cpp_typed_getter_blocks():
    blocks = []
    for stem in entries:
        blocks.append("const %s* StaticParsingManager::get_%s_data () const {\n    return m_%s_data;\n}\n\nu16 StaticParsingManager::get_%s_count () const {\n    return safe_size_to_u16(m_%s_reader.get_raw_items().size());\n}" % (
            data_struct(stem), stem, stem, stem, stem))
    return blocks

def lines_cpp_raw_getter_blocks():
    return []

def lines_cpp_callback_assign_parsers():
    return ["g_%s_name_parser = &m_%s_name_parser;" % (stem, stem) for stem in entries]

def lines_cpp_callback_assign_cbs():
    return ["m_name_to_idx_cbs.%s_name_to_idx = cb_%s_name_to_idx;" % (stem, stem) for stem in entries]

def lines_cpp_local_parsers():
    return ["%s %s_parser(m_%s_reader.get_raw_items(), m_name_to_idx_cbs);" % (parser_class(stem), stem, stem) for stem in entries]

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
    sub_pairs.append(("[STATIC_PARSE_HEADER_READERS_TAG]", join_tag_lines(lines_header_readers())))
    sub_pairs.append(("[STATIC_PARSE_HEADER_NAME_PARSERS_TAG]", join_tag_lines(lines_header_name_parsers())))
    sub_pairs.append(("[STATIC_PARSE_HEADER_DATA_PTRS_TAG]", join_tag_lines(lines_header_data_ptrs())))

    sub_pairs.append(("[STATIC_PARSE_CPP_CALLBACK_PARSERS_TAG]", join_tag_lines(lines_cpp_callback_parsers(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_CALLBACK_FUNCS_TAG]", join_tag_lines(lines_cpp_callback_func_blocks(), "\n\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_READER_INITS_TAG]", join_tag_lines(lines_cpp_reader_inits(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_CPP_NAME_PARSER_INITS_TAG]", join_tag_lines(lines_cpp_name_parser_inits(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_CPP_DATA_PTR_INITS_TAG]", join_tag_lines(lines_cpp_data_ptr_inits(), "\n    ")))
    sub_pairs.append(("[STATIC_PARSE_CPP_TYPED_GETTERS_TAG]", join_tag_lines(lines_cpp_typed_getter_blocks(), "\n\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_RAW_GETTERS_TAG]", join_tag_lines(lines_cpp_raw_getter_blocks(), "\n\n")))
    sub_pairs.append(("[STATIC_PARSE_CPP_CALLBACK_ASSIGN_PARSERS_TAG]", join_tag_lines(lines_cpp_callback_assign_parsers())))
    sub_pairs.append(("[STATIC_PARSE_CPP_CALLBACK_ASSIGN_CBS_TAG]", join_tag_lines(lines_cpp_callback_assign_cbs())))
    sub_pairs.append(("[STATIC_PARSE_CPP_CALLBACK_COUNT_TAG]", str(len(entries))))
    sub_pairs.append(("[STATIC_PARSE_CPP_LOCAL_PARSERS_TAG]", join_tag_lines(lines_cpp_local_parsers())))
    sub_pairs.append(("[STATIC_PARSE_CPP_PARSE_ASSIGNMENTS_TAG]", join_tag_lines(lines_cpp_parse_assignments())))

    sub_pairs.append(("[STATIC_PARSE_TESTER_PRINT_COUNTS_TAG]", join_tag_lines(lines_tester_print_counts())))
    sub_pairs.append(("[STATIC_PARSE_TESTER_REQ_TESTS_TAG]", join_tag_lines(lines_tester_req_test_blocks(), "\n    ")))

    sub_pairs.append(("[STATIC_PARSE_COMP_COMPILE_PARSERS_TAG]", join_tag_lines(lines_comp_compile_parsers(), "\n")))
    sub_pairs.append(("[STATIC_PARSE_COMP_LINK_PARSERS_TAG]", join_tag_lines(lines_comp_link_parsers(), "\n    ")))
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
