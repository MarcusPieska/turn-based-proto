#!/usr/bin/env python3

import os
import re
import sys

sys.dont_write_bytecode = True

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(THIS_DIR))

EFFECTS_PATH = os.path.join(ROOT, "game_config.effects")

ALL_SCOPES = ["LOCAL", "CITY", "CIV", "GLOBAL"]

GENERATED_SUFFIXES = [
    "_booster_register.h",
    "_booster_register.cpp",
    "_booster_register_tester.cpp",
    "_booster_register_comp",
]

#=================================================================================================================================
#=> - Scope helpers -
#=================================================================================================================================

SCOPE_ACTIVE_FN = {
    "LOCAL": "effect_enabler_active_local",
    "CITY": "effect_enabler_active_city",
    "CIV": "effect_enabler_active_civ",
    "GLOBAL": "effect_enabler_active_global",
}

#=================================================================================================================================
#=> - Name helpers -
#=================================================================================================================================

def to_pascal_token (s):
    return "".join(part.capitalize() for part in s.lower().split("_"))

def register_stem (booster_tp, scope):
    return "%s_%s" % (scope.lower(), booster_tp.lower())

def register_class (booster_tp, scope):
    return "%s%sBoosterRegister" % (to_pascal_token(scope), to_pascal_token(booster_tp))

def register_guard (booster_tp, scope):
    return "%s_BOOSTER_REGISTER_H" % register_stem(booster_tp, scope).upper()

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

#=================================================================================================================================
#=> - Game config parsing -
#=================================================================================================================================

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

def parse_booster_calls (path):
    specs = set()
    with open(path, "r") as ptr:
        for line in ptr:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            for m in re.finditer(r"booster\s*\(\s*(\w+)\s*,\s*(-?\d+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\)", line):
                booster_tp = m.group(1)
                scope = m.group(3)
                if booster_tp == "NONE":
                    continue
                specs.add((booster_tp, scope))
    return specs

def discover_booster_types ():
    types = set()
    for booster_tp, scope in parse_booster_calls(EFFECTS_PATH):
        types.add(booster_tp)
    return sorted(types)

def derive_register_specs ():
    specs = []
    for booster_tp in discover_booster_types():
        for scope in ALL_SCOPES:
            specs.append((booster_tp, scope))
    return specs

def parse_effects (path):
    rows = {}
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
            for m in re.finditer(r"booster\s*\(\s*(\w+)\s*,\s*(-?\d+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\)", body):
                booster_tp, amount, scope, mode = m.group(1), int(m.group(2)), m.group(3), m.group(4)
                rows.setdefault(nm, []).append((booster_tp, amount, scope, mode))
    return rows

def kind_for_catalog (catalog):
    if catalog == "building":
        return "EffectEnablerKind::BUILDING"
    if catalog == "small_wonder":
        return "EffectEnablerKind::SMALL_WONDER"
    if catalog == "tech":
        return "EffectEnablerKind::TECH"
    if catalog == "wonder":
        return "EffectEnablerKind::WONDER"
    return None

def find_entries (booster_tp, scope):
    effects = parse_effects(EFFECTS_PATH)
    catalogs = [
        ("building", load_stem_names(os.path.join(ROOT, "game_config.buildings"))),
        ("small_wonder", load_stem_names(os.path.join(ROOT, "game_config.small_wonders"))),
        ("tech", load_stem_names(os.path.join(ROOT, "game_config.techs"))),
        ("wonder", load_stem_names(os.path.join(ROOT, "game_config.wonders"))),
    ]
    entries = []
    for catalog, names in catalogs:
        kind = kind_for_catalog(catalog)
        if kind is None:
            continue
        name_to_idx = {nm: i for i, nm in enumerate(names)}
        for nm, fx_list in effects.items():
            if nm not in name_to_idx:
                continue
            for bt, amount, sc, mode in fx_list:
                if bt != booster_tp or sc != scope:
                    continue
                unit = amount if mode == "COUNT" else 0
                perc = amount if mode == "PERCENTAGE" else 0
                entries.append((kind, name_to_idx[nm], unit, perc, nm, catalog))
    entries.sort(key=lambda e: (e[0], e[1], -e[3], -e[2]))
    return entries

def emit_entry_row (kind, idx, unit, perc, nm, catalog):
    return "    { { %s, %u }, %d, %d }, // %s (%s)" % (kind, idx, unit, perc, nm, catalog)

def entry_rows_text (entries):
    lines = []
    for row in entries:
        lines.append(emit_entry_row(*row))
    return "\n".join(lines)

#=================================================================================================================================
#=> - Generation -
#=================================================================================================================================

def build_pairs (booster_tp, scope, entries):
    stem = register_stem(booster_tp, scope)
    cls = register_class(booster_tp, scope)
    hdr = "%s_booster_register.h" % stem
    cpp = "%s_booster_register.cpp" % stem
    tester_cpp = "%s_booster_register_tester.cpp" % stem
    tester_bin = "%s_booster_register_tester" % stem
    entry_n = len(entries)
    pairs = []
    pairs.append(("[REGISTER_CLASS]", cls))
    pairs.append(("[REGISTER_GUARD]", register_guard(booster_tp, scope)))
    pairs.append(("[REGISTER_HEADER]", hdr))
    pairs.append(("[REGISTER_CPP]", cpp))
    pairs.append(("[REGISTER_OBJ]", cpp.replace(".cpp", ".o")))
    pairs.append(("[REGISTER_TESTER_CPP]", tester_cpp))
    pairs.append(("[REGISTER_TESTER_OBJ]", tester_cpp.replace(".cpp", ".o")))
    pairs.append(("[REGISTER_TESTER_BIN]", tester_bin))
    pairs.append(("[SCOPE_ENUM]", scope))
    pairs.append(("[TYPE_ENUM]", booster_tp))
    pairs.append(("[SCOPE_LABEL]", scope))
    pairs.append(("[TYPE_LABEL]", booster_tp))
    pairs.append(("[ACTIVE_FN]", SCOPE_ACTIVE_FN[scope]))
    pairs.append(("[ENTRY_NUM]", str(entry_n)))
    pairs.append(("[ENTRY_ARR_N]", str(entry_n if entry_n > 0 else 1)))
    if entry_n == 0:
        pairs.append(("[ENTRY_ROWS]", "    // no effects for this scope/type"))
    else:
        pairs.append(("[ENTRY_ROWS]", entry_rows_text(entries)))
    return pairs, stem, entry_n

def generate_one (booster_tp, scope):
    if scope not in SCOPE_ACTIVE_FN:
        raise SystemExit("unknown scope: %s" % scope)
    entries = find_entries(booster_tp, scope)
    pairs, stem, entry_n = build_pairs(booster_tp, scope, entries)
    write_from_template("booster_register.h", "%s_booster_register.h" % stem, pairs)
    write_from_template("booster_register.cpp", "%s_booster_register.cpp" % stem, pairs)
    write_from_template("booster_register_tester.cpp", "%s_booster_register_tester.cpp" % stem, pairs)
    write_from_template("booster_register_comp", "%s_booster_register_comp" % stem, pairs)
    if entry_n == 0:
        print("wrote %s (0 entries)" % stem)
    else:
        print("wrote %s (%u entries)" % (stem, entry_n))
    return True

def cleanup_stale_generated (stems_written):
    stems_written = set(stems_written)
    for fn in os.listdir(THIS_DIR):
        if fn.startswith("TEMPLATE_"):
            continue
        for suf in GENERATED_SUFFIXES:
            if not fn.endswith(suf):
                continue
            stem = fn[:-len(suf)]
            if stem in stems_written:
                break
            path = os.path.join(THIS_DIR, fn)
            os.remove(path)
            print("removed stale %s" % fn)
            break

def write_all_comp (stems):
    lines = []
    lines.append("#!/bin/bash")
    lines.append("set -e")
    lines.append("")
    lines.append('cd "$(dirname "$0")"')
    lines.append("")
    for stem in stems:
        lines.append("./%s_booster_register_comp" % stem)
        lines.append("")
    out_path = os.path.join(THIS_DIR, "booster_register_all_comp")
    with open(out_path, "w") as ptr:
        ptr.write("\n".join(lines) + "\n")
    os.chmod(out_path, 0o755)
    print("wrote booster_register_all_comp (%u registers)" % len(stems))

def main ():
    specs = derive_register_specs()
    written = []
    for booster_tp, scope in specs:
        if generate_one(booster_tp, scope):
            written.append(register_stem(booster_tp, scope))
    cleanup_stale_generated(written)
    if written:
        write_all_comp(written)
    print("done: %u registers written (of %u specs)" % (len(written), len(specs)))

if __name__ == "__main__":
    main()

#=================================================================================================================================
#=> - End of file -
#=================================================================================================================================
