#!/usr/bin/env python3

import os
import re
import sys

sys.dont_write_bytecode = True

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(THIS_DIR))
EFFECTS_PATH = os.path.join(ROOT, "game_config.effects")

#================================================================================================================================
#=> - Yield catalog (locked; not modder-extensible) -
#================================================================================================================================

YIELD_TO_ID = {
    "FOOD": 1,
    "COMMERCE": 2,
    "PRODUCTION": 3,
    "SCIENCE": 4,
    "HAPPINESS": 5,
    "CULTURE": 6,
    "SANITATION": 7,
}
YIELD_N = 1 + max(YIELD_TO_ID.values())  # includes NONE at 0
MAX_EFFECT_COUNT = 5

#================================================================================================================================
#=> - Helpers -
#================================================================================================================================

def substitute (content, pairs):
    for old, new in pairs:
        content = content.replace(old, new)
    return content

def write_from_template (template_name, output_name, pairs):
    template_path = os.path.join(THIS_DIR, "TEMPLATE_" + template_name)
    out_path = os.path.join(THIS_DIR, output_name)
    with open(template_path, "r") as ptr:
        content = ptr.read()
    content = substitute(content, pairs)
    with open(out_path, "w") as ptr:
        ptr.write(content)
    if output_name.endswith("_comp"):
        os.chmod(out_path, 0o755)

def load_stem_names (path):
    names = []
    with open(path, "r") as ptr:
        for line in ptr:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            nm = line.split(":")[0].strip()
            if nm:
                names.append(nm)
    return names

def kind_for_catalog (catalog):
    if catalog == "building":
        return "EffectEnablerKind::BUILDING"
    if catalog == "tech":
        return "EffectEnablerKind::TECH"
    if catalog == "small_wonder":
        return "EffectEnablerKind::SMALL_WONDER"
    if catalog == "wonder":
        return "EffectEnablerKind::WONDER"
    return None

def parse_produce_effects (path):
    rows = []
    with open(path, "r") as ptr:
        for line in ptr:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split(":", 1)
            if len(parts) < 2:
                continue
            nm = parts[0].strip()
            body = parts[1]
            for m in re.finditer(r"produce\s*\(\s*([^,\)]+)\s*,\s*(-?\d+)\s*\)", body):
                target_nm = m.group(1).strip()
                amount = int(m.group(2))
                rows.append((nm, target_nm, amount))
    return rows

def resolve_target (target_nm, res_to_idx):
    if target_nm in YIELD_TO_ID:
        return ("ItemProduceKind::YIELD", YIELD_TO_ID[target_nm], "yield", target_nm)
    if target_nm in res_to_idx:
        return ("ItemProduceKind::RESOURCE", res_to_idx[target_nm], "resource", target_nm)
    return None

def find_groups ():
    produce_rows = parse_produce_effects(EFFECTS_PATH)
    catalogs = [
        ("building", load_stem_names(os.path.join(ROOT, "game_config.buildings"))),
        ("tech", load_stem_names(os.path.join(ROOT, "game_config.techs"))),
        ("small_wonder", load_stem_names(os.path.join(ROOT, "game_config.small_wonders"))),
        ("wonder", load_stem_names(os.path.join(ROOT, "game_config.wonders"))),
    ]
    resources = load_stem_names(os.path.join(ROOT, "game_config.resources"))
    res_to_idx = {nm: i for i, nm in enumerate(resources)}
    name_maps = []
    for catalog, names in catalogs:
        name_maps.append((catalog, {nm: i for i, nm in enumerate(names)}))

    groups = []
    group_key_to_idx = {}
    missing = []
    for host_nm, target_nm, amount in produce_rows:
        resolved = resolve_target(target_nm, res_to_idx)
        if resolved is None:
            missing.append("unknown produce target '%s' on '%s'" % (target_nm, host_nm))
            continue
        kind_tok, target_id, sink, sink_nm = resolved
        found = False
        for catalog, name_to_idx in name_maps:
            if host_nm not in name_to_idx:
                continue
            en_kind = kind_for_catalog(catalog)
            src_idx = name_to_idx[host_nm]
            key = (en_kind, src_idx)
            if key not in group_key_to_idx:
                group_key_to_idx[key] = len(groups)
                groups.append({
                    "en_kind": en_kind,
                    "src_idx": src_idx,
                    "host_nm": host_nm,
                    "catalog": catalog,
                    "slots": [],
                })
            g = groups[group_key_to_idx[key]]
            if len(g["slots"]) >= MAX_EFFECT_COUNT:
                missing.append("too many produce slots on '%s' (max %u)" % (host_nm, MAX_EFFECT_COUNT))
                found = True
                break
            g["slots"].append((kind_tok, target_id, amount, sink, sink_nm))
            found = True
            break
        if not found:
            missing.append("unknown host '%s' produce(%s)" % (host_nm, target_nm))
    if missing:
        for m in missing:
            print("ERROR: %s" % m)
        raise SystemExit(1)
    return groups, resources

def emit_slot (kind_tok, target_id, amount, sink, sink_nm):
    return "        { %s, %u, %d }, // %s %s" % (kind_tok, target_id, amount, sink, sink_nm)

def emit_group (g):
    slot_lines = [emit_slot(*sl) for sl in g["slots"]]
    slots_txt = "\n".join(slot_lines) if slot_lines else "        // empty"
    return (
        "    { { %s, %u }, %u, {\n%s\n    } }, // %s (%s)" % (
            g["en_kind"],
            g["src_idx"],
            len(g["slots"]),
            slots_txt,
            g["host_nm"],
            g["catalog"],
        )
    )

def group_rows_text (groups):
    if not groups:
        return "    // no produce effects"
    return "\n".join(emit_group(g) for g in groups)

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main ():
    groups, resources = find_groups()
    group_n = len(groups)
    inv_n = len(resources)
    pairs = []
    pairs.append(("[GROUP_NUM]", str(group_n)))
    pairs.append(("[GROUP_ARR_N]", str(group_n if group_n > 0 else 1)))
    pairs.append(("[INV_NUM]", str(inv_n if inv_n > 0 else 1)))
    pairs.append(("[YIELD_NUM]", str(YIELD_N)))
    pairs.append(("[GROUP_ROWS]", group_rows_text(groups)))
    write_from_template("produce_register.h", "produce_register.h", pairs)
    write_from_template("produce_register.cpp", "produce_register.cpp", pairs)
    write_from_template("produce_register_tester.cpp", "produce_register_tester.cpp", pairs)
    write_from_template("produce_register_comp", "produce_register_comp", pairs)
    print("wrote produce_register (%u groups, inv_n=%u, yield_n=%u)" % (group_n, inv_n, YIELD_N))

if __name__ == "__main__":
    main()

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
