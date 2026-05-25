#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os

from generate import ASSESSOR_SPECS, GAME_CONFIG_PATHS

#================================================================================================================================#
#=> - Constants -
#================================================================================================================================#

RESULTS_TO_MATCH_PREFIX = "RESULTS_TO_MATCH_"

#================================================================================================================================#
#=> - Tokenize -
#================================================================================================================================#

def is_numeric_token (tok):
    return len(tok) > 0 and tok.isdigit()

def is_land_type_token (tok):
    if '_' not in tok:
        return False
    for ch in tok:
        if not (ch.isupper() or ch == '_'):
            return False
    return True

def keep_cmp_token (tok):
    if is_numeric_token(tok):
        return False
    if is_land_type_token(tok):
        return False
    return True

def tokenize_line (line):
    content = line
    for ch in "():,":
        content = content.replace(ch, " ")
    content = content.replace(":", " ")
    tokens = []
    for part in content.split():
        if len(part) > 0 and keep_cmp_token(part):
            tokens.append(part)
    return tokens

def line_name (line):
    idx = line.find(":")
    if idx < 0:
        return line.strip()
    return line[:idx].strip()

def parse_named_lines (path):
    rows = {}
    with open(path, "r") as ptr:
        for line in ptr:
            line = line.rstrip("\n")
            if line.strip() == "":
                continue
            name = line_name(line)
            rows[name] = tokenize_line(line)
    return rows

#================================================================================================================================#
#=> - Compare -
#================================================================================================================================#

def compare_pair (inf_path, ref_path):
    inf = parse_named_lines(inf_path)
    ref = parse_named_lines(ref_path)
    only_inf_names = sorted(set(inf) - set(ref))
    only_ref_names = sorted(set(ref) - set(inf))
    line_mismatches = []
    for name in sorted(set(inf) & set(ref)):
        it = inf[name]
        rt = ref[name]
        if set(it) != set(rt):
            line_mismatches.append((name, it, rt))
    ok = (len(only_inf_names) == 0 and len(only_ref_names) == 0 and len(line_mismatches) == 0)
    return ok, only_inf_names, only_ref_names, line_mismatches, inf, ref

def print_line_tokens (label, tokens):
    print("    %s: %s" % (label, " ".join(tokens)))

def print_line_diff (name, inf_tokens, ref_tokens):
    print("  %s:" % name)
    print_line_tokens("inf", inf_tokens)
    print_line_tokens("ref", ref_tokens)
    only_inf = sorted(set(inf_tokens) - set(ref_tokens))
    only_ref = sorted(set(ref_tokens) - set(inf_tokens))
    if len(only_inf) > 0:
        print("    only in inf:", " ".join(only_inf))
    if len(only_ref) > 0:
        print("    only in ref:", " ".join(only_ref))

def print_res (out_prefix, inf_name, ref_path, plvl, ok, only_inf_names, only_ref_names, line_mismatches, inf, ref):
    if ok:
        if plvl >= 1:
            print("OK:", out_prefix, "tokens match")
        return
    print("WARN: token mismatch for", out_prefix)
    print("  inf:", inf_name)
    print("  ref:", ref_path)
    mismatch_by_name = {}
    for name, it, rt in line_mismatches:
        mismatch_by_name[name] = (it, rt)
    if plvl >= 1:
        all_names = sorted(set(inf) | set(ref))
        for name in all_names:
            if name in only_inf_names:
                print("  line only in inf:", name)
                print_line_tokens("tokens", inf[name])
            elif name in only_ref_names:
                print("  line only in ref:", name)
                print_line_tokens("tokens", ref[name])
            elif name in mismatch_by_name:
                it, rt = mismatch_by_name[name]
                print_line_diff(name, it, rt)
            else:
                print("  %s: OK" % name)
                print_line_tokens("inf", inf[name])
                print_line_tokens("ref", ref[name])
    else:
        for name in only_inf_names:
            print("  line only in inf:", name)
            print_line_tokens("tokens", inf[name])
        for name in only_ref_names:
            print("  line only in ref:", name)
            print_line_tokens("tokens", ref[name])
        for name, it, rt in line_mismatches:
            print_line_diff(name, it, rt)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))
    os.chdir(this_dir)

    plvl = 0
    if len(sys.argv) > 1:
        plvl = int(sys.argv[1])

    total_failures = 0

    for out_prefix, class_name, parsing_instructions in ASSESSOR_SPECS:
        macro = out_prefix.upper()
        inf_name = RESULTS_TO_MATCH_PREFIX + macro
        if out_prefix not in GAME_CONFIG_PATHS:
            print("WARN: no game_config path for", out_prefix)
            total_failures += 1
            continue
        ref_path = GAME_CONFIG_PATHS[out_prefix]
        if not os.path.isfile(inf_name):
            print("WARN: missing", inf_name)
            total_failures += 1
            continue
        if not os.path.isfile(ref_path):
            print("WARN: missing", ref_path)
            total_failures += 1
            continue
        ok, only_inf_names, only_ref_names, line_mismatches, inf, ref = compare_pair(inf_name, ref_path)
        if not ok:
            total_failures += 1
        print_res(out_prefix, inf_name, ref_path, plvl, ok, only_inf_names, only_ref_names, line_mismatches, inf, ref)

    print("")
    print("TOTAL FAILURES:", total_failures)
    if total_failures > 0:
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
