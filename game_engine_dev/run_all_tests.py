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

def get_all_tester_files(root_dir="."):
    suffix, comp_suffix = "_tester", "_comp"
    tester_files, comp_files = [], []
    for root, dirs, files in os.walk(root_dir):
        for filename in files:
            if filename.endswith(suffix):
                full_path = os.path.join(root, filename)
                rel_path = os.path.relpath(full_path, root_dir)
                tester_files.append(rel_path)
                comp_path = rel_path.replace(suffix, comp_suffix)
                comp_files.append(comp_path)
                assert os.path.exists(comp_path), "*** No compilation script: %s" % comp_path
    tester_files.sort()
    comp_files.sort()
    print("\n*** Found %d test driver file(s):" % len(tester_files))
    for i, filepath in enumerate(tester_files, 1):
        print("    %d. %s" % (i, filepath))
    print("\n")
    
    return tester_files, comp_files

def compile_test_driver(comp_file_path):
    original_cwd = os.getcwd()
    comp_dir = os.path.dirname(comp_file_path)
    comp_filename = os.path.basename(comp_file_path)
    
    try:
        if comp_dir:
            os.chdir(comp_dir)
        else:
            os.chdir(original_cwd)

        compile_result = subprocess.run(["bash", comp_filename], capture_output=True, text=True, timeout=60)
        return (compile_result.returncode == 0, compile_result.stdout, compile_result.stderr)
    except subprocess.TimeoutExpired:
        return (False, "", "Compilation script timed out")
    except Exception as e:
        return (False, "", "Error compiling test driver: %s" % str(e))
    finally:
        os.chdir(original_cwd)

def run_test_driver(tester_file_path):
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
        
        run_result = subprocess.run([exec_path, "0"], capture_output=True, text=True, timeout=60)
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
                print(stdout)
                print(stderr, file=sys.stderr)
                continue
        success, stdout, stderr = run_test_driver(filepath)
        if not success:
            failed += 1
        if stdout:
            print(stdout)
        if stderr:
            print(stderr, file=sys.stderr)
 
#================================================================================================================================#
#=> - End -
#================================================================================================================================#
