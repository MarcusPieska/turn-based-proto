#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def parse_techs_file(filename):
    techs = set()
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 1:
                tech_name = parts[0].strip()
                if tech_name:
                    techs.add(tech_name)
    return techs

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

def parse_wonders_file(filename):
    wonders = []
    with open(filename, 'r') as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(':')
            if len(parts) >= 3:
                wonder_name = parts[0].strip()
                cost_str = parts[1].strip()
                tech_prereq = parts[2].strip()
                if wonder_name and cost_str and tech_prereq:
                    resource_list = []
                    for i in range(3, len(parts)):
                        resource_name = parts[i].strip()
                        if resource_name:
                            resource_list.append(resource_name)
                    wonders.append((wonder_name, tech_prereq, resource_list))
    return wonders

def validate_wonders(wonders_file, techs_file, resources_file):
    print("Validating %s against %s and %s" % (wonders_file, techs_file, resources_file))
    techs = parse_techs_file(techs_file)
    resources = parse_resources_file(resources_file)
    wonders = parse_wonders_file(wonders_file)
    
    missing_techs = []
    missing_res = []
    
    for wonder_name, tech_prereq, resource_list in wonders:
        if tech_prereq not in techs:
            missing_techs.append((wonder_name, tech_prereq))
        for resource_name in resource_list:
            if resource_name not in resources:
                missing_res.append((wonder_name, resource_name))
    
    errors = False
    
    if missing_techs:
        errors = True
        print("ERROR: Tech prerequisites in wonders but not in techs file:")
        for wonder, tech in missing_techs:
            print("  - Wonder '" + wonder + "' requires tech '" + tech + "'")
        print()
    
    if missing_res:
        errors = True
        print("ERROR: Resources in wonders but not in resources file:")
        for wonder, resource in missing_res:
            print("  - Wonder '" + wonder + "' requires resource '" + resource + "'")
        print()
    
    if not errors:
        print("SUCCESS: All tech prerequisites and resources in wonders are defined in techs and resources files.")
        print("  - Validated " + str(len(wonders)) + " wonder entries")
        print("  - All tech prerequisites found in techs file (" + str(len(techs)) + " total techs)")
        print("  - All resources found in resources file (" + str(len(resources)) + " total resources)")
    else:
        tech_count = len(missing_techs)
        res_count = len(missing_res)
        print("FAILED: Found " + str(tech_count) + " missing tech" + ("s" if tech_count != 1 else "") + " and " + str(res_count) + " missing resource" + ("s" if res_count != 1 else "") + ".")
        sys.exit(1)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    validate_wonders("game_config.wonders", "game_config.techs", "game_config.resources")
    
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
