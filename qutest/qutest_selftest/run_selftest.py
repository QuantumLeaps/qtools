#!/usr/bin/env python3
#=============================================================================
# QUTest self-test runner
#
# Runs the QUTest script runner (qutest.py) -- selected EXPLICITLY by path --
# against the self-test scripts in ./scripts and checks the observable
# outcome of each run (per-test verdicts, summary counts, exit status, and
# required/forbidden output) against the expectations embedded in each
# script. The purpose is to verify qutest.py itself, in particular that it
# DETECTS failures (the fail-paths).
#
# Copyright (C) 2005-2026 Quantum Leaps, LLC. All rights reserved.
# SPDX-License-Identifier: LicenseRef-QL-commercial
# Licensed under the terms of the Quantum Leaps commercial license agreement.
#=============================================================================
"""
Expectations are embedded in each script as comment lines of the form

    #@ key: value

Recognized keys (all optional, except 'purpose'):
  purpose    one-line description of what the script verifies
  verdicts   expected verdict per test number, e.g. "1=PASS 2=FAIL 3=SKIPPED";
             alternatives are separated by '|', e.g. "3=FAIL|SKIPPED"
  tests      expected 'Tests' count in the summary
  failed     expected 'Failed' count in the summary
  skipped    expected 'Skipped' count in the summary
  exit       expected exit status: a number, 'zero', 'nonzero', or 'any'
  output     regular expression that MUST match the output (repeatable)
  no-output  regular expression that must NOT match the output (repeatable)
  args       extra command-line arguments for qutest.py (e.g. "-ox")
  bug        ID of a known defect in qutest.py (see README.md); the script
             describes the CORRECT behavior, so it is expected to fail until
             the defect is fixed (reported as KNOWN, not as FAIL)
  decision   marks the expected behavior as an open design decision
  summary    'none': the run must end WITHOUT a SUMMARY (a run-level error,
             e.g., QSPY not reachable, QSPY detached) and with a non-zero
             exit status; I4 is then replaced by exactly this check, and
             the test running at the abort may have no verdict (I1)
  host-max   maximum number of host executables (fixture processes) that
             may run at the same time during the script (sampled every
             50 ms by the runner, see I5 below)
  max-time   maximum run time of qutest.py for this script [s] (e.g. "0.6");
             detects fixed delays (e.g., at the end of a session)
  fakeqspy   'yes': run qutest.py in remote-target mode (no -e) against
             fakeqspy/fakeqspy.py (UDP port = --udp + 1, started fresh for
             this script) instead of the real QSPY and the fixture

In addition, the following integrity rules are always checked:
  I1  every test reported in the output has exactly one verdict; a verdict
      outside any test (a failure of a whole group, before its first test)
      must be FAIL, and there may be at most as many as the summary's FAILED
      list has "group:<n>" entries
  I2  the summary 'Tests' count equals the number of tests in the output
  I3  the summary 'Skipped' count equals the number of SKIPPED verdicts
  I4  the exit status is non-zero if and only if the summary reports
      failures (and the final banner says FAIL)
  I5  no host executable (fixture) is left running: within 2 s after
      qutest.py exits, no process with the fixture's image name may exist.
      A leaked host executable lingers and interferes with subsequent runs,
      so I5 makes the script FAIL even if it is tagged with #@ bug, and the
      runner kills the leaked process(es). The runner also refuses to start
      if a fixture process is already running (e.g., leaked by an earlier
      run). In addition, the maximum number of fixture processes running at
      the same time during each script is recorded in the report.
"""

import argparse
import datetime
import glob
import hashlib
import os
import platform
import re
import shutil
import signal
import subprocess
import sys
import threading
import time

VERSION = "1.6.0"  # 1.6.0: max-time; 1.5.0: summary: none
# 1.4.0: group failures (I1)
# 1.3.0: host-max; 1.2.0: I5
# 1.1.0: fakeqspy; 1.0.1: CRLF
HERE = os.path.dirname(os.path.abspath(__file__))
FAKEQSPY = os.path.join(HERE, "fakeqspy", "fakeqspy.py")
IS_WIN = (os.name == "nt")

ANSI_RE   = re.compile(r"\x1b\[[0-9;]*m")
HDR_RE    = re.compile(r"^\[\s*(\d+)\]([-^])")
PASS_RE   = re.compile(r"\[ PASS \(\s*[\d.]+s\) \]\s*$")
FAIL_RE   = re.compile(r"^! !.*\[ FAIL \(\s*[\d.]+s\) \]\s*$")
SKIP_RE   = re.compile(r"\[ SKIPPED \]\s*$")
SUMM_RE   = re.compile(r"^=+\[ SUMMARY \]=+")
CNT_RE    = re.compile(r"^(Groups|Tests|Skipped|Failed|FAILED)\s*:\s*(\d+)")
GROUP_RE  = re.compile(r"^=+\[Group\s+\d+\]=+")
FLIST_RE  = re.compile(r"^FAILED\s*:\s*\d+\s*\[(.*)\]")
FINAL_RE  = re.compile(r"^=+\[\s*(OK|FAIL)\s+\(")

#-----------------------------------------------------------------------------
def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()

def parse_expectations(path):
    exp = {"output": [], "no-output": []}
    with open(path, encoding="utf-8") as f:
        for line in f:
            m = re.match(r"^#@\s*([\w-]+)\s*:\s*(.*?)\s*$", line)
            if not m:
                continue
            key, val = m.group(1).lower(), m.group(2)
            if key in ("output", "no-output"):
                exp[key].append(val)
            else:
                exp[key] = val
    if "verdicts" in exp:
        verd = {}
        for item in exp["verdicts"].split():
            num, alts = item.split("=")
            verd[int(num)] = set(alts.upper().split("|"))
        exp["verdicts"] = verd
    return exp

def parse_output(text):
    """Parse the (ANSI-stripped) qutest.py output into a result record."""
    res = {"tests": [], "verdicts": {}, "orphans": [], "counts": {},
           "final": None, "failed_list": []}
    current = None
    in_summary = False
    for line in text.splitlines():
        if SUMM_RE.match(line):
            in_summary = True
            continue
        if in_summary:
            m = CNT_RE.match(line)
            if m:
                key = m.group(1).capitalize()
                res["counts"][key] = int(m.group(2))
            m = FLIST_RE.match(line)
            if m:
                res["failed_list"] = m.group(1).split()
            m = FINAL_RE.match(line)
            if m:
                res["final"] = m.group(1)
            continue
        if GROUP_RE.match(line): # a new group: no current test
            current = None
            continue
        m = HDR_RE.match(line)
        if m:
            current = int(m.group(1))
            res["tests"].append(current)
            res["verdicts"].setdefault(current, [])
            continue
        verdict = None
        if FAIL_RE.search(line):
            verdict = "FAIL"
        elif PASS_RE.search(line):
            verdict = "PASS"
        elif SKIP_RE.search(line):
            verdict = "SKIPPED"
        if verdict:
            if current is None:
                res["orphans"].append(verdict)
            else:
                res["verdicts"][current].append(verdict)
    return res

def check(exp, res, rc, text):
    """Return a list of human-readable problems (empty list = as expected)."""
    probs = []
    # integrity rules ........................................................
    aborted = (exp.get("summary", "").lower() == "none") # run-level error
    for num in res["tests"]:
        v = res["verdicts"].get(num, [])
        # NOTE: when the run is aborted by a run-level error, the test that was
        # running has no verdict -- but no test may have more than one
        if len(v) > 1 or (len(v) == 0 and not aborted):
            probs.append(f"I1: test {num} has {len(v)} verdicts {v}")
    n_grp = sum(1 for t in res["failed_list"] if t.startswith("group:"))
    if any(v != "FAIL" for v in res["orphans"]) or \
       len(res["orphans"]) > n_grp:
        probs.append(f"I1: verdicts outside any test: {res['orphans']} "
                     f"(group failures in the summary: {n_grp})")
    cnt = res["counts"]
    if "Tests" in cnt and cnt["Tests"] != len(res["tests"]):
        probs.append(f"I2: summary Tests={cnt['Tests']}, "
                     f"tests in output={len(res['tests'])}")
    n_skip = sum(v.count("SKIPPED") for v in res["verdicts"].values())
    if "Skipped" in cnt and cnt["Skipped"] != n_skip:
        probs.append(f"I3: summary Skipped={cnt['Skipped']}, "
                     f"SKIPPED verdicts={n_skip}")
    if exp.get("summary", "").lower() == "none": # run-level error expected
        if res["final"] is not None or cnt:
            probs.append("summary: expected no SUMMARY (run-level error), "
                         "but a SUMMARY was printed")
        if rc == 0:
            probs.append("summary: a run-level error must give a non-zero "
                         "exit status, got 0")
    elif res["final"] is None or "Failed" not in cnt:
        probs.append("I4: no complete SUMMARY in the output")
    else:
        failed = cnt["Failed"] > 0
        if failed != (rc != 0) or failed != (res["final"] == "FAIL"):
            probs.append(f"I4: inconsistent outcome: Failed={cnt['Failed']}, "
                         f"final={res['final']}, exit={rc}")
    # explicit expectations ..................................................
    for num, alts in exp.get("verdicts", {}).items():
        got = res["verdicts"].get(num)
        if not got:
            probs.append(f"test {num}: expected {'|'.join(sorted(alts))}, "
                         "but the test was not reported")
        elif got[-1] not in alts:
            probs.append(f"test {num}: expected {'|'.join(sorted(alts))}, "
                         f"got {got}")
    extra = [n for n in res["tests"] if n not in exp.get("verdicts", {})]
    if "verdicts" in exp and extra:
        probs.append(f"unexpected tests reported: {extra}")
    for key in ("tests", "failed", "skipped"):
        if key in exp:
            got = cnt.get(key.capitalize())
            if got != int(exp[key]):
                probs.append(f"summary {key.capitalize()}: expected "
                             f"{exp[key]}, got {got}")
    want = exp.get("exit", "any").lower()
    if want == "zero" and rc != 0 or want == "nonzero" and rc == 0 \
       or want.lstrip("-").isdigit() and rc != int(want):
        probs.append(f"exit status: expected {want}, got {rc}")
    for rex in exp["output"]:
        if not re.search(rex, text, re.MULTILINE):
            probs.append(f"missing output: /{rex}/")
    for rex in exp["no-output"]:
        if re.search(rex, text, re.MULTILINE):
            probs.append(f"forbidden output present: /{rex}/")
    return probs

#-----------------------------------------------------------------------------
def host_procs(exe):
    """PIDs of the running processes with the image name of exe.
    Raises RuntimeError if the processes can't be listed (so that the leak
    check can never pass silently because it doesn't work)."""
    name = os.path.basename(exe)
    pids = []
    try:
        if IS_WIN:
            out = subprocess.run(
                ["tasklist", "/FO", "CSV", "/NH",
                 "/FI", f"IMAGENAME eq {name}"],
                capture_output=True, text=True, timeout=20, check=True).stdout
            for line in out.splitlines():
                cols = [c.strip('"') for c in line.strip().split('","')]
                if len(cols) > 1 and cols[0].lower() == name.lower():
                    pids.append(int(cols[1]))
        else:
            out = subprocess.run(["ps", "-eo", "pid=,comm="],
                capture_output=True, text=True, timeout=20, check=True).stdout
            for line in out.splitlines():
                parts = line.split(None, 1)
                # NOTE: Linux truncates 'comm' to 15 characters;
                # macOS reports the full path
                if len(parts) == 2 and \
                   os.path.basename(parts[1].strip())[:15] == name[:15]:
                    pids.append(int(parts[0]))
    except (OSError, ValueError, subprocess.SubprocessError) as err:
        raise RuntimeError(f"cannot list the running processes: {err}")
    return pids

def kill_procs(pids):
    for pid in pids:
        try:
            if IS_WIN:
                subprocess.run(["taskkill", "/F", "/PID", str(pid)],
                               capture_output=True, timeout=20)
            else:
                os.kill(pid, signal.SIGKILL)
        except (OSError, subprocess.SubprocessError):
            pass

def read_raw(path):
    """Read the raw output of qutest.py: decoded, CRLF->LF, no ANSI colors."""
    try:
        with open(path, "rb") as f:
            raw = f.read().decode("utf-8", errors="replace")
    except OSError:
        return ""
    return ANSI_RE.sub("", raw.replace("\r\n", "\n"))

class HostMonitor(threading.Thread):
    """Samples the number of running host executables during a script."""
    def __init__(self, exe):
        super().__init__(daemon=True)
        self.exe = exe
        self.max = 0
        self.halt = threading.Event()
    def run(self):
        while not self.halt.is_set():
            try:
                self.max = max(self.max, len(host_procs(self.exe)))
            except RuntimeError:
                pass # reported by the leak check after the script
            self.halt.wait(0.05)
    def finish(self):
        self.halt.set()
        self.join()
        return self.max

def find_qspy(arg):
    if arg:
        return arg
    exe = "qspy.exe" if IS_WIN else "qspy"
    cands = []
    qtools = os.environ.get("QTOOLS")
    roots = [qtools] if qtools else []
    roots.append(os.path.normpath(os.path.join(HERE, "..", "..")))
    for root in roots:
        cands += [os.path.join(root, "bin", exe),
                  os.path.join(root, "bin_linux" if not IS_WIN else "bin",
                               exe)]
    for c in cands:
        if os.path.isfile(c):
            return c
    return shutil.which(exe)

def find_python(arg):
    """Interpreter used to run qutest.py (in a separate process).

    NOTE: in some embedded Python distributions (e.g., the MinGW Python
    bundled with QTools for Windows) sys.executable reports the Python
    DLL rather than an executable, so it cannot be used blindly.
    """
    if arg:
        return arg
    exe = sys.executable or ""
    ok_ext = (not IS_WIN) or exe.lower().endswith(".exe")
    if ok_ext and os.path.isfile(exe) and os.access(exe, os.X_OK):
        return exe
    if IS_WIN:
        # launcher used by the QTools 'python3.bat' (MinGW Python)
        cand = os.path.join(os.path.dirname(os.__file__),
                            "venv", "scripts", "nt", "python.exe")
        if os.path.isfile(cand):
            return cand
        return shutil.which("python.exe")
    return shutil.which("python3") or shutil.which("python")

def qspy_version(qspy):
    try:
        out = subprocess.run([qspy, "-h"], capture_output=True, text=True,
                             timeout=10).stdout
        m = re.search(r"QSPY\s+(\S+)", out)
        return m.group(1) if m else "?"
    except Exception:
        return "?"

def main():
    default_exe = os.path.join(HERE, "fixture", "build",
                  "selftest_fixture" + (".exe" if IS_WIN else ""))
    ap = argparse.ArgumentParser(
        description="QUTest self-test runner (verifies qutest.py itself)")
    ap.add_argument("--qutest", default=os.path.join(HERE, "..",
                    "qutest.py"),
                    help="qutest.py under test (default: ../qutest.py)")
    ap.add_argument("--python", default=None,
                    help="Python interpreter for running qutest.py "
                         "(default: auto-detected)")
    ap.add_argument("--qspy", default=None,
                    help="QSPY executable (default: $QTOOLS/bin or "
                         "bin_linux)")
    ap.add_argument("--exe", default=default_exe,
                    help="self-test fixture executable")
    ap.add_argument("--udp", type=int, default=7751, help="QSPY UDP port")
    ap.add_argument("--tcp", type=int, default=6751, help="QSPY TCP port")
    ap.add_argument("--timeout", type=int, default=180,
                    help="time limit per script [s]")
    ap.add_argument("--scripts", default=os.path.join(HERE, "scripts"),
                    help="directory with the self-test scripts")
    ap.add_argument("--results", default=os.path.join(HERE, "results"),
                    help="directory for the report and raw outputs")
    ap.add_argument("--strict", action="store_true",
                    help="treat KNOWN (bug-tagged) failures as failures")
    ap.add_argument("-k", dest="filter", default="",
                    help="run only scripts whose name contains this text")
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="print the problems found for every script")
    args = ap.parse_args()

    qutest = os.path.abspath(args.qutest)
    qspy = find_qspy(args.qspy)
    exe = os.path.abspath(args.exe)
    python = find_python(args.python)
    for what, path in (("Python", python), ("qutest.py", qutest),
                       ("QSPY", qspy),
                       ("fixture", exe)):
        if not path or not os.path.isfile(path):
            print(f"ERROR: {what} not found: {path}")
            if what == "fixture":
                print("       build it first: make -C fixture")
            return 2

    scripts = sorted(glob.glob(os.path.join(os.path.abspath(args.scripts),
                                       "st*.py")))
    scripts = [s for s in scripts if args.filter in os.path.basename(s)]
    if not scripts:
        print("ERROR: no self-test scripts selected")
        return 2

    run_id = datetime.datetime.now().strftime("%y%m%d_%H%M%S")
    out_dir = os.path.join(args.results, run_id)
    os.makedirs(out_dir, exist_ok=True)

    hdr = [
        f"QUTest self-test runner {VERSION}",
        f"Run ID     : {run_id}",
        f"Host       : {platform.node()} ({platform.platform()})",
        f"Python     : {platform.python_version()} (runner: "
        f"{sys.executable})",
        f"  qutest   : {python}",
        f"qutest.py  : {qutest}",
        f"  sha256   : {sha256(qutest)}",
        f"QSPY       : {qspy} (version {qspy_version(qspy)})",
        f"  sha256   : {sha256(qspy)}",
        f"Fixture    : {exe}",
        f"  sha256   : {sha256(exe)}",
        f"Fake QSPY  : {FAKEQSPY}",
        f"  sha256   : {sha256(FAKEQSPY)}",
        f"Scripts    : {len(scripts)}",
        "",
    ]
    print("\n".join(hdr))

    # no host executable may be running before the run (I5) ..............
    try:
        stale = host_procs(exe)
    except RuntimeError as err:
        print(f"ERROR: leak check (I5) not possible: {err}")
        return 2
    if stale:
        print(f"ERROR: {len(stale)} instance(s) of {os.path.basename(exe)} "
              f"already running (PIDs {stale}), probably leaked by an earlier "
              f"run; terminate them first")
        return 2

    # start QSPY ...........................................................
    qspy_log = open(os.path.join(out_dir, "qspy.log"), "w")
    qspy_proc = subprocess.Popen(
        [qspy, "-k", "-q", "-u", str(args.udp), "-t", str(args.tcp)],
        stdout=qspy_log, stderr=subprocess.STDOUT,
        stdin=subprocess.DEVNULL)
    time.sleep(1.0)
    if qspy_proc.poll() is not None:
        print("ERROR: QSPY failed to start (see qspy.log)")
        return 2

    rows = []
    tally = {"OK": 0, "FAIL": 0, "KNOWN": 0, "FIXED?": 0, "ERROR": 0}
    try:
        for scr in scripts:
            name = os.path.splitext(os.path.basename(scr))[0]
            exp = parse_expectations(scr)
            fake = None
            if exp.get("fakeqspy", "").lower() == "yes":
                # fresh fake QSPY (and fake target) for this script
                fport = args.udp + 1
                fake = subprocess.Popen(
                    [python, "-B", FAKEQSPY, str(fport)],
                    stdout=qspy_log, stderr=subprocess.STDOUT,
                    stdin=subprocess.DEVNULL)
                time.sleep(0.5)
                cmd = [python, qutest, f"-qlocalhost:{fport}"]
            else:
                cmd = [python, qutest, "-e" + exe,
                       f"-qlocalhost:{args.udp}:{args.tcp}"]
            cmd += exp.get("args", "").split()
            cmd += [os.path.basename(scr)]
            raw_path = os.path.join(out_dir, name + ".raw")
            t0 = time.time()
            monitor = HostMonitor(exe)
            monitor.start()
            try:
                if fake is not None and fake.poll() is not None:
                    raise RuntimeError("fake QSPY failed to start")
                # NOTE: the output goes to a FILE, not to a pipe: a host
                # executable leaked by qutest.py inherits the output handle,
                # and with a pipe the runner would wait for the leaked
                # process to end (hiding the leak from the I5 check)
                with open(raw_path, "wb") as raw_f:
                    t_run = time.time()
                    proc = subprocess.run(cmd, cwd=os.path.dirname(scr),
                        stdout=raw_f, stderr=subprocess.STDOUT,
                        stdin=subprocess.DEVNULL, timeout=args.timeout)
                    t_run = time.time() - t_run
                rc = proc.returncode
                text = read_raw(raw_path)
                probs = check(exp, parse_output(text), rc, text)
                if ("max-time" in exp) and (t_run > float(exp["max-time"])):
                    probs.append(f"qutest.py run time {t_run:.2f} s exceeds "
                                 f"max-time {exp['max-time']} s")
                timeout = False
            except subprocess.TimeoutExpired:
                rc, timeout = None, True
                text = read_raw(raw_path)
                probs = [f"qutest.py did not finish within {args.timeout}s"]
            except RuntimeError as err: # fake QSPY failed to start
                rc, timeout, text = None, True, ""
                probs = [str(err)]
            finally:
                host_max = monitor.finish()
                if fake is not None:
                    fake.terminate()
                    try:
                        fake.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        fake.kill()

            # I5: no leaked host executable ................................
            leaked = []
            try:
                deadline = time.time() + 2.0
                while True:
                    leaked = host_procs(exe)
                    if (not leaked) or (time.time() > deadline):
                        break
                    time.sleep(0.1)
                if leaked:
                    probs.append(f"I5: leaked host executable: {len(leaked)} "
                        f"process(es) {leaked} still running 2 s after "
                        f"qutest.py exited (killed by the runner)")
                    kill_procs(leaked)
            except RuntimeError as err:
                leaked = ["?"]
                probs.append(f"I5: leak check not possible: {err}")
            if "host-max" in exp and host_max > int(exp["host-max"]):
                probs.append(f"host executables running at the same time: "
                    f"{host_max}, expected at most {exp['host-max']}")
            dt = time.time() - t0
            with open(os.path.join(out_dir, name + ".out"), "w",
                      encoding="utf-8") as f:
                f.write(text)

            bug = exp.get("bug")
            if timeout:
                status = "ERROR"
            elif not probs:
                status = "FIXED?" if bug else "OK"
            else:
                status = "KNOWN" if bug else "FAIL"
            if leaked: # a leaked host executable is never "known"
                status = "FAIL" if not timeout else "ERROR"
            tally[status] += 1
            tag = f" [{bug}]" if bug else ""
            if "decision" in exp:
                tag += " (decision)"
            line = f"{name:<44} {status:<7}{tag}  ({dt:4.1f}s)"
            print(line)
            if probs and (args.verbose or status in ("FAIL", "ERROR")):
                for p in probs:
                    print(f"    - {p}")
            rows.append((name, status, exp, probs, rc, host_max))
    finally:
        qspy_proc.terminate()
        try:
            qspy_proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            qspy_proc.kill()
        qspy_log.close()

    summ = (f"\nOK={tally['OK']}  KNOWN={tally['KNOWN']}  "
            f"FIXED?={tally['FIXED?']}  FAIL={tally['FAIL']}  "
            f"ERROR={tally['ERROR']}")
    print(summ)

    # write the report .....................................................
    rep = os.path.join(out_dir, "report.txt")
    with open(rep, "w", encoding="utf-8") as f:
        f.write("\n".join(hdr) + "\n")
        for name, status, exp, probs, rc, host_max in rows:
            f.write(f"{name}: {status}")
            if exp.get("bug"):
                f.write(f" [{exp['bug']}]")
            f.write(f"  exit={rc}  host-exe-max-concurrent={host_max}\n")
            f.write(f"    purpose: {exp.get('purpose', '?')}\n")
            if "decision" in exp:
                f.write(f"    decision: {exp['decision']}\n")
            for p in probs:
                f.write(f"    - {p}\n")
        f.write(summ + "\n")
    print(f"Report     : {rep}")

    bad = tally["FAIL"] + tally["ERROR"] + tally["FIXED?"]
    if args.strict:
        bad += tally["KNOWN"]
    return 1 if bad else 0

if __name__ == "__main__":
    sys.exit(main())
