#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import subprocess

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    add_pairs = []
    add_pairs.append(("buildings_built", "BuildingsBuilt"))
    add_pairs.append(("techs_researched", "TechsResearched"))
    add_pairs.append(("resources_available", "ResourcesAvailable"))
    add_pairs.append(("civs_loaded", "CivsLoaded"))
    add_pairs.append(("civ_traits", "CivTraits"))
    add_pairs.append(("city_flags", "CityFlags"))

    total_failures = 0

    for output_prefix, class_name in add_pairs:
        print("\n=======================================================")
        print(" Generating:", class_name, " (%s)" % output_prefix)
        print("=======================================================")

        gen_cmd = ["python3", "generate.py", output_prefix, class_name]
        subprocess.run(gen_cmd, check=True)

        comp_script = "./" + output_prefix + "_array_comp"
        subprocess.run([comp_script], check=True)

        tester_bin = "./" + output_prefix + "_array_tester"
        test_result = subprocess.run([tester_bin], check=False)

        if test_result.returncode != 0:
            total_failures += 1

    print("\n\n=======================================================")
    print(" TOTAL FAILING TEST DRIVERS:", total_failures)
    print("=======================================================")

    if total_failures > 0:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
