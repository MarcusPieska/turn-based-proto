#!/usr/bin/env python3
#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import argparse
import os
import subprocess
import sys

sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - Paths -
#================================================================================================================================#

HERE = os.path.dirname(os.path.abspath(__file__))
PROBE = os.path.join(HERE, "combat_mng_probe")
IN_TRACE = os.path.join(HERE, "combat_mng_in.trace")
OUT_TRACE = os.path.join(HERE, "combat_mng_out.trace")

#================================================================================================================================#
#=> - Colors -
#================================================================================================================================#

BLUE = "\033[94m"
RED = "\033[91m"
RESET = "\033[0m"

#================================================================================================================================#
#=> - Cases -
#================================================================================================================================#

CASES = []

def up(name, worse, better=""):
    CASES.append((name, worse, better))

# One line per case: worse scenario vs better; assert win(better) > win(worse).
up("plains_over_hills", "terr == hills", "terr == plains")
up("open_over_forest", "ov == forest", "ov == none")
up("full_hp_over_half", "atk_health == 50", "atk_health == 100")

up("green_over_very_green", "atk_level == VERY_GREEN", "atk_level == GREEN")
up("regular_over_green", "atk_level == GREEN", "atk_level == REGULAR")
up("disciplined_over_regular", "atk_level == REGULAR", "atk_level == DISCIPLINED")
up("hardened_over_disciplined", "atk_level == DISCIPLINED", "atk_level == HARDENED")
up("veteran_over_hardened", "atk_level == HARDENED", "atk_level == VETERAN")
up("commando_over_veteran", "atk_level == VETERAN", "atk_level == COMMANDO")
up("elite_over_commando", "atk_level == COMMANDO", "atk_level == ELITE")

up("no_walls_over_walls", "city == 1; bld == Walls", "city == 1")
up("no_great_wall_over_great_wall", "city == 1; wonder == Great Wall", "city == 1")

up("bowman_over_horseman_vs_spear", "", "atk_unit == Bowman")

#================================================================================================================================#
#=> - Probe -
#================================================================================================================================#

def norm_diff(text):
    lines = []
    for part in text.replace(";", "\n").splitlines():
        s = part.strip()
        if s:
            lines.append(s)
    return "\n".join(lines) + ("\n" if lines else "")

def run_probe(diff_text):
    with open(IN_TRACE, "w", encoding="utf-8") as f:
        f.write(norm_diff(diff_text))
    r = subprocess.run([PROBE, IN_TRACE, OUT_TRACE], cwd=HERE, capture_output=True, text=True)
    if r.returncode != 0 and not os.path.isfile(OUT_TRACE):
        return None, "probe exit %d" % r.returncode
    win = None
    samples = None
    err = None
    with open(OUT_TRACE, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line.startswith("win_prob =="):
                win = int(line.split("==", 1)[1].strip())
            elif line.startswith("samples =="):
                samples = int(line.split("==", 1)[1].strip())
            elif line.startswith("error =="):
                err = line.split("==", 1)[1].strip()
    if err is not None:
        return None, err
    if win is None:
        return None, "missing win_prob"
    if samples is None or samples <= 0:
        return None, "missing samples"
    return (win, samples), None

def pct(result):
    win, samples = result
    return 100.0 * float(win) / float(samples)

def fmt_pct(result, good):
    color = BLUE if good else RED
    return "%s%5.2f%%%s" % (color, pct(result), RESET)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

def main():
    ap = argparse.ArgumentParser(description="CombatMng relative win-rate suite")
    g = ap.add_mutually_exclusive_group()
    g.add_argument("-a", "--all", action="store_true", help="print all results")
    g.add_argument("-f", "--failures", action="store_true", help="print failures only (default)")
    args = ap.parse_args()
    show_all = bool(args.all)

    if not os.path.isfile(PROBE):
        sys.stderr.write("missing probe binary: %s (run ./combat_mng_probe_comp)\n" % PROBE)
        return 1

    passed = 0
    failed = 0
    for name, worse, better in CASES:
        w_win, w_err = run_probe(worse)
        b_win, b_err = run_probe(better)
        if w_err is not None or b_err is not None:
            failed += 1
            msg = w_err if w_err is not None else b_err
            print("%sFAIL%s  %-32s  probe error: %s" % (RED, RESET, name, msg))
            continue
        ok = b_win[0] > w_win[0]
        if ok:
            passed += 1
        else:
            failed += 1
        if show_all or not ok:
            tag = ("%sPASS%s" % (BLUE, RESET)) if ok else ("%sFAIL%s" % (RED, RESET))
            print("%s  %-32s  worse=%s  better=%s" % (tag, name, fmt_pct(w_win, ok), fmt_pct(b_win, ok)))

    total = passed + failed
    print("tally: %d/%d passed, %d failed" % (passed, total, failed))
    return 0 if failed == 0 else 1

if __name__ == "__main__":
    sys.exit(main()) 

#================================================================================================================================#
#=> - End of file -
#================================================================================================================================#
