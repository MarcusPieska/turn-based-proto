#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def parse_buildings_file(filename):
    buildings = set()
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 1:
                building_name = parts[0].strip()
                if building_name:
                    buildings.add(building_name)
    return buildings

def parse_civ_traits_file(filename):
    trait_buildings = []
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 2:
                trait_name = parts[0].strip()
                if trait_name:
                    for i in range(1, len(parts)):
                        building_name = parts[i].strip()
                        if building_name:
                            trait_buildings.append((trait_name, building_name))
    return trait_buildings

def validate_trait_buildings(traits_file, buildings_file):
    print("Validating %s against %s" % (traits_file, buildings_file))
    valid_buildings = parse_buildings_file(buildings_file)
    trait_buildings = parse_civ_traits_file(traits_file)
    
    missing_buildings = []
    
    for trait_name, building_name in trait_buildings:
        if building_name not in valid_buildings:
            missing_buildings.append((trait_name, building_name))
    
    errors = False
    
    if missing_buildings:
        errors = True
        print("ERROR: Buildings assigned to traits but not defined in buildings file:")
        for trait, building in missing_buildings:
            print("  - Trait '" + trait + "' has building '" + building + "'")
        print()
    
    if not errors:
        print("SUCCESS: All buildings assigned to traits are defined in buildings file.")
        print("  - Validated " + str(len(trait_buildings)) + " trait-building assignments")
        print("  - All buildings found in buildings file (" + str(len(valid_buildings)) + " total buildings)")
    else:
        building_count = len(missing_buildings)
        print("FAILED: Found " + str(building_count) + " missing building" + ("s" if building_count != 1 else "") + ".")
        sys.exit(1)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    validate_trait_buildings("game_config.civ_traits", "game_config.buildings")
    
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
