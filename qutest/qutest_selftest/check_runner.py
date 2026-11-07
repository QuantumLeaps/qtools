#!/usr/bin/env python3
#=============================================================================
# check_runner.py -- mutation check of the self-test runner (run_selftest.py)
#
# Copyright (C) 2005-2026 Quantum Leaps, LLC. All rights reserved.
# SPDX-License-Identifier: LicenseRef-QL-commercial
# Licensed under the terms of the Quantum Leaps commercial license agreement.
#
# Verifies that run_selftest.py DETECTS wrong expectations and wrong outcomes,
# i.e., that an "OK" from the runner means something. The scripts are copied
# to a scratch directory; each mutant changes ONE thing (an expectation line,
# a tag, an added script, a stale process), the runner is run on it, and the
# runner's verdict for the mutated script, its exit code, and (optionally)
# a required text in its output are compared with what the mutant must give.
# A control run of the unmodified scripts must report OK for all of them.
#
# usage: python3 check_runner.py [run_selftest.py options, e.g. --exe --qspy]
# exit status: 0 if every mutant is caught (and the control run is OK)
#=============================================================================
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
RUNNER = os.path.join(HERE, "run_selftest.py")

# the runner is started with the same interpreter detection as the runner
# uses for qutest.py (sys.executable is the Python DLL in the QTools MinGW
# Python on Windows, so it cannot be used blindly)
sys.dont_write_bytecode = True   # no __pycache__ in the suite folder
sys.path.insert(0, HERE)
from run_selftest import find_python  # pylint: disable=wrong-import-position
PYTHON = find_python(None)
IS_WIN = (os.name == "nt")
STATUS_RE = re.compile(r"^(st\w+)\s+(OK|KNOWN|FIXED\?|FAIL|ERROR)(?=\s|$)", re.M)

# a passing script that leaves a process with the fixture's image name
# running after qutest.py exits (a decoy, started by the script itself), so
# that ONLY the leak check I5 can make it fail, independently of whether
# qutest.py itself leaks (since the A6 fix it does not)
LEAK_SCRIPT = """# mutant: leaked host executable
#@ purpose: (mutant) a host executable left running must be caught by I5
{tag}#@ verdicts: 1=PASS
#@ tests: 1
#@ exit: zero
import subprocess
subprocess.Popen({decoy!r}, stdout=subprocess.DEVNULL,
                 stderr=subprocess.DEVNULL, stdin=subprocess.DEVNULL)
test("passes, but a host executable is left running")
command("CMD_ECHO", 2)
expect("@timestamp ECHO 2")
expect("@timestamp Trg-Done QS_RX_COMMAND")
"""

# (id, script, old text, new text, expected status, expected runner exit,
#  required text in the runner output, extra runner args)
MUTANTS = [
 ("M01 verdict",        "st01_pass_basic", "#@ verdicts: 1=PASS",
                        "#@ verdicts: 1=FAIL", "FAIL", 1, None, []),
 ("M02 tests count",    "st02_pass_noreset", "#@ tests: 3",
                        "#@ tests: 4", "FAIL", 1, None, []),
 ("M03 exit status",    "st03_fail_mismatch", "#@ exit: nonzero",
                        "#@ exit: zero", "FAIL", 1, None, []),
 ("M04 required output","st07_fail_unexpected_assert", "Loc=42",
                        "Loc=43", "FAIL", 1, None, []),
 ("M05 failed count",   "st08_fail_noreset_after_fail", "#@ failed: 2",
                        "#@ failed: 1", "FAIL", 1, None, []),
 ("M06 skipped count",  "st09_pass_skip", "#@ skipped: 1",
                        "#@ skipped: 0", "FAIL", 1, None, []),
 ("M07 forbidden output","st31_bug_C2_ao_filter_remove_by_name",
                        "#@ no-output: Object Dictionary not found",
                        "#@ no-output: ao_filter\\(\\) remove and add",
                        "FAIL", 1, None, []),
 ("M08 fakeqspy verdict","st36_pass_fakeqspy_baseline",
                        "#@ verdicts: 1=PASS 2=PASS 3=PASS",
                        "#@ verdicts: 1=PASS 2=FAIL 3=PASS", "FAIL", 1, None, []),
 ("M09 fakeqspy output","st37_bug_A4_lost_packet", r"\(lost packets\)",
                        r"\(lost packet!\)", "FAIL", 1, None, []),
 ("M10 numeric exit",   "st34_bug_A1_exit_status_256_failures",
                        "#@ exit: 255", "#@ exit: 254", "FAIL", 1, None, []),
 ("M11 bug tag on a passing script", "st21_bug_C1_command_while_skipping",
                        "#@ fixed: C1", "#@ bug: C1", "FIXED?", 1, None, []),
 ("M12 KNOWN (non-strict)", "st01_pass_basic", "#@ verdicts: 1=PASS",
                        "#@ bug: MUT\n#@ verdicts: 1=FAIL", "KNOWN", 0, None, []),
 ("M13 KNOWN (--strict)", "st01_pass_basic", "#@ verdicts: 1=PASS",
                        "#@ bug: MUT\n#@ verdicts: 1=FAIL", "KNOWN", 1, None,
                        ["--strict"]),
 ("M17 host-max exceeded", "st45_bug_A6_assert_overlap", "#@ host-max: 1",
                        "#@ host-max: 0", "FAIL", 1,
                        "host executables running at the same time", []),
 ("M18 run-level error not declared", "st66_fail_qspy_unreachable",
                        "#@ summary: none\n", "", "FAIL", 1,
                        "I4: no complete SUMMARY", []),
 ("M19 max-time exceeded", "st69_bug_M8_session_end_delay",
                        "#@ max-time: 0.6", "#@ max-time: 0.01", "FAIL", 1,
                        "exceeds max-time", []),
 ("M14 leaked host exe (I5)", "stM1_leak", None, "LEAK:",
                        "FAIL", 1, "I5: leaked host executable", []),
 ("M15 leaked host exe, bug-tagged (I5)", "stM2_leak", None,
                        "LEAK:#@ bug: MUT\n",
                        "FAIL", 1, "I5: leaked host executable", []),
]

def run(scripts_dir, results, args, key):
    cmd = [PYTHON, "-B", RUNNER, "--scripts", scripts_dir,
           "--results", results, "-k", key] + args
    with tempfile.TemporaryFile() as out:  # file, not pipe (see I5)
        rc = subprocess.run(cmd, stdout=out, stderr=subprocess.STDOUT,
                            stdin=subprocess.DEVNULL).returncode
        out.seek(0)
        text = out.read().decode("utf-8", "replace").replace("\r\n", "\n")
    return rc, text, dict(STATUS_RE.findall(text))

def main():
    fwd = sys.argv[1:] # options forwarded to run_selftest.py
    tmp = tempfile.mkdtemp(prefix="check_runner_")
    scr = os.path.join(tmp, "scripts")
    shutil.copytree(os.path.join(HERE, "scripts"), scr)
    res = os.path.join(tmp, "results")
    bad = 0
    print(f"check_runner: scratch dir {tmp}")
    print(f"check_runner: runner started with {PYTHON}")

    # decoy: a long-running process with the fixture's image name ..........
    exe_name = "selftest_fixture" + (".exe" if IS_WIN else "")
    decoy = os.path.join(tmp, "decoy", exe_name)
    os.makedirs(os.path.dirname(decoy))
    if IS_WIN:
        shutil.copy(os.path.join(os.environ.get("SystemRoot", r"C:\Windows"),
                                 "System32", "PING.EXE"), decoy)
        decoy_cmd = [decoy, "-n", "30", "127.0.0.1"]
    else:
        shutil.copy(shutil.which("sleep"), decoy)
        decoy_cmd = [decoy, "30"]

    # control: every script used by a mutant must be OK unmodified ........
    for name in sorted({m[1] for m in MUTANTS if m[2] is not None}):
        rc, text, st = run(scr, res, fwd, name)
        ok = (st.get(name) == "OK") and (rc == 0)
        bad += not ok
        print(f"  control  {name:<44} {st.get(name, '?'):<7} "
              f"{'ok' if ok else 'NOT OK'}")

    # mutants ..............................................................
    for mid, name, old, new, exp_st, exp_rc, need, extra in MUTANTS:
        path = os.path.join(scr, name + ".py")
        orig = None
        if old is None: # added script
            if new.startswith("LEAK:"):
                new = LEAK_SCRIPT.format(tag=new[5:], decoy=decoy_cmd)
            with open(path, "w", encoding="utf-8") as f:
                f.write(new)
        else:
            with open(path, encoding="utf-8") as f:
                orig = f.read()
            assert orig.count(old) == 1, f"{mid}: '{old}' not unique in {name}"
            with open(path, "w", encoding="utf-8") as f:
                f.write(orig.replace(old, new))
        try:
            rc, text, st = run(scr, res, fwd + extra, name)
        finally:
            if orig is None:
                os.remove(path)
            else:
                with open(path, "w", encoding="utf-8") as f:
                    f.write(orig)
        got = st.get(name, "?")
        ok = (got == exp_st) and (rc == exp_rc) and \
             (need is None or need in text)
        bad += not ok
        print(f"  {mid:<38} expect {exp_st:<6} rc={exp_rc}  "
              f"got {got:<6} rc={rc}  {'caught' if ok else 'NOT CAUGHT'}")

    # stale host executable before the run: the runner must refuse to run ..
    proc = subprocess.Popen(decoy_cmd, stdout=subprocess.DEVNULL,
                            stderr=subprocess.DEVNULL)
    time.sleep(0.5)
    try:
        rc, text, st = run(scr, res, fwd, "st01_pass_basic")
    finally:
        proc.kill()
        proc.wait()
    ok = (rc == 2) and ("already running" in text) and not st
    bad += not ok
    print(f"  {'M16 stale host exe before the run':<38} expect refuse rc=2  "
          f"got rc={rc}  {'caught' if ok else 'NOT CAUGHT'}")

    shutil.rmtree(tmp, ignore_errors=True)
    print(f"\ncheck_runner: {'ALL CAUGHT' if bad == 0 else f'{bad} PROBLEM(S)'}")
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
