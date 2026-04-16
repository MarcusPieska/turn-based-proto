#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - Path config file endings (game_config.<ending>) -
#================================================================================================================================#

path_config_endings = []
path_config_endings.append("buildings")
path_config_endings.append("city_flags")
path_config_endings.append("civ_traits")
path_config_endings.append("civs")
path_config_endings.append("effects")
path_config_endings.append("governments")
path_config_endings.append("resources")
path_config_endings.append("small_wonders")
path_config_endings.append("techs")
path_config_endings.append("unit_abilities")
path_config_endings.append("unit_types")
path_config_endings.append("units")
path_config_endings.append("wonders")

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def join_tag_lines(lines, indent_between="\n    "):
    return indent_between.join(lines)

def lines_path_header_getters(stems):
    lines = []
    for s in stems:
        lines.append("const std::string& get_path_to_%s () const;" % s)
    return lines

def unroll_path_header_getters(stems):
    return join_tag_lines(lines_path_header_getters(stems))

def lines_path_header_members(stems):
    lines = []
    for s in stems:
        lines.append("std::string m_path_%s;" % s)
    return lines

def unroll_path_header_members(stems):
    return join_tag_lines(lines_path_header_members(stems))

def lines_path_cpp_getter_block(stem):
    lines = []
    lines.append("const std::string& PathMng::get_path_to_%s () const {" % stem)
    lines.append("    return m_path_%s;" % stem)
    lines.append("}")
    return lines

def unroll_path_cpp_getters(stems):
    blocks = []
    for s in stems:
        blocks.append(join_tag_lines(lines_path_cpp_getter_block(s), "\n"))
    return "\n\n".join(blocks)

def lines_path_cpp_build_assignments(stems):
    lines = []
    for s in stems:
        lines.append("m_path_%s = m_path_offset + \"/game_config.%s\";" % (s, s))
    return lines

def unroll_path_cpp_build_paths(stems):
    return join_tag_lines(lines_path_cpp_build_assignments(stems))

def lines_path_cpp_validate_block(stem):
    lines = []
    lines.append("if (!does_file_exist(m_path_%s)) {" % stem)
    lines.append("    printf(\"ERROR: Missing file: %%s\\n\", m_path_%s.c_str());" % stem)
    lines.append("    ++error_count;")
    lines.append("}")
    return lines

def unroll_path_cpp_validate_paths(stems):
    lines = []
    for s in stems:
        lines.extend(lines_path_cpp_validate_block(s))
    return join_tag_lines(lines, "\n    ")

def lines_path_mng_tester_note_all_paths_exist(stems):
    lines = []
    for s in stems:
        lines.append("note_result(does_file_exist(paths.get_path_to_%s()), (str(tag) + \" %s exists\").c_str());" % (s, s))
    return lines

def unroll_path_mng_tester_note_all_paths_exist(stems):
    return join_tag_lines(lines_path_mng_tester_note_all_paths_exist(stems))

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))
    sub_pairs = []

    sub_pairs.append(("[PATH_MNG_HEADER_GETTERS_TAG]", unroll_path_header_getters(path_config_endings)))
    sub_pairs.append(("[PATH_MNG_HEADER_MEMBERS_TAG]", unroll_path_header_members(path_config_endings)))
    sub_pairs.append(("[PATH_MNG_CPP_GETTERS_TAG]", unroll_path_cpp_getters(path_config_endings)))
    sub_pairs.append(("[PATH_MNG_CPP_BUILD_PATHS_TAG]", unroll_path_cpp_build_paths(path_config_endings)))
    sub_pairs.append(("[PATH_MNG_CPP_VALIDATE_PATHS_TAG]", unroll_path_cpp_validate_paths(path_config_endings)))
    sub_pairs.append(("[PATH_MNG_TESTER_NOTE_ALL_PATHS_EXIST_TAG]", unroll_path_mng_tester_note_all_paths_exist(path_config_endings)))

    file_prefix = "TEMPLATE_"
    files = []
    files.append("path_mng.h")
    files.append("path_mng.cpp")
    files.append("path_mng_tester.cpp")

    for output_path in files:
        template_path = os.path.join(this_dir, file_prefix + output_path)
        out_path = os.path.join(this_dir, output_path)
        with open(template_path, "r") as ptr:
            content = ptr.read()
        for old_string, new_string in sub_pairs:
            content = content.replace(old_string, new_string)
        with open(out_path, "w") as ptr:
            ptr.write(content)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
