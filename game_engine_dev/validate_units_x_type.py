#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def parse_unit_types_file(filename):
    unit_types = set()
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            unit_type_name = line.strip()
            if unit_type_name:
                unit_types.add(unit_type_name)
    return unit_types

def parse_units_file(filename):
    unit_type_assignments = []
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 2:
                unit_name = parts[0].strip()
                unit_type = parts[1].strip()
                if unit_name and unit_type:
                    unit_type_assignments.append((unit_name, unit_type))
    return unit_type_assignments

def validate_unit_types(units_file, types_file):
    print("Validating %s against %s" % (units_file, types_file))
    valid_unit_types = parse_unit_types_file(types_file)
    unit_type_assignments = parse_units_file(units_file)
    
    missing_types = []
    
    for unit_name, unit_type in unit_type_assignments:
        if unit_type not in valid_unit_types:
            missing_types.append((unit_name, unit_type))
    
    errors = False
    
    if missing_types:
        errors = True
        print("ERROR: Unit types assigned to units but not defined in unit types file:")
        for unit, unit_type in missing_types:
            print("  - Unit '" + unit + "' has type '" + unit_type + "'")
        print()
    
    if not errors:
        print("SUCCESS: All unit types assigned to units are defined in unit types file.")
        print("  - Validated " + str(len(unit_type_assignments)) + " unit-type assignments")
        print("  - All unit types found in unit types file (" + str(len(valid_unit_types)) + " total unit types)")
    else:
        type_count = len(missing_types)
        print("FAILED: Found " + str(type_count) + " missing unit type" + ("s" if type_count != 1 else "") + ".")
        sys.exit(1)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    validate_unit_types("game_config.units", "game_config.unit_types")
    
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
