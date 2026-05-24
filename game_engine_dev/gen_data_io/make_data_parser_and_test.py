#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import subprocess
import os

from generate import PARSER_SPECS

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def select_specs(specs, selected_prefixes):
    return [spec for spec in specs if spec[0] in selected_prefixes]

def apply_substitution(content, old_string, new_string):
    return content.replace(old_string, new_string)

def derive_manager_includes(specs):
    lines = []
    for prefix, class_name, parsing_instructions in specs:
        lines.append('#include "%s_parser_tester.h"' % prefix)
    return lines

def derive_manager_members(specs):
    lines = []
    for prefix, class_name, parsing_instructions in specs:
        lines.append("%sParserTester m_%s;" % (class_name, prefix))
    return lines

def derive_manager_accessors(specs):
    lines = []
    for prefix, class_name, parsing_instructions in specs:
        lines.append("%sParserTester& %s () { return m_%s; }" % (class_name, prefix, prefix))
    return lines

def derive_manager_set_plvl(specs):
    lines = []
    for prefix, class_name, parsing_instructions in specs:
        lines.append("m_%s.set_plvl(lvl);" % prefix)
    return lines

def derive_manager_wire_sd(specs):
    lines = []
    for prefix, class_name, parsing_instructions in specs:
        for sd_prefix, sd_class_name, sd_instr in specs:
            lines.append("m_%s.set_%s_sd(&statics.%s());" % (prefix, sd_prefix, sd_prefix))
    return lines

def derive_manager_clr_sd(specs):
    lines = []
    for prefix, class_name, parsing_instructions in specs:
        for sd_prefix, sd_class_name, sd_instr in specs:
            lines.append("m_%s.set_%s_sd(NULL);" % (prefix, sd_prefix))
    return lines

def derive_manager_print_all(specs):
    lines = []
    for prefix, class_name, parsing_instructions in specs:
        key_name = class_name + "StaticDataKey"
        lines.append("m_%s.open_writer();" % prefix)
        lines.append("for (u16 i = 0; i < statics.%s().get_item_count(); ++i) {" % prefix)
        lines.append("    m_%s.pr_item(statics.%s().get_item(%s::from_raw(i)));" % (prefix, prefix, key_name))
        lines.append("}")
        lines.append("m_%s.close_writer();" % prefix)
        lines.append("")
    if len(lines) > 0 and lines[-1] == "":
        lines.pop()
    return lines

def generate_parser_test_manager(specs):
    this_dir = os.path.dirname(os.path.abspath(__file__))
    sub_pairs = []
    sub_pairs.append(("[MANAGER_INCLUDES_TAG]", "\n".join(derive_manager_includes(specs))))
    sub_pairs.append(("[MANAGER_MEMBERS_TAG]", "\n    ".join(derive_manager_members(specs))))
    sub_pairs.append(("[MANAGER_ACCESSORS_TAG]", "\n    ".join(derive_manager_accessors(specs))))
    sub_pairs.append(("[MANAGER_SET_PLVL_TAG]", "\n    ".join(derive_manager_set_plvl(specs))))
    sub_pairs.append(("[MANAGER_WIRE_SD_TAG]", "\n    ".join(derive_manager_wire_sd(specs))))
    sub_pairs.append(("[MANAGER_PRINT_ALL_TAG]", "\n    ".join(derive_manager_print_all(specs))))
    sub_pairs.append(("[MANAGER_CLR_SD_TAG]", "\n    ".join(derive_manager_clr_sd(specs))))
    template_files = ["TEMPLATE_parser_test_manager.h", "TEMPLATE_parser_test_manager.cpp"]
    output_files = ["parser_test_manager.h", "parser_test_manager.cpp"]
    for template_file, output_file in zip(template_files, output_files):
        template_path = os.path.join(this_dir, template_file)
        out_path = os.path.join(this_dir, output_file)
        with open(template_path, "r") as ptr:
            content = ptr.read()
        for old_string, new_string in sub_pairs:
            content = apply_substitution(content, old_string, new_string)
        with open(out_path, "w") as ptr:
            ptr.write(content)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    specs = PARSER_SPECS
    #specs = select_specs(specs, ["building"])

    total_failures = 0

    for output_prefix, class_name, parsing_instructions in specs:
        print("\n=======================================================")
        print(" Generating:", class_name, " (%s)" % output_prefix)
        print("=======================================================")

        struct_name = class_name + "StaticDataStruct"
        gen_cmd = ["python3", "generate.py", output_prefix, class_name, struct_name, parsing_instructions]
        subprocess.run(gen_cmd, check=True)

        comp_script = "./" + output_prefix + "_parser_comp"
        subprocess.run([comp_script], check=True)

        tester_bin = "./" + output_prefix + "_parser_tester"
        test_result = subprocess.run([tester_bin, "3"], check=False)
        if test_result.returncode != 0:
            total_failures += 1
        os.remove(tester_bin)

        print("=======================================================")
        print(" Generated:", class_name, " (%s)" % output_prefix)
        print("=======================================================")
        input(" Press Enter to continue:")

    print("\n=======================================================")
    print(" Generating: ParserTestManager")
    print("=======================================================")
    generate_parser_test_manager(PARSER_SPECS)

    print("\n\n=======================================================")
    print(" TOTAL FAILING TEST DRIVERS:", total_failures)
    print("=======================================================")

    if total_failures > 0:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
