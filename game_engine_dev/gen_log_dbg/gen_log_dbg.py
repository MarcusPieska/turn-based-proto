#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

from __future__ import annotations

import os
import re
import subprocess
import sys

#================================================================================================================================#
#=> - Data -
#================================================================================================================================#

LOGS = [
    ("NewTurn", "u16 turn", "NEW_TURN:%u\n"),
    ("CityFoundation", "u16 x, u16 y, u16 player", "city founded at=(%u,%u) civ=%u\n"),
    ("CivSpawnPt", "u16 x, u16 y, u16 civ_idx", "civ spawned at=(%u,%u) civ=%u\n"),
    ("UnitSpawn", "u16 typ_idx, u16 civ_idx, u16 x, u16 y", "unit spawned typ=%u civ=%u at=(%u,%u)\n"),
    ("WarMuster", "unsigned seat, unsigned enemy, unsigned sx, unsigned sy, unsigned n, unsigned turn", "war muster start seat=%u enemy=%u staging=(%u,%u) muster_n=%u turn=%u\n"),
    ("WarPeaceMock", "unsigned seat, unsigned x, unsigned y, unsigned turn", "war peace mock-fail seat=%u city=(%u,%u) turn=%u\n"),
    ("WarArmySize", "unsigned seat, unsigned army_n, unsigned tile_n, unsigned sx, unsigned sy, unsigned turn", "war army size seat=%u army_units=%u tile_units=%u staging=(%u,%u) turn=%u\n"),
    ("WarCityCapture", "unsigned seat, unsigned from, unsigned x, unsigned y, unsigned turn", "war city capture seat=%u from=%u city=(%u,%u) turn=%u\n"),
    ("CityJobFood", "u16 jobs, u16 yield", "city job food jobs=%u yield=%u\n"),
    ("CityJobProduction", "u16 jobs, u16 yield", "city job production jobs=%u yield=%u\n"),
    ("CityJobCommerce", "u16 jobs, u16 yield", "city job commerce jobs=%u yield=%u\n"),
    ("CityJobCulture", "u16 jobs, u16 yield", "city job culture jobs=%u yield=%u\n"),
    ("CityJobScience", "u16 jobs, u16 yield", "city job science jobs=%u yield=%u\n"),
    ("CityJobReligion", "u16 jobs, u16 yield", "city job religion jobs=%u yield=%u\n"),
    ("CityJobYields", "u16 n, i32 food, i32 production, i32 commerce, i32 culture, i32 science, i32 religion", "city job yields n=%u food=%d production=%d commerce=%d culture=%d science=%d religion=%d\n"),
    ("PlayerCommerce", "u16 player, u32 amount", "player=%u commerce=%u\n"),
    ("PlayerCommerceRaw", "u16 player, u32 amount", "player=%u commerce_raw=%u\n"),
    ("PlayerScience", "u16 player, u32 amount", "player=%u science=%u\n"),
    ("PlayerResearchPerc", "u16 player, u16 perc", "player=%u research_perc=%u\n"),
    ("PlayerTechDiscover", "u16 player, u16 tech", "player=%u tech=%u\n"),
]

ASSERTS = [
    ("CombatReady", "CombatMng ready"),
    ("SectorSupportBound", "SectorSupport bound"),
    ("WarArmyCanFight", "War army can fight"),
    ("CityJobAreCovered", "City jobs are covered"),
]

VALIDATES = [
    ("CityTileWorkCount", "const GameArraySimple& map, u16 city_idx, u16 pop"),
    ("ArmyUnitSupport", "GameState& state"),
    ("NavyUnitSupport", "GameState& state"),
]

#================================================================================================================================#
#=> - Paths -
#================================================================================================================================#

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
OUT_DIR = os.path.join(ROOT_DIR, "log_dbg")
ROOT_HEADER = os.path.join(ROOT_DIR, "log_dbg.h")
ALL_ENABLED_HEADER = os.path.join(ROOT_DIR, "log_dbg_all_enabled.h")
TOGGLES_HEADER = os.path.join(ROOT_DIR, "log_dbg_toggles.h")
COMP_SCRIPT = os.path.join(OUT_DIR, "log_dbg_comp")

#================================================================================================================================#
#=> - Helper functions -
#================================================================================================================================#

def snake (suffix: str) -> str:
    s1 = re.sub(r"(.)([A-Z][a-z]+)", r"\1_\2", suffix)
    return re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", s1).lower()

def parse_args (signature: str) -> list[tuple[str, str]]:
    signature = signature.strip()
    if not signature:
        return []
    out: list[tuple[str, str]] = []
    for part in signature.split(","):
        part = part.strip()
        if not part:
            continue
        m = re.match(r"^(.*\S)\s+(\w+)$", part)
        if not m:
            raise ValueError("bad signature fragment: %r" % (part,))
        out.append((m.group(1).strip(), m.group(2)))
    return out

def unpack_entry (entry: tuple) -> tuple[str, str, str, str | None]:
    if len(entry) == 3:
        return entry[0], entry[1], entry[2], None
    if len(entry) == 4:
        return entry[0], entry[1], entry[2], entry[3]
    raise ValueError("bad LOGS entry: %r" % (entry,))

def unpack_assert (entry: tuple) -> tuple[str, str]:
    if len(entry) != 2:
        raise ValueError("bad ASSERTS entry: %r" % (entry,))
    return entry[0], entry[1]

def unpack_eval (entry: tuple) -> tuple[str, str]:
    if len(entry) != 2:
        raise ValueError("bad VALIDATES entry: %r" % (entry,))
    return entry[0], entry[1]

def read_template (name: str) -> str:
    path = os.path.join(SCRIPT_DIR, name)
    with open(path, "r", encoding="utf-8") as f:
        return f.read()

def write_text (path: str, text: str) -> None:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote %s" % path)

def write_text_if_absent (path: str, text: str) -> bool:
    if os.path.isfile(path):
        print("kept %s" % path)
        return False
    write_text(path, text)
    return True

def apply_tags (text: str, pairs: list[tuple[str, str]]) -> str:
    for tag, value in pairs:
        text = text.replace(tag, value)
    return text

def fmt_c_string (fmt: str) -> str:
    out = fmt.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    if not fmt.endswith("\n") and not out.endswith("\\n"):
        out += "\\n"
    return out

def c_string_lit (s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')

TOGGLE_LIST_MARK = "LOG_DBG_TOGGLE_LIST"
TOGGLE_LINE_RE = re.compile(r"^\s*(?P<off>//\s*)?#define\s+(?P<name>[A-Za-z_][A-Za-z0-9_]*)\s*$")

def read_existing_toggle_states (path: str) -> dict[str, bool]:
    if not os.path.isfile(path):
        return {}
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()
    start = -1
    for i, line in enumerate(lines):
        if TOGGLE_LIST_MARK in line:
            start = i + 1
            break
    if start < 0:
        return {}
    states: dict[str, bool] = {}
    for line in lines[start:]:
        stripped = line.strip()
        if stripped.startswith("#endif"):
            break
        if stripped == "":
            continue
        m = TOGGLE_LINE_RE.match(line.rstrip("\n"))
        if not m:
            continue
        states[m.group("name")] = m.group("off") is None
    return states

def format_toggle_line (name: str, enabled: bool) -> str:
    if enabled:
        return "#define %s" % name
    return "//#define %s" % name

def emit_toggle_group (names: list[str], prev: dict[str, bool]) -> list[str]:
    lines = []
    for name in names:
        enabled = prev[name] if name in prev else True
        lines.append(format_toggle_line(name, enabled))
    return lines

def build_toggle_defines (
    master_name: str,
    log_names: list[str],
    assert_names: list[str],
    validate_names: list[str],
    prev: dict[str, bool],
) -> str:
    groups: list[list[str]] = []
    groups.append(emit_toggle_group([master_name], prev))
    if log_names:
        groups.append(emit_toggle_group(log_names, prev))
    if assert_names:
        groups.append(emit_toggle_group(assert_names, prev))
    if validate_names:
        groups.append(emit_toggle_group(validate_names, prev))
    body = "\n\n".join("\n".join(g) for g in groups)
    return "\n" + body + "\n"

def gen_one_log (entry: tuple) -> tuple[str, str]:
    suffix, signature, fmt, wrap = unpack_entry(entry)
    sn = snake(suffix)
    args = parse_args(signature)
    names = ", ".join(n for _, n in args)
    printf_args = (", " + names) if names else ""
    void_lines = "\n".join("        (void)%s;" % n for _, n in args)
    if void_lines:
        void_lines += "\n"
    wrap_include = ""
    wrap_call = ""
    if wrap:
        wrap_out = wrap
        for _, n in args:
            wrap_out = wrap_out.replace("{" + n + "}", n)
        if "TRACE_" in wrap:
            wrap_include = '#include "runtime_trace_dbg.h"\n'
        wrap_call = "        %s;\n" % wrap_out
    class_name = "LOG_%s" % sn.upper()

    pairs = [
        ("[LOG_GUARD_TAG]", "LOG_%s_H" % sn.upper()),
        ("[LOG_CLASS_TAG]", class_name),
        ("[LOG_ENABLE_TAG]", "ENABLED_LOG_%s" % sn.upper()),
        ("[LOG_PARAMS_TAG]", signature),
        ("[LOG_HEADER_TAG]", "log_%s.h" % sn),
        ("[LOG_FMT_TAG]", fmt_c_string(fmt)),
        ("[LOG_PRINTF_ARGS_TAG]", printf_args),
        ("[LOG_WRAP_INCLUDE_TAG]", wrap_include),
        ("[LOG_WRAP_CALL_TAG]", wrap_call),
        ("[LOG_VOID_ARGS_TAG]", void_lines),
    ]
    h = apply_tags(read_template("TEMPLATE_log.h"), pairs)
    cpp = apply_tags(read_template("TEMPLATE_log.cpp"), pairs)
    write_text(os.path.join(OUT_DIR, "log_%s.h" % sn), h)
    write_text(os.path.join(OUT_DIR, "log_%s.cpp" % sn), cpp)
    return sn, "ENABLED_LOG_%s" % sn.upper()

def gen_one_assert (entry: tuple) -> tuple[str, str]:
    suffix, info = unpack_assert(entry)
    sn = snake(suffix)
    info_lit = c_string_lit(info)
    class_name = "ASSERT_%s" % sn.upper()
    pairs = [
        ("[ASSERT_GUARD_TAG]", "ASSERT_%s_H" % sn.upper()),
        ("[ASSERT_CLASS_TAG]", class_name),
        ("[ASSERT_ENABLE_TAG]", "ENABLED_ASSERT_%s" % sn.upper()),
        ("[ASSERT_HEADER_TAG]", "assert_%s.h" % sn),
        ("[ASSERT_INFO_TAG]", info_lit),
    ]
    h = apply_tags(read_template("TEMPLATE_assert.h"), pairs)
    cpp = apply_tags(read_template("TEMPLATE_assert.cpp"), pairs)
    write_text(os.path.join(OUT_DIR, "assert_%s.h" % sn), h)
    write_text(os.path.join(OUT_DIR, "assert_%s.cpp" % sn), cpp)
    return sn, "ENABLED_ASSERT_%s" % sn.upper()

def gen_one_eval (entry: tuple) -> tuple[str, str]:
    suffix, signature = unpack_eval(entry)
    sn = snake(suffix)
    args = parse_args(signature)
    void_lines = "\n".join("        (void)%s;" % n for _, n in args)
    if void_lines:
        void_lines += "\n"
    class_name = "EVAL_%s" % sn.upper()
    pairs = [
        ("[EVAL_GUARD_TAG]", "EVAL_%s_H" % sn.upper()),
        ("[EVAL_CLASS_TAG]", class_name),
        ("[EVAL_ENABLE_TAG]", "ENABLED_EVAL_%s" % sn.upper()),
        ("[EVAL_HEADER_TAG]", "eval_%s.h" % sn),
        ("[EVAL_PARAMS_TAG]", signature),
        ("[EVAL_VOID_ARGS_TAG]", void_lines),
    ]
    h_path = os.path.join(OUT_DIR, "eval_%s.h" % sn)
    cpp_path = os.path.join(OUT_DIR, "eval_%s.cpp" % sn)
    h = apply_tags(read_template("TEMPLATE_eval.h"), pairs)
    cpp = apply_tags(read_template("TEMPLATE_eval.cpp"), pairs)
    write_text_if_absent(h_path, h)
    write_text_if_absent(cpp_path, cpp)
    return sn, "ENABLED_EVAL_%s" % sn.upper()

def main () -> int:
    os.makedirs(OUT_DIR, exist_ok=True)
    prev_toggles = read_existing_toggle_states(TOGGLES_HEADER)
    log_toggle_names: list[str] = []
    assert_toggle_names: list[str] = []
    validate_toggle_names: list[str] = []
    include_lines: list[str] = []
    so_objs: list[str] = []

    for entry in LOGS:
        sn, en = gen_one_log(entry)
        log_toggle_names.append(en)
        include_lines.append('#include "log_dbg/log_%s.h"' % sn)
        so_objs.append("log_%s" % sn)

    for entry in ASSERTS:
        sn, en = gen_one_assert(entry)
        assert_toggle_names.append(en)
        include_lines.append('#include "log_dbg/assert_%s.h"' % sn)
        so_objs.append("assert_%s" % sn)

    for entry in VALIDATES:
        sn, en = gen_one_eval(entry)
        validate_toggle_names.append(en)
        include_lines.append('#include "log_dbg/eval_%s.h"' % sn)
        so_objs.append("eval_%s" % sn)

    toggles = apply_tags(
        read_template("TEMPLATE_log_dbg_toggles.h"),
        [("[LOG_TOGGLE_DEFINES_TAG]", build_toggle_defines(
            "LOG_DBG_ENABLE",
            log_toggle_names,
            assert_toggle_names,
            validate_toggle_names,
            prev_toggles,
        ))],
    )
    write_text(TOGGLES_HEADER, toggles)

    includes = "\n".join(include_lines) + "\n"
    root = apply_tags(
        read_template("TEMPLATE_log_dbg.h"),
        [("[LOG_INCLUDES_TAG]", includes)],
    )
    write_text(ROOT_HEADER, root)

    all_en = apply_tags(
        read_template("TEMPLATE_log_dbg_all_enabled.h"),
        [("[LOG_INCLUDES_TAG]", includes)],
    )
    write_text(ALL_ENABLED_HEADER, all_en)

    write_text(COMP_SCRIPT, build_comp_script(so_objs))
    os.chmod(COMP_SCRIPT, 0o755)

    print("generated logs=%d asserts=%d validates=%d" %(len(LOGS), len(ASSERTS), len(VALIDATES)))
    print("building %s ..." % COMP_SCRIPT, flush=True)
    rc = subprocess.call(["bash", COMP_SCRIPT])
    if rc != 0:
        print("log_dbg_comp failed with exit %d" % rc, file=sys.stderr)
        return rc
    return 0

def build_comp_script (so_objs: list[str]) -> str:
    compile_lines = []
    obj_lines = []
    rm_lines = []
    for stem in so_objs:
        compile_lines.append(
            "g++ $CXXFLAGS $INC -c %s.cpp -o %s.o" % (stem, stem)
        )
        obj_lines.append("    %s.o \\" % stem)
        rm_lines.append("%s.o" % stem)
    body = "\n".join(compile_lines)
    objs = "\n".join(obj_lines)
    rms = " ".join(rm_lines)
    return """#!/bin/bash
set -e

cd "$(dirname "$0")"

INC="-I.. -I../misc -I../map_loader -I../simple_map_gen -I../dyn_state -I../data_io -I../static_state -I../city -I../city/assessor -I../city/effector -I../gen_bit_banks -I../adv_map_gen -I../game -I../ai_pathing/walk_general -I../ai_pathing/walk_cities -I../ai_pathing/walk_target -I../ai_proc -I."
CXXFLAGS="-std=c++17 -Wall -Wextra -fPIC"
OUT_SO="log_dbg.so"

%s

g++ -shared -o $OUT_SO \\
%s
    -Wl,-soname,$OUT_SO

rm -f %s
echo "built $(pwd)/$OUT_SO"
""" % (body, objs, rms)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    sys.exit(main())

#================================================================================================================================#
#=> - End of file -
#================================================================================================================================#
