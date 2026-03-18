#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def parse_civ_traits_file(filename):
    traits = set()
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 1:
                trait_name = parts[0].strip()
                if trait_name:
                    traits.add(trait_name)
    return traits

def parse_civs_file(filename):
    civ_traits = []
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 2:
                civ_name = parts[0].strip()
                if civ_name:
                    for i in range(1, len(parts)):
                        trait_name = parts[i].strip()
                        if trait_name:
                            civ_traits.append((civ_name, trait_name))
    return civ_traits

def validate_civ_traits(civs_file, traits_file):
    print("Validating %s against %s" % (civs_file, traits_file))
    valid_traits = parse_civ_traits_file(traits_file)
    civ_traits = parse_civs_file(civs_file)
    
    missing_traits = []
    
    for civ_name, trait_name in civ_traits:
        if trait_name not in valid_traits:
            missing_traits.append((civ_name, trait_name))
    
    errors = False
    
    if missing_traits:
        errors = True
        print("ERROR: Traits assigned to civs but not defined in traits file:")
        for civ, trait in missing_traits:
            print("  - Civ '" + civ + "' has trait '" + trait + "'")
        print()
    
    if not errors:
        print("SUCCESS: All traits assigned to civs are defined in traits file.")
        print("  - Validated " + str(len(civ_traits)) + " civ-trait assignments")
        print("  - All traits found in traits file (" + str(len(valid_traits)) + " total traits)")
    else:
        trait_count = len(missing_traits)
        print("FAILED: Found " + str(trait_count) + " missing trait" + ("s" if trait_count != 1 else "") + ".")
        sys.exit(1)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    validate_civ_traits("game_config.civs", "game_config.civ_traits")
    
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
