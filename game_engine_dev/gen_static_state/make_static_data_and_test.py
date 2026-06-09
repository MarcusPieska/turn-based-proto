#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import subprocess
import os
import shutil

#================================================================================================================================#
#=> - Helpers -
#================================================================================================================================#

def migrate_generated_files (this_dir, output_prefix):
    static_state_dir = os.path.normpath(os.path.join(this_dir, "..", "static_state"))
    static_variable_state_dir = os.path.normpath(os.path.join(this_dir, "..", "static_variable_state"))
    os.makedirs(static_state_dir, exist_ok=True)
    os.makedirs(static_variable_state_dir, exist_ok=True)
    static_names = []
    static_names.append(output_prefix + "_static_data.cpp")
    static_names.append(output_prefix + "_static_data.h")
    static_names.append(output_prefix + "_static_key.h")
    static_names.append(output_prefix + "_static_data_tester.cpp")
    static_names.append(output_prefix + "_static_data_comp")
    bit_names = []
    bit_names.append(output_prefix + "_bit_array.cpp")
    bit_names.append(output_prefix + "_bit_array.h")
    bit_names.append(output_prefix + "_bit_array_tester.cpp")
    bit_names.append(output_prefix + "_bit_array_comp")
    for name in static_names:
        src = os.path.join(this_dir, name)
        if os.path.isfile(src):
            dst = os.path.join(static_state_dir, name)
            shutil.move(src, dst)
            print("  moved:", name, "->", static_state_dir)
    for name in bit_names:
        src = os.path.join(this_dir, name)
        if os.path.isfile(src):
            dst = os.path.join(static_variable_state_dir, name)
            shutil.move(src, dst)
            print("  moved:", name, "->", static_variable_state_dir)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))
    std_members = "std::string name; ItemReqsStruct reqs; "
    specs = []
    
    specs.append(("building", "Building", std_members + "u32 cost; ItemEffectsStruct effects;", "cost", "effects.items[0].type"))
    specs.append(("city_flag", "CityFlag", std_members + "ItemEffectsStruct effects;", "effects.items[0].type", "effects.items[1].type"))
    specs.append(("civ", "Civ", "std::string name; CivTraitStruct traits;", "None", "None"))
    specs.append(("civ_trait", "CivTrait", "std::string name;", "None", "None"))
    specs.append(("resource", "Resource", std_members + "u16 food; u16 shields; u16 commerce;", "food", "shields"))
    
    specs.append(("small_wonder", "SmallWonder", std_members + "u32 cost; ItemEffectsStruct effects;", "cost", "effects.items[0].type"))
    specs.append(("tech", "Tech", std_members + "ItemEffectsStruct effects; u32 cost; u16 tier;", "cost", "tier"))
    specs.append(("unit", "Unit", std_members + "u32 cost; u16 type; u16 attack; u16 defense; u16 mvt_pts;", "cost", "type"))
    specs.append(("unit_action", "UnitAction", "std::string name;", "None", "None"))
    specs.append(("unit_type", "UnitType", "std::string name;", "None", "None"))
    
    specs.append(("wonder", "Wonder", std_members + "u32 cost; ItemEffectsStruct effects;", "cost", "effects.items[0].type"))

    total_failures = 0

    for output_prefix, class_name, struct_members, struct_member1, struct_member2 in specs:
        print("\n=======================================================")
        print(" Generating:", class_name, " (%s)" % output_prefix)
        print("=======================================================")

        gen_cmd = ["python3", "generate.py", output_prefix, class_name, struct_members, struct_member1, struct_member2]
        subprocess.run(gen_cmd, check=True, cwd=this_dir)
        if "None" in [struct_member1, struct_member2]:
            migrate_generated_files(this_dir, output_prefix)
            continue

        comp_script = "./" + output_prefix + "_static_data_comp"
        subprocess.run([comp_script], check=True, cwd=this_dir)

        tester_bin = "./" + output_prefix + "_static_data_tester"
        test_result = subprocess.run([tester_bin], check=False, cwd=this_dir)
        if test_result.returncode != 0:
            total_failures += 1
        else:
            migrate_generated_files(this_dir, output_prefix)
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
