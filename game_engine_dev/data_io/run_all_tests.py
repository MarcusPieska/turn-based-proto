#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True

import subprocess

#================================================================================================================================#
#=> - Local scope -
#================================================================================================================================#

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
BOOTSTRAP_COMPS = ["runtime_static_loader_lib_comp"]

#================================================================================================================================#
#=> - Discovery -
#================================================================================================================================#

def list_local_comps (root_dir):
    comps = []
    for fn in sorted(os.listdir(root_dir)):
        if not fn.endswith("_comp"):
            continue
        if fn.startswith("TEMPLATE_"):
            continue
        comps.append(fn)
    return comps

def comp_to_testers (comp_name):
    if comp_name == "runtime_static_loader_lib_comp":
        return []
    if comp_name == "static_parsing_manager_comp":
        return ["static_parsing_manager_tester", "static_parsing_manager_opt_tester"]
    stem = comp_name[: -len("_comp")]
    return [stem + "_tester"]

def discover_specs (root_dir):
    specs = []
    for comp in list_local_comps(root_dir):
        if comp in BOOTSTRAP_COMPS:
            continue
        for tester in comp_to_testers(comp):
            specs.append((comp, tester))
    return specs

#================================================================================================================================#
#=> - Run helpers -
#================================================================================================================================#

def run_comp (comp_name):
    comp_path = os.path.join(SCRIPT_DIR, comp_name)
    try:
        r = subprocess.run(["bash", comp_path], cwd=SCRIPT_DIR, capture_output=True, text=True, timeout=600)
        return r.returncode == 0, r.stdout, r.stderr
    except subprocess.TimeoutExpired:
        return False, "", "Compilation script timed out: %s" % comp_name

def run_tester (tester_name):
    tester_path = os.path.join(SCRIPT_DIR, tester_name)
    if not os.path.isfile(tester_path):
        return False, "", "Executable not found: %s" % tester_name
    try:
        r = subprocess.run([tester_path, "0"], cwd=SCRIPT_DIR, capture_output=True, text=True, timeout=600)
        return r.returncode == 0, r.stdout, r.stderr
    except subprocess.TimeoutExpired:
        return False, "", "Test driver timed out: %s" % tester_name

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    do_compile = len(sys.argv) > 1 and sys.argv[1].lower().startswith("c")
    specs = discover_specs(SCRIPT_DIR)
    print("\n*** data_io: %d test driver(s):" % len(specs))
    for i, (comp, tester) in enumerate(specs, 1):
        print("    %d. %s (%s)" % (i, tester, comp))
    print("")
    failed = 0
    if do_compile:
        for comp in BOOTSTRAP_COMPS:
            comp_path = os.path.join(SCRIPT_DIR, comp)
            if not os.path.isfile(comp_path):
                continue
            ok, stdout, stderr = run_comp(comp)
            if not ok:
                failed += 1
                print("COMPILE FAIL:", comp)
                if stdout:
                    print(stdout)
                if stderr:
                    print(stderr, file=sys.stderr)
        built = set()
        for comp, tester in specs:
            if comp in built:
                continue
            built.add(comp)
            ok, stdout, stderr = run_comp(comp)
            if not ok:
                failed += 1
                print("COMPILE FAIL:", comp)
                if stdout:
                    print(stdout)
                if stderr:
                    print(stderr, file=sys.stderr)
    for comp, tester in specs:
        ok, stdout, stderr = run_tester(tester)
        if not ok:
            failed += 1
            print("RUN FAIL:", tester, "(%s)" % comp)
        if stdout:
            print(stdout)
        if stderr:
            print(stderr, file=sys.stderr)
    if failed:
        print("\n*** data_io TOTAL FAILURES: %d" % failed)
        sys.exit(1)
    print("\n*** data_io: all %d test driver(s) passed" % len(specs))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
