#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path

#================================================================================================================================
#=> - Suite -
#================================================================================================================================

def load_suite(path: Path):
    names = []
    for line in path.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name = line[2:] if line.startswith("./") else line
        names.append(name)
    return names

def run_cmd(argv, cwd: Path):
    print("---", " ".join(argv), flush=True)
    r = subprocess.run(argv, cwd=str(cwd))
    return r.returncode

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    force_comp = len(sys.argv) > 1 and sys.argv[1] == "comp"
    here = Path(__file__).resolve().parent
    suite_path = here / "current_suite"
    if not suite_path.is_file():
        print(f"missing suite list: {suite_path}", file=sys.stderr)
        return 1
    names = load_suite(suite_path)
    if not names:
        print(f"empty suite: {suite_path}", file=sys.stderr)
        return 1
    failed = []
    for name in names:
        exe = here / name
        if not name.endswith("_tester"):
            print(f"skip (not *_tester): {name}", file=sys.stderr)
            failed.append(name)
            continue
        comp = here / (name[: -len("_tester")] + "_comp")
        need_build = force_comp or not exe.is_file()
        if need_build:
            if not comp.is_file():
                print(f"missing comp script: {comp}", file=sys.stderr)
                failed.append(name)
                continue
            rc = run_cmd(["bash", str(comp)], here)
            if rc != 0:
                failed.append(name)
            continue
        if not exe.is_file():
            print(f"missing exe after build: {exe}", file=sys.stderr)
            failed.append(name)
            continue
        rc = run_cmd([str(exe)], here)
        if rc != 0:
            failed.append(name)
    if failed:
        print(f"current_suite failed: {' '.join(failed)}", file=sys.stderr)
        return 1
    print("current_suite ok")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
