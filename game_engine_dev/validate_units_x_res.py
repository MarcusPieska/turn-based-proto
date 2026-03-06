#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def parse_units_file(filename):
    units = set()
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 1:
                unit_name = parts[0].strip()
                if unit_name:
                    units.add(unit_name)
    return units

def parse_resources_file(filename):
    resources = set()
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 1:
                resource_name = parts[0].strip()
                if resource_name:
                    resources.add(resource_name)
    return resources

def parse_unit_res_cost_file(filename):
    unit_resources = []
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 1:
                unit_name = parts[0].strip()
                if unit_name:
                    resources = []
                    for i in range(1, len(parts)):
                        resource_name = parts[i].strip()
                        if resource_name:
                            resources.append(resource_name)
                    unit_resources.append((unit_name, resources))
    return unit_resources

def validate_unit_res_cost(unit_res_cost_file, units_file, resources_file):
    print("Validating %s against %s and %s" % (unit_res_cost_file, units_file, resources_file))
    units = parse_units_file(units_file)
    resources = parse_resources_file(resources_file)
    unit_resources = parse_unit_res_cost_file(unit_res_cost_file)
    missing_units = []
    missing_res = []
    for unit_name, resource_list in unit_resources:
        if unit_name not in units:
            missing_units.append(unit_name)
        for resource_name in resource_list:
            if resource_name not in resources:
                missing_res.append((unit_name, resource_name))
    errors = False
    
    if missing_units:
        errors = True
        print("ERROR: Units in unit_res_cost but not in units file:")
        for unit in missing_units:
            print("  - " + unit)
        print()
    
    if missing_res:
        errors = True
        print("ERROR: Resources in unit_res_cost but not in resources file:")
        for unit, resource in missing_res:
            print("  - Unit '" + unit + "' requires '" + resource + "'")
        print()
    
    if not errors:
        print("SUCCESS: All units and resources in unit_res_cost are defined in units and resources files.")
        print("  - Validated " + str(len(unit_resources)) + " unit entries")
        print("  - All units found in units file (" + str(len(units)) + " total units)")
        print("  - All resources found in resources file (" + str(len(resources)) + " total resources)")
    else:
        print("FAILED: Found " + str(len(missing_units)) + " missing units and " + str(len(missing_res)) + " missing resources.")
        sys.exit(1)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    validate_unit_res_cost("game_config.unit_res_cost", "game_config.units", "game_config.resources")
    
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
