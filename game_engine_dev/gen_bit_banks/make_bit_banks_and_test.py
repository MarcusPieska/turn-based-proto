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
    bank_specs = []
    bank_specs.append(("general", "General", "City"))

    total_failures = 0

    for output_prefix, class_name, user_class_name in bank_specs:
        print("\n=======================================================")
        print(" Generating:", class_name, " (%s)" % output_prefix)
        print("=======================================================")

        gen_cmd = ["python3", "generate.py", output_prefix, class_name, user_class_name]
        subprocess.run(gen_cmd, check=True)

        comp_script = "./" + output_prefix + "_bit_bank_comp"
        subprocess.run([comp_script], check=True)

        tester_bin = "./" + output_prefix + "_bit_bank_tester"
        test_result = subprocess.run([tester_bin], check=False)
        if test_result.returncode != 0:
            total_failures += 1
        os.remove(tester_bin)

        tester_bin = "./" + output_prefix + "_bit_bank_major1_tester"
        test_result = subprocess.run([tester_bin], check=False)
        if test_result.returncode != 0:
            total_failures += 1
        os.remove(tester_bin)

        tester_bin = "./" + output_prefix + "_bit_bank_major2_tester"
        test_result = subprocess.run([tester_bin], check=False)
        if test_result.returncode != 0:
            total_failures += 1
        os.remove(tester_bin)

        tester_bin = "./" + output_prefix + "_bit_bank_visual1_tester"
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
