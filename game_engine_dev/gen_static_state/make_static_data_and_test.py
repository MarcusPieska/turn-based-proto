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
    this_dir = os.path.dirname(os.path.abspath(__file__))
    static_state_dir = os.path.normpath(os.path.join(this_dir, "..", "static_state"))
    std_members = "ItemReqsStruct reqs; "
    specs = []
    
    specs.append(("building", "Building", std_members + "u32 cost; ItemEffectsStruct effects;", "cost", "effects.items[0].type"))
    specs.append(("city_flag", "CityFlag", std_members + "ItemEffectsStruct effects;", "effects.items[0].type", "effects.items[1].type"))
    specs.append(("civ", "Civ", "CivTraitStruct traits;", "traits.indices[0]", "traits.indices[1]"))
    specs.append(("civ_trait", "CivTrait", "", "None", "None"))
    specs.append(("tile_attribute", "TileAttribute", "u16 mvt_cost; i16 food; u16 production; u16 commerce; u16 culture; u16 science; u16 religion; u16 attack_mod; u16 defense_mod;", "None", "None"))
    specs.append(("resource", "Resource", std_members + "u16 food; u16 shields; u16 commerce; u16 type; u16 res_dist_idx;", "food", "type"))
    specs.append(("res_dist", "ResDist", "u8 has_plc; ResPlacement plc;", "None", "None"))
    specs.append(("res_type", "ResType", "", "None", "None"))
    
    specs.append(("small_wonder", "SmallWonder", std_members + "u32 cost; ItemEffectsStruct effects;", "cost", "effects.items[0].type"))
    specs.append(("tech", "Tech", std_members + "ItemEffectsStruct effects; u32 cost; u16 tier;", "cost", "tier"))
    specs.append(("unit", "Unit", std_members + "u32 cost; u16 type; u16 attack; u16 defense; u16 mvt_pts; u16 sight;", "cost", "type"))
    specs.append(("unit_action", "UnitAction", "", "None", "None"))
    specs.append(("unit_type", "UnitType", "", "None", "None"))
    
    specs.append(("wonder", "Wonder", std_members + "u32 cost; ItemEffectsStruct effects;", "cost", "effects.items[0].type"))
    specs.append(("worker_job", "WorkerJob", std_members + "u32 cost;", "cost", "reqs.types[0]"))

    total_failures = 0

    for output_prefix, class_name, struct_members, struct_member1, struct_member2 in specs:
        print("\n=======================================================")
        print(" Generating:", class_name, " (%s)" % output_prefix)
        print("=======================================================")

        gen_cmd = ["python3", "generate.py", output_prefix, class_name, struct_members, struct_member1, struct_member2]
        subprocess.run(gen_cmd, check=True, cwd=this_dir)
        if "None" in [struct_member1, struct_member2]:
            continue

        comp_script = os.path.join(static_state_dir, output_prefix + "_static_data_comp")
        subprocess.run([comp_script], check=True, cwd=static_state_dir)

        tester_bin = os.path.join(static_state_dir, output_prefix + "_static_data_tester")
        test_result = subprocess.run([tester_bin], check=False, cwd=static_state_dir)
        if test_result.returncode != 0:
            total_failures += 1
        if os.path.isfile(tester_bin):
            os.remove(tester_bin)

    print("\n\n=======================================================")
    print(" TOTAL FAILING TEST DRIVERS:", total_failures)
    print("=======================================================")

    if total_failures > 0:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
