#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import sys

sys.dont_write_bytecode = True

RESULTS_NEW_PREFIX = "RESULTS_NEW_"
RESULTS_VALID_PREFIX = "RESULTS_VALID_"

#================================================================================================================================#
#=> - Results -
#================================================================================================================================#

class Results:
    
    def __init__ (m, name, full_text, tokens, source_path):
        m.name = name
        m.full = full_text
        m.tokens = tokens
        m.source_path = source_path

    @staticmethod
    def parse_file (path):
        with open(path, "r") as ptr:
            content = ptr.read()
        items = []
        for part in content.split("name:"):
            chunk = part.strip()
            if len(chunk) == 0:
                continue
            full_text = "name:" + part
            if not full_text.endswith("\n"):
                full_text = full_text + "\n"
            tokens = full_text.split()
            first_line = full_text.split("\n", 1)[0]
            if not first_line.startswith("name:"):
                continue
            name = first_line[len("name:"):].strip()
            if len(name) == 0:
                continue
            items.append(Results(name, full_text, tokens, path))
        return items

class ResultsFile:
    
    def __init__ (m, tag, path, by_name):
        m.tag = tag
        m.path = path
        m.by_name = by_name

#================================================================================================================================#
#=> - Load helpers -
#================================================================================================================================#

def type_tag_fropath (path, prefix):
    base = os.path.basename(path)
    if not base.startswith(prefix):
        return base
    return base[len(prefix):]

def find_results_files (search_dir, prefix):
    paths = []
    for name in os.listdir(search_dir):
        if not name.startswith(prefix):
            continue
        full = os.path.join(search_dir, name)
        if os.path.isfile(full):
            paths.append(full)
    paths.sort()
    return paths

def load_results_files (search_dir, prefix):
    files = []
    for path in find_results_files(search_dir, prefix):
        tag = type_tag_fropath(path, prefix)
        by_name = {}
        for res in Results.parse_file(path):
            by_name[res.name] = res
        files.append(ResultsFile(tag, path, by_name))
    return files

def find_results_file_by_tag (files, tag):
    for res_file in files:
        if res_file.tag == tag:
            return res_file
    return None

def valid_path_for_tag (search_dir, tag):
    return os.path.join(search_dir, RESULTS_VALID_PREFIX + tag)

def load_results_file_for_tag (search_dir, prefix, tag):
    path = os.path.join(search_dir, prefix + tag)
    if not os.path.isfile(path):
        return None
    by_name = {}
    for res in Results.parse_file(path):
        by_name[res.name] = res
    return ResultsFile(tag, path, by_name)

#================================================================================================================================#
#=> - Validator -
#================================================================================================================================#

class Validator:

    def __init__ (m, search_dir):
        m.search_dir = search_dir

    def ask_yn (m, prompt):
        while True:
            ans = input(prompt).strip().lower()
            if ans == "y" or ans == "n":
                print("%s %s" % (prompt.rstrip(), ans))
                return ans == "y"
            print("Enter y or n")

    def print_compare (m, new_res, valid_res):
        print("========== VALID (baseline) ==========")
        if valid_res is None:
            print("(not present)")
        else:
            print(valid_res.full, end="")
        print("========== NEW (candidate) ===========")
        print(new_res.full, end="")
        print("-----------------------------------------------------------")

    def run_auto (m, new_file, valid_file):
        no_count = 0
        for name in sorted(new_file.by_name.keys()):
            new_res = new_file.by_name[name]
            valid_res = valid_file.by_name.get(name)
            if valid_res is not None and new_res.tokens == valid_res.tokens:
                continue
            m.print_compare(new_res, valid_res)
            if valid_res is None:
                prompt = "%s:%s missing in valid (y/n): " % (new_file.tag, name)
            else:
                prompt = "%s:%s token mismatch (y/n): " % (new_file.tag, name)
            if not m.ask_yn(prompt):
                no_count = no_count + 1
        return no_count

    def run_manual (m, new_file):
        no_count = 0
        for name in sorted(new_file.by_name.keys()):
            new_res = new_file.by_name[name]
            print(new_res.full)
            print("-----------------------------------------------------------")
            prompt = "%s:%s manual accept (y/n): " % (new_file.tag, name)
            if not m.ask_yn(prompt):
                no_count = no_count + 1
        return no_count

    def promote_new_to_valid (m, new_file):
        valid_path = valid_path_for_tag(m.search_dir, new_file.tag)
        with open(new_file.path, "r") as ptr:
            content = ptr.read()
        with open(valid_path, "w") as ptr:
            ptr.write(content)
        os.remove(new_file.path)
        print("PROMOTED %s -> %s, removed %s" % (new_file.path, valid_path, new_file.path))

    def validate_file (m, new_file, valid_files):
        valid_file = find_results_file_by_tag(valid_files, new_file.tag)
        if valid_file is None:
            valid_file = load_results_file_for_tag(m.search_dir, RESULTS_VALID_PREFIX, new_file.tag)
        print("FILE %s (%s)" % (new_file.tag, new_file.path))
        if valid_file is not None:
            print("  auto compare vs %s" % valid_file.path)
            no_count = m.run_auto(new_file, valid_file)
        else:
            print("  manual compare (no valid file)")
            no_count = m.run_manual(new_file)
        print("  FILE %s: no_count=%d" % (new_file.tag, no_count))
        if no_count == 0:
            m.promote_new_to_valid(new_file)
            print("  FILE %s: promoted" % new_file.tag)
        else:
            print("  FILE %s: not promoted" % new_file.tag)
        return no_count == 0

    def run (m, new_files, valid_files):
        print("=== results validate session ===")
        for new_file in new_files:
            m.validate_file(new_file, valid_files)

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    this_dir = os.path.dirname(os.path.abspath(__file__))
    search_dir = this_dir
    if len(sys.argv) > 1:
        search_dir = sys.argv[1]
    results_new_files = load_results_files(search_dir, RESULTS_NEW_PREFIX)
    results_valid_files = load_results_files(search_dir, RESULTS_VALID_PREFIX)
    Validator(search_dir).run(results_new_files, results_valid_files)

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
