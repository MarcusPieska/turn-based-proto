#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def parse_roles_file (filename):
    roles = []
    with open(filename, "r") as ptr:
        for line in ptr:
            role = line.strip()
            if role:
                roles.append(role)
    return roles

def parse_units_with_roles (filename):
    units = []
    with open(filename, "r") as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = line.split(":")
            if len(parts) < 3:
                continue
            name = parts[0].strip()
            role = parts[2].strip()
            if name and role:
                units.append((name, role))
    return units

def validate_units_x_role (units_file, roles_file):
    print("Validating %s roles against %s" % (units_file, roles_file))
    roles = parse_roles_file(roles_file)
    role_set = set(roles)
    units = parse_units_with_roles(units_file)
    by_role = {}
    for role in roles:
        by_role[role] = []
    unknown = []
    for name, role in units:
        if role not in role_set:
            unknown.append((name, role))
            continue
        by_role[role].append(name)
    if unknown:
        print("ERROR: Units with role not in roles file:")
        for name, role in unknown:
            print("  - Unit '" + name + "' has role '" + role + "'")
        print()
        print("FAILED: Found %u unit(s) with unknown role." % len(unknown))
        sys.exit(1)
    print("SUCCESS: All %u units have a defined role (%u roles)." % (len(units), len(roles)))
    print()
    print("Role counts:")
    for role in roles:
        print("  %-14s %u" % (role, len(by_role[role])))
    print()
    print("Units by role:")
    for role in roles:
        print("  %s (%u):" % (role, len(by_role[role])))
        for name in by_role[role]:
            print("    - " + name)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    validate_units_x_role("game_config.units", "game_config.unit_roles")

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
