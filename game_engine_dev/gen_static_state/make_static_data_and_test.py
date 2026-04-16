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
    std_members = "std::string name; ItemReqsStruct reqs; "
    specs = []
    #specs.append(("building", "Building", std_members + "u32 cost; ItemEffectsStruct effects;", "cost", "effects.indices[0]"))
    #specs.append(("city_flag", "CityFlag", std_members + "ItemEffectsStruct effects;", "effects.indices[0]", "effects.indices[1]"))
    #specs.append(("civ", "Civ", "std::string name; CivTraitStruct traits;", "traits.indices[0]", "traits.indices[1]"))
    #specs.append(("civ_trait", "CivTrait", "std::string name;", "None", "None"))
    #specs.append(("resource", "Resource", std_members + "u16 food; u16 shields; u16 commerce;", "food", "shields"))
    #specs.append(("small_wonder", "SmallWonder", std_members + "u32 cost; ItemEffectsStruct effects;", "cost", "effects.indices[0]"))
    #specs.append(("tech", "Tech", std_members + "u32 cost; u16 tier;", "cost", "tier"))
    #specs.append(("unit", "Unit", std_members + "u32 cost; u16 type; u16 attack; u16 defense; u16 mvt_pts;", "cost", "type"))
    #specs.append(("unit_type", "UnitType", "std::string name;", "None", "None"))
    #specs.append(("wonder", "Wonder", std_members + "u32 cost; ItemEffectsStruct effects;", "cost", "effects.indices[0]"))

    total_failures = 0

    for output_prefix, class_name, struct_members, struct_member1, struct_member2 in specs:
        print("\n=======================================================")
        print(" Generating:", class_name, " (%s)" % output_prefix)
        print("=======================================================")

        gen_cmd = ["python3", "generate.py", output_prefix, class_name, struct_members, struct_member1, struct_member2]
        subprocess.run(gen_cmd, check=True)
        if "None" in [struct_member1, struct_member2]:
            continue

        comp_script = "./" + output_prefix + "_static_data_comp"
        subprocess.run([comp_script], check=True)

        tester_bin = "./" + output_prefix + "_static_data_tester"
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
