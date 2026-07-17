#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - Parse config -
#================================================================================================================================#

def get_entries ():
    return [
        "building",
        "city_flag",
        "civ",
        "civ_trait",
        "tile_attribute",
        "resource",
        "res_dist",
        "small_wonder",
        "tech",
        "unit",
        "unit_action",
        "unit_type",
        "wonder",
    ]

def get_req_test_stems ():
    return [
        "tech",
        "resource",
        "city_flag",
        "building",
        "unit",
        "wonder",
        "small_wonder",
    ]

def get_map_specs ():
    return [
        ("unit_type_action_map", "unit_type", "unit_action"),
        ("civ_bld_discount_map", "civ_trait", "building"),
    ]

#================================================================================================================================#
#=> - Name helpers -
#================================================================================================================================#

def join_tag_lines (lines, indent_between="\n    "):
    return indent_between.join(lines)

def to_pascal (snake):
    return "".join([part.capitalize() for part in snake.split("_")])

def parser_header (stem):
    return "%s_parser.h" % stem

def data_header (stem):
    return "%s_static_data.h" % stem

def parser_class (stem):
    return "%sParser" % to_pascal(stem)

def data_struct (stem):
    return "%sStaticDataStruct" % to_pascal(stem)

def static_data_class (stem):
    return "%sStaticData" % to_pascal(stem)

def static_data_key (stem):
    return "%sStaticDataKey" % to_pascal(stem)

def path_from_stem (stem):
    return stem + "s"

def map_class_name (map_base):
    return "".join([part.capitalize() for part in map_base.split("_")])

def map_col_getter_method_suffix (col_stem):
    if col_stem == "unit_action":
        return "action"
    return col_stem

#================================================================================================================================#
#=> - Shared comp / tester fragments -
#================================================================================================================================#

def lines_comp_compile_maps ():
    lines = []
    for map_base, row_stem, col_stem in get_map_specs():
        lines.append("g++ $INC -c ../static_state/%s.cpp -o %s.o" % (map_base, map_base))
        lines.append("g++ $INC -c %s_parsing.cpp -o %s_parsing.o" % (map_base, map_base))
    return lines

def lines_comp_link_maps ():
    lines = []
    for map_base, row_stem, col_stem in get_map_specs():
        lines.append("%s.o \\" % map_base)
        lines.append("%s_parsing.o \\" % map_base)
    return lines

def lines_comp_clean_maps ():
    lines = []
    for map_base, row_stem, col_stem in get_map_specs():
        lines.append("%s.o \\" % map_base)
        lines.append("%s_parsing.o \\" % map_base)
    return lines

def lines_comp_compile_parsers ():
    return ["g++ $INC -c %s_parser.cpp -o %s_parser.o" % (stem, stem) for stem in get_entries()]

def lines_comp_link_parsers ():
    return ["%s_parser.o \\" % stem for stem in get_entries()]

def lines_comp_clean_parsers ():
    return ["%s_parser.o \\" % stem for stem in get_entries()]

def lines_comp_compile_parser_test_suite ():
    lines = []
    lines.append("g++ $INC $CXXFLAGS -c ../misc/opt_str_mng.cpp -o opt_str_mng.o")
    lines.append("g++ $INC $CXXFLAGS -c path_mng.cpp -o path_mng.o")
    lines.append("g++ $INC $CXXFLAGS -c data_parser_base.cpp -o data_parser_base.o")
    lines.append("g++ $INC $CXXFLAGS -c item_effect_handler.cpp -o item_effect_handler.o")
    lines.append("g++ $INC $CXXFLAGS -c item_effect_helpers.cpp -o item_effect_helpers.o")
    for stem in get_entries():
        lines.append("g++ $INC $CXXFLAGS -c %s_parser.cpp -o %s_parser.o" % (stem, stem))
        lines.append("g++ $INC $CXXFLAGS -c %s_parser_tester.cpp -o %s_parser_tester.o" % (stem, stem))
    lines.append("g++ $INC $CXXFLAGS -c parser_test_manager.cpp -o parser_test_manager.o")
    return lines

def lines_comp_link_parser_test_suite ():
    lines = [
        "opt_str_mng.o \\",
        "path_mng.o \\",
        "item_effect_handler.o \\",
        "item_effect_helpers.o \\",
        "data_parser_base.o \\",
    ]
    for stem in get_entries():
        lines.append("%s_parser.o \\" % stem)
        lines.append("%s_parser_tester.o \\" % stem)
    lines.append("parser_test_manager.o \\")
    return lines

def lines_comp_clean_parser_test_suite ():
    lines = [
        "opt_str_mng.o \\",
        "path_mng.o \\",
        "item_effect_handler.o \\",
        "item_effect_helpers.o \\",
        "data_parser_base.o \\",
    ]
    for stem in get_entries():
        lines.append("%s_parser.o \\" % stem)
        lines.append("%s_parser_tester.o \\" % stem)
    lines.append("parser_test_manager.o \\")
    return lines

def lines_tester_map_includes ():
    lines = []
    for map_base, row_stem, col_stem in get_map_specs():
        lines.append("#include \"%s.h\"" % map_base)
    return lines

def lines_tester_map_tests ():
    lines = []
    for map_base, row_stem, col_stem in get_map_specs():
        col_getter = map_col_getter_method_suffix(col_stem)
        lines.append("note_result(statics.%s().get_%s_count() == parser.get_%s_count(), \"%s map row count\");" % (
            map_base, row_stem, row_stem, map_base))
        lines.append("note_result(statics.%s().get_%s_count() == parser.get_%s_count(), \"%s map col count\");" % (
            map_base, col_getter, col_stem, map_base))
        lines.append("note_result(statics.%s().get_%s_count() == statics.%s().get_item_count(), \"%s row match\");" % (
            map_base, row_stem, row_stem, map_base))
        lines.append("note_result(statics.%s().get_%s_count() == statics.%s().get_item_count(), \"%s col match\");" % (
            map_base, col_getter, col_stem, map_base))
    lines.append("if (print_level >= 1) {")
    for map_base, row_stem, col_stem in get_map_specs():
        col_getter = map_col_getter_method_suffix(col_stem)
        lines.append("    printf(\" %s: row_count=%%u col_count=%%u\\n\", statics.%s().get_%s_count(), statics.%s().get_%s_count());" % (
            map_base, map_base, row_stem, map_base, col_getter))
    lines.append("}")
    return lines

#================================================================================================================================#
#=> - Template generation -
#================================================================================================================================#

def generate_from_templates (this_dir, template_name, output_name, sub_pairs):
    template_path = os.path.join(this_dir, "TEMPLATE_" + template_name)
    out_path = os.path.join(this_dir, output_name)
    with open(template_path, "r") as ptr:
        content = ptr.read()
    for old_string, new_string in sub_pairs:
        content = content.replace(old_string, new_string)
    with open(out_path, "w") as ptr:
        ptr.write(content)
    if output_name.endswith("_comp"):
        os.chmod(out_path, 0o755)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
