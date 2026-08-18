#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import re
import sys
sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

REQ_RE = re.compile(r"^([A-Za-z_]+)\((.*)\)$")

def parse_names (filename):
    names = set()
    with open(filename, "r") as ptr:
        for line in ptr:
            line = line.strip()
            if not line:
                continue
            name = line.split(":", 1)[0].strip()
            if name:
                names.add(name)
    return names

def parse_worker_job_imps (filename):
    rows = []
    with open(filename, "r") as ptr:
        for line_n, line in enumerate(ptr, 1):
            raw = line.strip()
            if not raw:
                continue
            parts = [p.strip() for p in raw.split(":")]
            if len(parts) < 4:
                rows.append((line_n, raw, None, None, None, "need name : mother : cost : req..."))
                continue
            name = parts[0]
            mother = parts[1]
            cost = parts[2]
            reqs = []
            bad_req = None
            for part in parts[3:]:
                if not part:
                    continue
                m = REQ_RE.match(part)
                if not m:
                    bad_req = part
                    break
                reqs.append((m.group(1).strip().lower(), m.group(2).strip()))
            if not name or not mother or not cost:
                rows.append((line_n, raw, None, None, None, "empty name/mother/cost"))
                continue
            if bad_req is not None:
                rows.append((line_n, name, mother, cost, None, "bad req token '%s'" % bad_req))
                continue
            rows.append((line_n, name, mother, cost, reqs, None))
    return rows

def validate_worker_job_imps (
    imps_file,
    worker_jobs_file,
    effects_file,
    techs_file,
    resources_file,
    buildings_file,
    toggle_city_file,
    civs_file):
    print("Validating %s against mother imps, effects, and req catalogs" % imps_file)
    mothers = parse_names(worker_jobs_file)
    effects = parse_names(effects_file)
    catalogs = {
        "tech": parse_names(techs_file),
        "resource": parse_names(resources_file),
        "building": parse_names(buildings_file),
        "flag": parse_names(toggle_city_file),
        "civ": parse_names(civs_file),
    }
    rows = parse_worker_job_imps(imps_file)

    bad_fmt = []
    missing_mother = []
    missing_effect = []
    missing_req = []
    unknown_req_kind = []
    ok_n = 0

    for line_n, name, mother, cost, reqs, err in rows:
        if err is not None:
            bad_fmt.append((line_n, name if name else "?", err))
            continue
        if mother not in mothers:
            missing_mother.append((name, mother))
        if name not in effects:
            missing_effect.append(name)
        for kind, item in reqs:
            if kind not in catalogs:
                unknown_req_kind.append((name, kind, item))
                continue
            if item not in catalogs[kind]:
                missing_req.append((name, kind, item))
        ok_n += 1

    errors = False

    if bad_fmt:
        errors = True
        print("ERROR: Malformed worker_job_imp rows:")
        for line_n, name, err in bad_fmt:
            print("  - line %u '%s': %s" % (line_n, name, err))
        print()

    if missing_mother:
        errors = True
        print("ERROR: Mother improvements not found in %s:" % worker_jobs_file)
        for name, mother in missing_mother:
            print("  - Imp '%s' has mother '%s'" % (name, mother))
        print()

    if missing_effect:
        errors = True
        print("ERROR: Worker job imps with no matching effect in %s:" % effects_file)
        for name in missing_effect:
            print("  - " + name)
        print()

    if unknown_req_kind:
        errors = True
        print("ERROR: Unknown req kinds (expected tech/resource/building/flag/civ):")
        for name, kind, item in unknown_req_kind:
            print("  - Imp '%s' has %s(%s)" % (name, kind, item))
        print()

    if missing_req:
        errors = True
        print("ERROR: Req items not found in their catalogs:")
        for name, kind, item in missing_req:
            print("  - Imp '%s' requires %s('%s')" % (name, kind, item))
        print()

    if not errors:
        print("SUCCESS: All worker job imps have effects, valid mothers, and valid reqs.")
        print("  - Validated %u worker_job_imp entries" % ok_n)
        print("  - Mother jobs checked against %u worker_jobs" % len(mothers))
        print("  - Effects checked against %u effect names" % len(effects))
        print("  - Reqs checked against tech/resource/building/flag/civ catalogs")
        return True

    print("FAILED: format=%u mother=%u effect=%u req_kind=%u req_item=%u" % (
        len(bad_fmt), len(missing_mother), len(missing_effect),
        len(unknown_req_kind), len(missing_req)))
    return False

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    ok = validate_worker_job_imps(
        "game_config.worker_job_imps",
        "game_config.worker_jobs",
        "game_config.effects",
        "game_config.techs",
        "game_config.resources",
        "game_config.buildings",
        "game_config.toggle_city",
        "game_config.civs")
    if not ok:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
