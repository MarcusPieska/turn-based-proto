#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import subprocess
import os

from generate import ASSESSOR_SPECS, generate_template_files

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(this_dir)

    total_failures = 0

    print("=======================================================")
    print(" Generating template outputs")
    print("=======================================================")
    generate_template_files()

    for output_prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        print("\n=======================================================")
        print(" Generating assessor tester:", class_name, " (%s)" % output_prefix)
        print("=======================================================")

        gen_cmd = ["python3", "generate.py", output_prefix, class_name]
        subprocess.run(gen_cmd, check=True)

        comp_script = "./" + output_prefix + "_assessor_tester_comp"
        subprocess.run([comp_script], check=True)

        tester_bin = "./" + output_prefix + "_assessor_tester"
        test_result = subprocess.run([tester_bin], check=False)
        if test_result.returncode != 0:
            total_failures += 1

        to_match = "RESULTS_TO_MATCH_%s" % output_prefix.upper()
        readable = "RESULTS_READABLE_%s" % output_prefix.upper()
        if os.path.isfile(to_match):
            print(" Wrote:", to_match)
        else:
            print(" Missing:", to_match)
            total_failures += 1
        if os.path.isfile(readable):
            print(" Wrote:", readable)
        else:
            print(" Missing:", readable)
            total_failures += 1

        print("=======================================================")
        print(" Done:", class_name)
        print("=======================================================")

    print("\n=======================================================")
    print(" Validating RESULTS_TO_MATCH vs game_config")
    print("=======================================================")
    validate_result = subprocess.run(["python3", "assessor_results_validate.py"], check=False)
    if validate_result.returncode != 0:
        total_failures += 1

    print("\n=======================================================")
    print(" TOTAL FAILING ASSESSOR DRIVERS:", total_failures)
    print("=======================================================")

    if total_failures > 0:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
