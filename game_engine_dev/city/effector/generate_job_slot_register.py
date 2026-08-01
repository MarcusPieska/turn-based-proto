#!/usr/bin/env python3

import os
import re
import sys

sys.dont_write_bytecode = True

THIS_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(THIS_DIR))
EFFECTS_PATH = os.path.join(ROOT, "game_config.effects")
CITY_JOBS_PATH = os.path.join(ROOT, "game_config.city_jobs")

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

def scope_tok (nm):
    return "ItemEffectsScope::%s" % nm

def mode_tok (nm):
    return "ItemEffectAmountMode::%s" % nm

def parse_job_slots_effects (path):
    rows = []
    rx = re.compile(
        r"jobSlots\s*\(\s*([^,\)]+)\s*,\s*(-?\d+)\s*,\s*([^,\)]+)\s*,\s*([^,\)]+)\s*\)"
    )
    with open(path, "r") as ptr:
        for line in ptr:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split(":", 1)
            if len(parts) < 2:
                continue
            host_nm = parts[0].strip()
            body = parts[1]
            for m in rx.finditer(body):
                job_nm = m.group(1).strip()
                amount = int(m.group(2))
                scope_nm = m.group(3).strip()
                mode_nm = m.group(4).strip()
                rows.append((host_nm, job_nm, amount, scope_nm, mode_nm))
    return rows

def find_entries ():
    job_names = load_stem_names(CITY_JOBS_PATH)
    job_to_idx = {nm: i for i, nm in enumerate(job_names)}
    catalogs = [
        ("building", load_stem_names(os.path.join(ROOT, "game_config.buildings"))),
        ("tech", load_stem_names(os.path.join(ROOT, "game_config.techs"))),
        ("small_wonder", load_stem_names(os.path.join(ROOT, "game_config.small_wonders"))),
        ("wonder", load_stem_names(os.path.join(ROOT, "game_config.wonders"))),
    ]
    name_maps = [(c, {nm: i for i, nm in enumerate(names)}) for c, names in catalogs]

    raw = parse_job_slots_effects(EFFECTS_PATH)
    missing = []
    by_job = [[] for _ in job_names]
    for host_nm, job_nm, amount, scope_nm, mode_nm in raw:
        if job_nm not in job_to_idx:
            missing.append("unknown city_job '%s' on '%s'" % (job_nm, host_nm))
            continue
        if scope_nm not in ("LOCAL", "CITY", "CIV", "GLOBAL"):
            missing.append("unknown jobSlots scope '%s' on '%s'" % (scope_nm, host_nm))
            continue
        if mode_nm not in ("COUNT", "PERCENTAGE"):
            missing.append("unknown jobSlots mode '%s' on '%s'" % (mode_nm, host_nm))
            continue
        found = False
        for catalog, name_to_idx in name_maps:
            if host_nm not in name_to_idx:
                continue
            en_kind = kind_for_catalog(catalog)
            src_idx = name_to_idx[host_nm]
            by_job[job_to_idx[job_nm]].append({
                "en_kind": en_kind,
                "src_idx": src_idx,
                "host_nm": host_nm,
                "catalog": catalog,
                "job_id": job_to_idx[job_nm],
                "job_nm": job_nm,
                "amount": amount,
                "scope": scope_nm,
                "mode": mode_nm,
            })
            found = True
            break
        if not found:
            missing.append("unknown host '%s' jobSlots(%s)" % (host_nm, job_nm))
    if missing:
        for m in missing:
            print("ERROR: %s" % m)
        raise SystemExit(1)

    entries = []
    offs = [0]
    for job_id, rows in enumerate(by_job):
        entries.extend(rows)
        offs.append(len(entries))
    return job_names, entries, offs

def emit_entry (e):
    return "    { { %s, %u }, %u, %d, %s, %s }, // %s (%s) -> %s" % (
        e["en_kind"],
        e["src_idx"],
        e["job_id"],
        e["amount"],
        scope_tok(e["scope"]),
        mode_tok(e["mode"]),
        e["host_nm"],
        e["catalog"],
        e["job_nm"],
    )

def entry_rows_text (entries):
    if not entries:
        return "    // no jobSlots effects"
    return "\n".join(emit_entry(e) for e in entries)

def offs_text (offs):
    return "    " + ", ".join(str(v) for v in offs)

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main ():
    job_names, entries, offs = find_entries()
    job_n = len(job_names)
    entry_n = len(entries)
    pairs = []
    pairs.append(("[JOB_NUM]", str(job_n)))
    pairs.append(("[ENTRY_NUM]", str(entry_n)))
    pairs.append(("[ENTRY_ARR_N]", str(entry_n if entry_n > 0 else 1)))
    pairs.append(("[JOB_OFF_N]", str(job_n + 1)))
    pairs.append(("[ENTRY_ROWS]", entry_rows_text(entries)))
    pairs.append(("[JOB_OFFS]", offs_text(offs)))
    write_from_template("job_slot_register.h", "job_slot_register.h", pairs)
    write_from_template("job_slot_register.cpp", "job_slot_register.cpp", pairs)
    write_from_template("job_slot_register_tester.cpp", "job_slot_register_tester.cpp", pairs)
    write_from_template("job_slot_register_comp", "job_slot_register_comp", pairs)
    print("wrote job_slot_register (jobs=%u entries=%u)" % (job_n, entry_n))

if __name__ == "__main__":
    main()

#================================================================================================================================
#=> - End -
#================================================================================================================================
