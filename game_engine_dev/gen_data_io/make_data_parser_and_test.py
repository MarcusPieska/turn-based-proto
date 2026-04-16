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
    #specs.append(("building", "Building", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))
    #specs.append(("city_flag", "CityFlag", ""))
    #specs.append(("civ", "Civ", "traits,1,CivTraitStruct"))
    #specs.append(("civ_trait", "CivTrait", ""))
    #specs.append(("resource", "Resource", "food,1,u16:shields,2,u16:commerce,3,u16:reqs,4,ItemReqsStruct"))
    #specs.append(("small_wonder", "SmallWonder", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))
    #specs.append(("tech", "Tech", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))
    #specs.append(("unit", "Unit", "type,1,UnitType:cost,2,u32:attack,3,u16:defense,4,u16:mvt_pts,5,u16:reqs,6,ItemReqsStruct"))
    #specs.append(("unit_type", "UnitType", ""))
    specs.append(("unit_abilities", "UnitAbilities", ""))
    #specs.append(("wonder", "Wonder", "cost,1,u32:reqs,2,ItemReqsStruct:effects,3,ItemEffectsStruct"))

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
