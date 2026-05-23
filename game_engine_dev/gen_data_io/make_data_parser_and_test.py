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

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    specs = PARSER_SPECS
    #specs = select_specs (specs, "buildings")

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

    print("\n\n=======================================================")
    print(" TOTAL FAILING TEST DRIVERS:", total_failures)
    print("=======================================================")

    if total_failures > 0:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
