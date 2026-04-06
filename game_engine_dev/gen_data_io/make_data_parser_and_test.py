#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import subprocess
import os

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    specs = []
    specs.append(("building", "Building", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))

    

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
        test_result = subprocess.run([tester_bin], check=False)
        if test_result.returncode != 0:
            total_failures += 1
        os.remove(tester_bin)

    print("\n\n=======================================================")
    print(" TOTAL FAILING TEST DRIVERS:", total_failures)
    print("=======================================================")

    if total_failures > 0:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
