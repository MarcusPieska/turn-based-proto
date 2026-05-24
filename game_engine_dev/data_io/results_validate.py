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
        m.log_path = os.path.join(search_dir, "results_validate.log")

    def log (m, line):
        with open(m.log_path, "a") as ptr:
            ptr.write(line + "\n")
        print(line)

    def ask_yn (m, prompt):
        while True:
            ans = input(prompt).strip().lower()
            if ans == "y" or ans == "n":
                m.log("%s %s" % (prompt.rstrip(), ans))
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
        all_y = True
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
                all_y = False
        return all_y

    def run_manual (m, new_file):
        all_y = True
        for name in sorted(new_file.by_name.keys()):
            new_res = new_file.by_name[name]
            print(new_res.full)
            print("-----------------------------------------------------------")
            prompt = "%s:%s manual accept (y/n): " % (new_file.tag, name)
            if not m.ask_yn(prompt):
                all_y = False
        return all_y

    def promote_new_to_valid (m, new_file):
        valid_path = valid_path_for_tag(m.search_dir, new_file.tag)
        with open(new_file.path, "r") as ptr:
            content = ptr.read()
        with open(valid_path, "w") as ptr:
            ptr.write(content)
        os.remove(new_file.path)
        m.log("PROMOTED %s -> %s, removed %s" % (new_file.path, valid_path, new_file.path))

    def validate_file (m, new_file, valid_files):
        valid_file = find_results_file_by_tag(valid_files, new_file.tag)
        if valid_file is None:
            valid_file = load_results_file_for_tag(m.search_dir, RESULTS_VALID_PREFIX, new_file.tag)
        m.log("FILE %s (%s)" % (new_file.tag, new_file.path))
        if valid_file is not None:
            m.log("  auto compare vs %s" % valid_file.path)
            all_y = m.run_auto(new_file, valid_file)
        else:
            m.log("  manual compare (no valid file)")
            all_y = m.run_manual(new_file)
        if all_y:
            m.promote_new_to_valid(new_file)
            m.log("  FILE %s: all accepted" % new_file.tag)
        else:
            m.log("  FILE %s: not promoted" % new_file.tag)
        return all_y

    def run (m, new_files, valid_files):
        m.log("=== results validate session ===")
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
