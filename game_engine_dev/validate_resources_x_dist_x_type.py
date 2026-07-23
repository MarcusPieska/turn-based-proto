#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def parse_name_order (filename):
    names = []
    with open(filename, "r") as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            name = line.split(":", 1)[0].strip()
            if name:
                names.append(name)
    return names

def parse_resource_types (filename):
    pairs = []
    with open(filename, "r") as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            parts = [p.strip() for p in line.split(":")]
            name = parts[0] if len(parts) > 0 else ""
            typ = parts[4] if len(parts) > 4 else ""
            if name:
                pairs.append((name, typ))
    return pairs

def validate_res_dist (resources_file, res_dists_file):
    print("Validating %s against %s" % (res_dists_file, resources_file))
    resources = parse_name_order(resources_file)
    dists = set(parse_name_order(res_dists_file))
    missing = [name for name in resources if name not in dists]
    if missing:
        print("WARN: %u resource(s) missing from %s:" % (len(missing), res_dists_file))
        for name in missing:
            print("  - " + name)
        print()
        print("FAILED: %u of %u resources have no res_dist entry." % (len(missing), len(resources)))
        return False
    print("SUCCESS: Every resource has a res_dist entry.")
    print("  - Checked %u resources against %u res_dist names" % (len(resources), len(dists)))
    return True

def validate_res_types (resources_file, res_types_file):
    print("Validating %s types against %s" % (resources_file, res_types_file))
    known = set(parse_name_order(res_types_file))
    pairs = parse_resource_types(resources_file)
    bad = []
    for name, typ in pairs:
        if not typ:
            bad.append((name, "(missing)"))
        elif typ not in known:
            bad.append((name, typ))
    if bad:
        print("WARN: %u resource(s) with unknown or missing type:" % len(bad))
        for name, typ in bad:
            print("  - %s : %s" % (name, typ))
        print()
        print("FAILED: %u of %u resources have an invalid type." % (len(bad), len(pairs)))
        return False
    print("SUCCESS: Every resource type is listed in %s." % res_types_file)
    print("  - Checked %u resources against %u types" % (len(pairs), len(known)))
    return True

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    ok_dist = validate_res_dist("game_config.resources", "game_config.res_dists")
    print()
    ok_type = validate_res_types("game_config.resources", "game_config.res_types")
    if not ok_dist or not ok_type:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
