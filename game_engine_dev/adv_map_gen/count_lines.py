#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import sys
sys.dont_write_bytecode = True

import os

#================================================================================================================================#
#=> - functions -
#================================================================================================================================#

def get_files_by_extension(root_dir="."):
    cpp_files = []
    h_files = []
    py_files = []
    
    for root, dirs, files in os.walk(root_dir):
        for filename in files:
            if filename.endswith(".cpp"):
                full_path = os.path.join(root, filename)
                cpp_files.append(full_path)
            elif filename.endswith(".h"):
                full_path = os.path.join(root, filename)
                h_files.append(full_path)
            elif filename.endswith(".py"):
                full_path = os.path.join(root, filename)
                py_files.append(full_path)
    
    return cpp_files, h_files, py_files

def count_lines_in_files(file_paths):
    total_lines = 0
    for file_path in file_paths:
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
                total_lines += len(lines)
        except Exception as e:
            pass
    return total_lines

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    cpp_files, h_files, py_files = get_files_by_extension()
    
    cpp_lines = count_lines_in_files(cpp_files)
    h_lines = count_lines_in_files(h_files)
    py_lines = count_lines_in_files(py_files)
    
    print("File type counts:")
    print("  .cpp files: %d files, %d lines" % (len(cpp_files), cpp_lines))
    print("  .h files:   %d files, %d lines" % (len(h_files), h_lines))
    print("  .py files:  %d files, %d lines" % (len(py_files), py_lines))
    print("  Total:      %d files, %d lines" % (len(cpp_files) + len(h_files) + len(py_files), 
                                                 cpp_lines + h_lines + py_lines))

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
