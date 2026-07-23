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

def parse_res_dist_lines (filename):
    by_name = {}
    order = []
    with open(filename, "r") as ptr:
        for line in ptr:
            raw = line.rstrip("\n")
            stripped = raw.strip()
            if not stripped:
                continue
            name = stripped.split(":", 1)[0].strip()
            if not name:
                continue
            if name not in by_name:
                order.append(name)
            by_name[name] = stripped
    return by_name, order

def format_line (name, body, name_w):
    if body is None:
        return "%-*s" % (name_w, name)
    return "%-*s : %s" % (name_w, name, body)

def body_after_name (line):
    if ":" not in line:
        return None
    return line.split(":", 1)[1].strip()

def sort_res_dist (resources_file, res_dists_file):
    print("Sorting %s to match %s" % (res_dists_file, resources_file))
    resources = parse_name_order(resources_file)
    by_name, dist_order = parse_res_dist_lines(res_dists_file)
    inserted = []
    out_names = []
    for name in resources:
        out_names.append(name)
        if name not in by_name:
            by_name[name] = name
            inserted.append(name)
    extras = [name for name in dist_order if name not in set(resources)]
    out_names.extend(extras)
    name_w = max(len(name) for name in out_names)
    lines = []
    for name in out_names:
        body = body_after_name(by_name[name])
        lines.append(format_line(name, body, name_w))
    with open(res_dists_file, "w") as ptr:
        for line in lines:
            ptr.write(line + "\n")
        ptr.write("\n")
    if inserted:
        print("Inserted %u missing name(s) (name only):" % len(inserted))
        for name in inserted:
            print("  - " + name)
    if extras:
        print("Appended %u res_dist name(s) not in resources:" % len(extras))
        for name in extras:
            print("  - " + name)
    print("Wrote %u lines to %s" % (len(lines), res_dists_file))

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    sort_res_dist("game_config.resources", "game_config.res_dists")

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
