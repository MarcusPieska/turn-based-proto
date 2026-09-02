#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os
import subprocess

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def resolve_tester_comp (rel_tester, root_dir="."):
    dirname, filename = os.path.split(rel_tester)
    if filename == "static_parsing_manager_opt_tester":
        comp = os.path.join(dirname, "static_parsing_manager_comp") if dirname else "static_parsing_manager_comp"
        if os.path.exists(os.path.join(root_dir, comp)):
            run_dir = os.path.dirname(comp) or "."
            return comp, run_dir, os.path.join(run_dir, filename)
        return None
    comp_name = filename.replace("_tester", "_comp")
    candidates = []
    if dirname:
        candidates.append(os.path.join(dirname, comp_name))
        candidates.append(os.path.join(dirname, "test_drivers", comp_name))
    else:
        candidates.append(comp_name)
        candidates.append(os.path.join("test_drivers", comp_name))
    for comp in candidates:
        if os.path.exists(os.path.join(root_dir, comp)):
            run_dir = os.path.dirname(comp) or "."
            return comp, run_dir, os.path.join(run_dir, filename)
    return None

def get_all_tester_files (root_dir="."):
    suffix = "_tester"
    seen_comps = set()
    tester_files, comp_files = [], []
    for root, dirs, files in os.walk(root_dir):
        for filename in files:
            if not filename.endswith(suffix):
                continue
            full_path = os.path.join(root, filename)
            rel_path = os.path.relpath(full_path, root_dir)
            resolved = resolve_tester_comp(rel_path, root_dir)
            if resolved is None:
                continue
            comp_path, run_dir, tester_run = resolved
            if comp_path in seen_comps:
                continue
            seen_comps.add(comp_path)
            comp_files.append(comp_path)
            tester_files.append(tester_run)
    tester_files.sort()
    comp_files.sort()
    print("\n*** Found %d test driver file(s):" % len(tester_files))
    for i, filepath in enumerate(tester_files, 1):
        print("    %d. %s" % (i, filepath))
    print("\n")
    return tester_files, comp_files

def compile_test_driver (comp_file_path):
    original_cwd = os.getcwd()
    comp_dir = os.path.dirname(comp_file_path)
    comp_filename = os.path.basename(comp_file_path)
    try:
        if comp_dir:
            os.chdir(comp_dir)
        else:
            os.chdir(original_cwd)
        compile_result = subprocess.run(["bash", comp_filename], capture_output=True, text=True, timeout=300)
        return (compile_result.returncode == 0, compile_result.stdout, compile_result.stderr)
    except subprocess.TimeoutExpired:
        return (False, "", "Compilation script timed out")
    except Exception as e:
        return (False, "", "Error compiling test driver: %s" % str(e))
    finally:
        os.chdir(original_cwd)

def run_test_driver (tester_file_path):
    original_cwd = os.getcwd()
    test_dir = os.path.dirname(tester_file_path)
    test_filename = os.path.basename(tester_file_path)
    try:
        if test_dir:
            os.chdir(test_dir)
        else:
            os.chdir(original_cwd)
        exec_path = "./" + test_filename
        if not os.path.exists(exec_path):
            return (False, "", "Executable not found: %s" % exec_path)
        run_result = subprocess.run([exec_path, "0"], capture_output=True, text=True, timeout=300)
        return (run_result.returncode == 0, run_result.stdout, run_result.stderr)
    except subprocess.TimeoutExpired:
        return (False, "", "Test driver timed out")
    except Exception as e:
        return (False, "", "Error running test driver: %s" % str(e))
    finally:
        os.chdir(original_cwd)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    tester_files, comp_files = get_all_tester_files()
    do_compile = False
    if len(sys.argv) > 1:
        do_compile = sys.argv[1].lower().startswith("c")

    failed = 0
    for filepath, comp_filepath in zip(tester_files, comp_files):
        if do_compile:
            success, stdout, stderr = compile_test_driver(comp_filepath)
            if not success:
                failed += 1
                print("COMPILE FAIL:", comp_filepath)
                if stdout:
                    print(stdout)
                if stderr:
                    print(stderr, file=sys.stderr)
                continue
        success, stdout, stderr = run_test_driver(filepath)
        if not success:
            failed += 1
            print("RUN FAIL:", filepath)
        if stdout:
            print(stdout)
        if stderr:
            print(stderr, file=sys.stderr)
    if failed:
        print("\n*** TOTAL FAILURES: %d" % failed)
        sys.exit(1)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
