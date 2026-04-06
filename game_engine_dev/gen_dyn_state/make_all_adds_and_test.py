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
    add_pairs.append(("mine_add", "MineAdd"))
    add_pairs.append(("fort_add", "FortAdd"))
    add_pairs.append(("plantation_add", "PlantationAdd"))
    add_pairs.append(("shipyard_add", "ShipyardAdd"))
    add_pairs.append(("outpost_add", "OutpostAdd"))
    add_pairs.append(("trade_post_add", "TradePostAdd"))
    add_pairs.append(("monastery_add", "MonasteryAdd"))

    total_failures = 0

    for output_prefix, class_name in add_pairs:
        print("\n=======================================================")
        print(" Generating:", class_name, " (%s)" % output_prefix)
        print("=======================================================")

        gen_cmd = ["python3", "generate.py", output_prefix, class_name]
        subprocess.run(gen_cmd, check=True)

        comp_script = "./" + output_prefix + "_vector_comp"
        subprocess.run([comp_script], check=True)

        tester_bin = "./" + output_prefix + "_vector_tester"
        test_result = subprocess.run([tester_bin], check=False)

        if test_result.returncode != 0:
            total_failures += 1

        tester_bin = "./" + output_prefix + "_vector_tester_visual"
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
