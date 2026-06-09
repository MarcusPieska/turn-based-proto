#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import subprocess
import os
from generate import ASSESSOR_SPECS, generate_all_files, migrate_to_city, remove_executables

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))

    total_failures = 0

    print("=======================================================")
    print(" Generating assessor outputs")
    print("=======================================================")
    generate_all_files()

    print("\n=======================================================")
    print(" Building and running assessor_tester")
    print("=======================================================")

    comp_script = os.path.join(this_dir, "assessor_tester_comp")
    subprocess.run([comp_script], check=True, cwd=this_dir)

    tester_bin = os.path.join(this_dir, "assessor_tester")
    test_result = subprocess.run([tester_bin], check=False, cwd=this_dir)
    if test_result.returncode != 0:
        total_failures += 1

    remove_executables(this_dir)

    for output_prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        to_match = "RESULTS_TO_MATCH_%s" % output_prefix.upper()
        readable = "RESULTS_READABLE_%s" % output_prefix.upper()
        if os.path.isfile(os.path.join(this_dir, to_match)):
            print(" Wrote:", to_match)
        else:
            print(" Missing:", to_match)
            total_failures += 1
        if os.path.isfile(os.path.join(this_dir, readable)):
            print(" Wrote:", readable)
        else:
            print(" Missing:", readable)
            total_failures += 1

    print("\n=======================================================")
    print(" Validating RESULTS_TO_MATCH vs game_config")
    print("=======================================================")
    validate_result = subprocess.run(["python3", "assessor_results_validate.py"], check=False, cwd=this_dir)
    if validate_result.returncode != 0:
        total_failures += 1

    if total_failures == 0:
        print("\n=======================================================")
        print(" Migrating sources to city/")
        print("=======================================================")
        migrate_to_city(this_dir)

    print("\n=======================================================")
    print(" TOTAL FAILURES:", total_failures)
    print("=======================================================")

    if total_failures > 0:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
