#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
import shutil
import subprocess

from generate_dyn_state import template_files, get_output_filename

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

    dyn_state_dir = os.path.join(os.path.dirname(__file__), "..", "dyn_state")
    total_failures = 0

    for output_prefix, class_name in add_pairs:
        print("\n=======================================================")
        print(" Generating:", class_name, " (%s)" % output_prefix)
        print("=======================================================")

        gen_cmd = ["python3", "generate_dyn_state.py", output_prefix, class_name]
        subprocess.run(gen_cmd, check=True)

        comp_script = "./" + output_prefix + "_vector_comp"
        subprocess.run([comp_script], check=True)

        tester_bins = []
        tester_bins.append("./" + output_prefix + "_vector_tester")
        tester_bins.append("./" + output_prefix + "_visual_tester")

        for tester_bin in tester_bins:
            test_result = subprocess.run([tester_bin], check=False)
            if test_result.returncode != 0:
                total_failures += 1
            os.remove(tester_bin)

        for template_file in template_files:
            output_file = get_output_filename(output_prefix, template_file)
            shutil.move(output_file, os.path.join(dyn_state_dir, output_file))

    print("\n\n=======================================================")
    print(" TOTAL FAILING TEST DRIVERS:", total_failures)
    print("=======================================================")

    if total_failures > 0:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#

