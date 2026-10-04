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
    ("WarMuster", "unsigned seat, unsigned enemy, unsigned sx, unsigned sy, unsigned unit_n, unsigned size_sum, unsigned city_n, unsigned turn", "war muster start seat=%u enemy=%u staging=(%u,%u) units=%u size=%u cities=%u turn=%u\n"),
    ("WarPeaceMock", "unsigned seat, unsigned x, unsigned y, unsigned turn", "war peace mock-fail seat=%u city=(%u,%u) turn=%u\n"),
    ("WarPeace", "unsigned seat, unsigned enemy, unsigned turn", "war peace seat=%u enemy=%u turn=%u\n"),
    ("WarTargets", "unsigned seat, unsigned enemy, unsigned tn, unsigned turn", "war targets seat=%u enemy=%u n=%u turn=%u\n"),
    ("WarArmySize", "unsigned seat, unsigned unit_n, unsigned size_sum, unsigned sx, unsigned sy, unsigned turn", "war army size seat=%u units=%u size=%u staging=(%u,%u) turn=%u\n"),
    ("WarCityCapture", "unsigned seat, unsigned from, unsigned x, unsigned y, unsigned turn", "war city capture seat=%u from=%u city=(%u,%u) turn=%u\n"),
    ("WarBeginFail", "unsigned seat_n, unsigned turn", "war begin fail seat_n=%u turn=%u\n"),
    ("WarSecBeginFail", "unsigned turn, unsigned reason", "war sec begin fail turn=%u reason=%u\n"),
    ("WarPickEnemyFail", "unsigned seat, unsigned turn, unsigned reason", "war pick enemy fail seat=%u turn=%u reason=%u\n"),
    ("WarEngageFail", "unsigned seat, unsigned enemy, unsigned turn, unsigned reason", "war engage fail seat=%u enemy=%u turn=%u reason=%u\n"),
    ("WarHandleCampFail", "unsigned seat, unsigned enemy, unsigned turn", "war handle camp fail seat=%u enemy=%u turn=%u\n"),
    ("WarStartCampFail", "unsigned seat, unsigned enemy, unsigned turn, unsigned reason", "war start camp fail seat=%u enemy=%u turn=%u reason=%u\n"),
    ("WarDeclareFail", "unsigned seat, unsigned enemy, unsigned turn, unsigned reason", "war declare fail seat=%u enemy=%u turn=%u reason=%u\n"),
    ("WarStagingFail", "unsigned seat, unsigned enemy, unsigned turn, unsigned reason", "war staging fail seat=%u enemy=%u turn=%u reason=%u\n"),
    ("WarOwnFreeCompFail", "unsigned seat, unsigned turn, unsigned reason", "war own free comp fail seat=%u turn=%u reason=%u\n"),
    ("WarMusterGradFail", "unsigned seat, unsigned sx, unsigned sy, unsigned turn", "war muster grad fail seat=%u staging=(%u,%u) turn=%u\n"),
    ("WarExposureFail", "unsigned seat, unsigned enemy, unsigned turn", "war exposure fail seat=%u enemy=%u turn=%u\n"),
    ("WarWalkMusterFail", "unsigned seat, unsigned turn", "war walk muster fail seat=%u turn=%u\n"),
    ("WarMusterCanStepFail", "unsigned seat, unsigned unit, unsigned reason, unsigned x, unsigned y, unsigned nx, unsigned ny, unsigned turn", "war muster can_step fail seat=%u unit=%u reason=%u at=(%u,%u) to=(%u,%u) turn=%u\n"),
    ("WarMusterPeekFail", "unsigned seat, unsigned unit, unsigned reason, unsigned x, unsigned y, unsigned turn", "war muster peek fail seat=%u unit=%u reason=%u at=(%u,%u) turn=%u\n"),
    ("WarTileEntryResolve", "unsigned seat, unsigned unit, unsigned x, unsigned y, unsigned reason, unsigned outcome, unsigned turn", "war tile entry resolve seat=%u unit=%u at=(%u,%u) reason=%u outcome=%u turn=%u\n"),
    ("WarFormArmyFail", "unsigned seat, unsigned enemy, unsigned turn", "war form army fail seat=%u enemy=%u turn=%u\n"),
    ("WarArmyUpgrade", "unsigned seat, unsigned civ, unsigned from_u, unsigned to_u, unsigned turn", "war army upgrade seat=%u civ=%u from=%u to=%u turn=%u\n"),
    ("WarSetTargetFail", "unsigned seat, unsigned enemy, unsigned turn, unsigned reason", "war set target fail seat=%u enemy=%u turn=%u reason=%u\n"),
    ("WarRefillTargetsFail", "unsigned seat, unsigned enemy, unsigned turn, unsigned reason", "war refill targets fail seat=%u enemy=%u turn=%u reason=%u\n"),
    ("WarFindEnemySeedFail", "unsigned seat, unsigned enemy, unsigned turn", "war find enemy seed fail seat=%u enemy=%u turn=%u\n"),
    ("WarSetGoalFail", "unsigned seat, unsigned tx, unsigned ty, unsigned turn", "war set goal fail seat=%u goal=(%u,%u) turn=%u\n"),
    ("WarAssaultStallStop", "unsigned seat, unsigned enemy, unsigned tx, unsigned ty, unsigned turn", "war assault stall stop seat=%u enemy=%u city=(%u,%u) turn=%u\n"),
    ("WarAssaultFailStop", "unsigned seat, unsigned enemy, unsigned tx, unsigned ty, unsigned turn", "war assault fail stop seat=%u enemy=%u city=(%u,%u) turn=%u\n"),
    ("WarAssaultCityFail", "unsigned seat, unsigned tx, unsigned ty, unsigned army_i, unsigned turn", "war assault city fail seat=%u city=(%u,%u) army=%u turn=%u\n"),
    ("WarCamAssaultFail", "unsigned seat, unsigned tx, unsigned ty, unsigned turn, unsigned reason", "war cam assault fail seat=%u city=(%u,%u) turn=%u reason=%u\n"),
    ("WarCamMeleeFail", "unsigned seat, unsigned tx, unsigned ty, unsigned turn, unsigned reason", "war cam melee fail seat=%u city=(%u,%u) turn=%u reason=%u\n"),
    ("WarClaimCityMiss", "unsigned seat, unsigned x, unsigned y, unsigned turn", "war claim city miss seat=%u tile=(%u,%u) turn=%u\n"),
    ("WarRejoinMoveFail", "unsigned seat, unsigned enemy, unsigned army_i, unsigned turn, unsigned reason, unsigned sx, unsigned sy, unsigned dx, unsigned dy", "war rejoin move fail seat=%u enemy=%u army=%u turn=%u reason=%u stay=(%u,%u) occ=(%u,%u)\n"),
    ("WarRejoinLinkFail", "unsigned seat, unsigned enemy, unsigned army_i, unsigned turn", "war rejoin link fail seat=%u enemy=%u army=%u turn=%u\n"),
    ("WarArmyCantFight", "unsigned seat, unsigned enemy, unsigned army_i, unsigned turn", "war army cant fight seat=%u enemy=%u army=%u turn=%u\n"),
    ("WarRetargetFail", "unsigned seat, unsigned enemy, unsigned turn", "war retarget fail seat=%u enemy=%u turn=%u\n"),
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
    ("UnitState", "u16 owner, u16 unit, u16 x, u16 y", "unit state owner=%u unit=%u at=(%u,%u)\n"),
    ("ArmyInfo", "cstr tag", "army state=%s\n"),
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
    ("ArmyStateToWar", "GameState& state, u16 head"),
    ("ArmyStatePeace", "GameState& state, u16 head"),
    ("ArmyStateMusterGroup", "GameState& state, u16 sx, u16 sy, const u16* heads, u16 n"),
]

PROFS = [
    "GAME_LOOP",
    "TH_WAR",
]

#================================================================================================================================#
#=> - Paths -
#================================================================================================================================#

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.dirname(SCRIPT_DIR)
OUT_DIR = os.path.join(ROOT_DIR, "log_dbg")
GAME_EVAL_DIR = os.path.join(ROOT_DIR, "game_eval")
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

def fmt_scan_string (fmt: str) -> str:
    s = fmt
    if s.endswith("\n"):
        s = s[:-1]
    return s.replace("\\", "\\\\").replace('"', '\\"')

def c_string_lit (s: str) -> str:
    return s.replace("\\", "\\\\").replace('"', '\\"')

# ctype -> (scanf temp ctype, cast to assign into out ptr; "" if same)
SCAN_TEMP = {
    "u8": ("unsigned", "static_cast<u8>"),
    "u16": ("unsigned", "static_cast<u16>"),
    "u32": ("unsigned", "static_cast<u32>"),
    "unsigned": ("unsigned", ""),
    "i16": ("int", "static_cast<i16>"),
    "i32": ("int", "static_cast<i32>"),
    "int": ("int", ""),
}

def build_parse_bits (args: list[tuple[str, str]]) -> tuple[str, str]:
    if not args:
        params = ""
        body = (
            "    if (line == nullptr) {\n"
            "        return false;\n"
            "    }\n"
            "    return true;\n"
        )
        return params, body
    param_parts = []
    tmp_decls = []
    scan_addrs = []
    assigns = []
    null_checks = ["line == nullptr"]
    for i, (ty, name) in enumerate(args):
        if ty == "cstr":
            param_parts.append("char* %s" % name)
            param_parts.append("u32 %s_cap" % name)
            null_checks.append("%s == nullptr" % name)
            null_checks.append("%s_cap < 2u" % name)
            tmp = "t%u" % i
            tmp_decls.append("    char %s[128];" % tmp)
            tmp_decls.append("    %s[0] = 0;" % tmp)
            scan_addrs.append("%s" % tmp)
            assigns.append("    if (std::snprintf(%s, %s_cap, \"%%s\", %s) < 0) {" % (name, name, tmp))
            assigns.append("        return false;")
            assigns.append("    }")
            continue
        if ty not in SCAN_TEMP:
            raise ValueError("unsupported PARSE type %r for %r" % (ty, name))
        tmp_ty, cast = SCAN_TEMP[ty]
        param_parts.append("%s* %s" % (ty, name))
        null_checks.append("%s == nullptr" % name)
        tmp = "t%u" % i
        tmp_decls.append("    %s %s = 0;" % (tmp_ty, tmp))
        scan_addrs.append("&%s" % tmp)
        if cast:
            assigns.append("    *%s = %s(%s);" % (name, cast, tmp))
        else:
            assigns.append("    *%s = %s;" % (name, tmp))
    params = ", " + ", ".join(param_parts)
    n = len(args)
    body_lines = []
    body_lines.append("    if (%s) {" % " || ".join(null_checks))
    body_lines.append("        return false;")
    body_lines.append("    }")
    body_lines.extend(tmp_decls)
    scan_fmt = "[LOG_SCAN_FMT_TAG]"
    if any(ty == "cstr" for ty, _ in args):
        scan_fmt = scan_fmt  # replaced later; %s -> %127s applied in gen_one_log
    body_lines.append(
        "    if (std::sscanf(line, \"%s\", %s) != %d) {"
        % (scan_fmt, ", ".join(scan_addrs), n)
    )
    body_lines.append("        return false;")
    body_lines.append("    }")
    body_lines.extend(assigns)
    body_lines.append("    return true;")
    return params, "\n".join(body_lines) + "\n"

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
    prof_names: list[str],
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
    if prof_names:
        groups.append(emit_toggle_group(prof_names, prev))
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
    parse_params, parse_body = build_parse_bits(args)
    scan_fmt = fmt_scan_string(fmt)
    if any(ty == "cstr" for ty, _ in args):
        scan_fmt = scan_fmt.replace("%s", "%127s")
    parse_body = parse_body.replace("[LOG_SCAN_FMT_TAG]", scan_fmt)

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
        ("[LOG_PARSE_PARAMS_TAG]", parse_params),
        ("[LOG_PARSE_BODY_TAG]", parse_body),
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

def unpack_prof (entry: str) -> str:
    if not isinstance(entry, str) or not entry:
        raise ValueError("bad PROFS entry: %r" % (entry,))
    return entry

def gen_one_prof (entry: str) -> tuple[str, str]:
    suffix = unpack_prof(entry)
    sn = snake(suffix)
    su = suffix.upper()
    class_name = "PROF_%s" % su
    pairs = [
        ("[PROF_GUARD_TAG]", "PROF_%s_H" % su),
        ("[PROF_CLASS_TAG]", class_name),
        ("[PROF_ENABLE_TAG]", "ENABLED_PROF_%s" % su),
        ("[PROF_HEADER_TAG]", "prof_%s.h" % sn),
        ("[PROF_FILE_TAG]", "prof_%s.txt" % sn),
        ("[PROF_SETUP_MACRO_TAG]", "PROF_%s_SETUP" % su),
        ("[PROF_ENTER_MACRO_TAG]", "PROF_%s_ENTER" % su),
    ]
    h = apply_tags(read_template("TEMPLATE_prof.h"), pairs)
    cpp = apply_tags(read_template("TEMPLATE_prof.cpp"), pairs)
    write_text(os.path.join(OUT_DIR, "prof_%s.h" % sn), h)
    write_text(os.path.join(OUT_DIR, "prof_%s.cpp" % sn), cpp)
    return sn, "ENABLED_PROF_%s" % su

def gen_prof_setup (entries: list[str]) -> None:
    inc_lines = []
    call_lines = []
    for entry in entries:
        suffix = unpack_prof(entry)
        sn = snake(suffix)
        su = suffix.upper()
        inc_lines.append('#include "prof_%s.h"' % sn)
        call_lines.append("    PROF_%s_SETUP(dir);" % su)
    pairs = [
        ("[PROF_SETUP_INCLUDES_TAG]", "\n".join(inc_lines) + "\n"),
        ("[PROF_SETUP_CALLS_TAG]", "\n".join(call_lines) + "\n"),
    ]
    h = read_template("TEMPLATE_prof_setup.h")
    cpp = apply_tags(read_template("TEMPLATE_prof_setup.cpp"), pairs)
    write_text(os.path.join(OUT_DIR, "prof_dbg_setup.h"), h)
    write_text(os.path.join(OUT_DIR, "prof_dbg_setup.cpp"), cpp)

def log_field_name (suffix: str) -> str:
    return "m_" + snake(suffix)

def gen_log_need_mask (entries: list[tuple]) -> None:
    fields = [log_field_name(unpack_entry(e)[0]) for e in entries]
    enables = ["ENABLED_LOG_%s" % snake(unpack_entry(e)[0]).upper() for e in entries]
    n = len(fields)
    lines = []
    lines.append("//" + ("=" * 128))
    lines.append("//=> - WARNING -")
    lines.append("//" + ("=" * 128))
    lines.append("//")
    lines.append("//  - This file is AUTO-GENERATED by gen_log_dbg.py")
    lines.append("//  - Do not edit this file manually.")
    lines.append("//")
    lines.append("//" + ("=" * 128))
    lines.append("//=> - Include guards -")
    lines.append("//" + ("=" * 128))
    lines.append("")
    lines.append("#ifndef LOG_NEED_MASK_H")
    lines.append("#define LOG_NEED_MASK_H")
    lines.append("")
    lines.append('#include "game_primitives.h"')
    lines.append('#include "log_dbg_toggles.h"')
    lines.append("")
    lines.append("//" + ("=" * 128))
    lines.append("//=> - LogNeedMask -")
    lines.append("//" + ("=" * 128))
    lines.append("//")
    lines.append("//  One u8 per log channel (0 = not required). Drivers set desired channels to 1.")
    lines.append("//  EvalDriver::chk verifies ENABLED_LOG_* (via on()), not occurrence counts.")
    lines.append("//")
    lines.append("//" + ("=" * 128))
    lines.append("")
    lines.append("static const u16 LOG_NEED_N = %du;" % n)
    lines.append("")
    lines.append("struct LogNeedMask {")
    for f in fields:
        lines.append("    u8 %s;" % f)
    lines.append("")
    lines.append("    LogNeedMask ();")
    lines.append("    bool any () const;")
    lines.append("    u8* bytes ();")
    lines.append("    const u8* bytes () const;")
    lines.append("    static u16 n ();")
    lines.append("    static cstr nm (u16 i);")
    lines.append("    static bool on (u16 i);")
    lines.append("};")
    lines.append("")
    lines.append("inline LogNeedMask::LogNeedMask ()")
    init = ",\n      ".join("%s(0)" % f for f in fields)
    lines.append("    : %s {" % init)
    lines.append("}")
    lines.append("")
    lines.append("inline bool LogNeedMask::any () const {")
    lines.append("    const u8* b = bytes();")
    lines.append("    for (u16 i = 0; i < LOG_NEED_N; ++i) {")
    lines.append("        if (b[i] != 0) {")
    lines.append("            return true;")
    lines.append("        }")
    lines.append("    }")
    lines.append("    return false;")
    lines.append("}")
    lines.append("")
    lines.append("inline u8* LogNeedMask::bytes () {")
    lines.append("    return &%s;" % fields[0])
    lines.append("}")
    lines.append("")
    lines.append("inline const u8* LogNeedMask::bytes () const {")
    lines.append("    return &%s;" % fields[0])
    lines.append("}")
    lines.append("")
    lines.append("inline u16 LogNeedMask::n () {")
    lines.append("    return LOG_NEED_N;")
    lines.append("}")
    lines.append("")
    lines.append("inline cstr LogNeedMask::nm (u16 i) {")
    lines.append("    static const cstr k_nm[LOG_NEED_N] = {")
    for e in entries:
        suffix = unpack_entry(e)[0]
        lines.append('        "%s",' % suffix)
    lines.append("    };")
    lines.append("    if (i >= LOG_NEED_N) {")
    lines.append('        return "";')
    lines.append("    }")
    lines.append("    return k_nm[i];")
    lines.append("}")
    lines.append("")
    lines.append("inline bool LogNeedMask::on (u16 i) {")
    lines.append("    static const u8 k_on[LOG_NEED_N] = {")
    for en in enables:
        lines.append("#if defined(LOG_DBG_SO_BUILD) || defined(LOG_DBG_FORCE_ALL) || (defined(LOG_DBG_ENABLE) && defined(%s))" % en)
        lines.append("        1u,")
        lines.append("#else")
        lines.append("        0u,")
        lines.append("#endif")
    lines.append("    };")
    lines.append("    if (i >= LOG_NEED_N) {")
    lines.append("        return false;")
    lines.append("    }")
    lines.append("    return k_on[i] != 0u;")
    lines.append("}")
    lines.append("")
    lines.append("#endif // LOG_NEED_MASK_H")
    lines.append("")
    lines.append("//" + ("=" * 128))
    lines.append("//=> - End of file -")
    lines.append("//" + ("=" * 128))
    write_text(os.path.join(OUT_DIR, "log_need_mask.h"), "\n".join(lines) + "\n")

def gen_eval_log_count_inc (entries: list[tuple]) -> None:
    lines = []
    lines.append("// AUTO-GENERATED by gen_log_dbg.py — included by eval_log.cpp")
    for idx, e in enumerate(entries):
        suffix, signature, fmt, wrap = unpack_entry(e)
        sn = snake(suffix)
        args = parse_args(signature)
        class_name = "LOG_%s" % sn.upper()
        lines.append("    case %du: {" % idx)
        call_args = []
        for ty, name in args:
            if ty == "cstr":
                lines.append("        char %s[128];" % name)
                lines.append("        %s[0] = 0;" % name)
                call_args.append(name)
                call_args.append("128u")
            else:
                lines.append("        %s %s = 0;" % (ty, name))
                call_args.append("&%s" % name)
        if args:
            lines.append("        if (%s::PARSE(s, %s)) {" % (class_name, ", ".join(call_args)))
        else:
            lines.append("        if (%s::PARSE(s)) {" % class_name)
        lines.append("            cnt = cnt + 1u;")
        lines.append("        }")
        lines.append("        break;")
        lines.append("    }")
    os.makedirs(GAME_EVAL_DIR, exist_ok=True)
    write_text(os.path.join(GAME_EVAL_DIR, "eval_log_count.inc"), "\n".join(lines) + "\n")

def gen_eval_log_includes (entries: list[tuple]) -> None:
    lines = []
    lines.append("// AUTO-GENERATED by gen_log_dbg.py — included by eval_log.cpp")
    for e in entries:
        sn = snake(unpack_entry(e)[0])
        lines.append('#include "log_dbg/log_%s.h"' % sn)
    write_text(os.path.join(GAME_EVAL_DIR, "eval_log_includes.inc"), "\n".join(lines) + "\n")

def main () -> int:
    os.makedirs(OUT_DIR, exist_ok=True)
    prev_toggles = read_existing_toggle_states(TOGGLES_HEADER)
    log_toggle_names: list[str] = []
    assert_toggle_names: list[str] = []
    validate_toggle_names: list[str] = []
    prof_toggle_names: list[str] = []
    include_lines: list[str] = []
    so_objs: list[str] = []

    for entry in LOGS:
        sn, en = gen_one_log(entry)
        log_toggle_names.append(en)
        include_lines.append('#include "log_dbg/log_%s.h"' % sn)
        so_objs.append("log_%s" % sn)

    gen_log_need_mask(LOGS)
    gen_eval_log_includes(LOGS)
    gen_eval_log_count_inc(LOGS)

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

    for entry in PROFS:
        sn, en = gen_one_prof(entry)
        prof_toggle_names.append(en)
        include_lines.append('#include "log_dbg/prof_%s.h"' % sn)
        so_objs.append("prof_%s" % sn)

    gen_prof_setup(PROFS)
    include_lines.append('#include "log_dbg/prof_dbg_setup.h"')
    so_objs.append("prof_dbg_setup")

    toggles = apply_tags(
        read_template("TEMPLATE_log_dbg_toggles.h"),
        [("[LOG_TOGGLE_DEFINES_TAG]", build_toggle_defines(
            "LOG_DBG_ENABLE",
            log_toggle_names,
            assert_toggle_names,
            validate_toggle_names,
            prof_toggle_names,
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

    print("generated logs=%d asserts=%d validates=%d profs=%d" % (
        len(LOGS), len(ASSERTS), len(VALIDATES), len(PROFS)))
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
