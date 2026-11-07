# QUTest self-test suite

This suite verifies the QUTest script runner (`qutest.py`) **itself**, with
emphasis on its fail-paths: that QUTest *detects* and correctly reports
failing tests. It is deliberately separate from the QP test suites
(e.g., `safe-qpc/tests`), which verify QP and use QUTest as a tool.

## Contents

| Path               | Purpose |
|--------------------|---------|
| `fixture/`         | Minimal QUTest target whose behavior is fully controlled by commands (echo, burst, silence, assertion, delay, exit, setup/teardown output, a variable, an active object `AO_Tst` for `ao_filter()`). Builds against a pinned QP/C (`../../../../safe-qpc` by default). |
| `scripts/`         | Self-test scripts `stNN_*.py`, each with its expected outcome embedded as `#@ key: value` lines (see the docstring of `run_selftest.py`). |
| `run_selftest.py`  | Meta-runner: starts QSPY, runs the selected `qutest.py` **by explicit path** on every script, and checks verdicts, summary counts, exit status, and output against the expectations, plus the integrity rules I1-I5 (I5: no leaked host executable). |
| `check_runner.py`  | Mutation check of the runner itself (see below): `python3 check_runner.py` (accepts the `run_selftest.py` options, e.g. `--exe`, `--qspy`). |
| `fakeqspy/`        | `fakeqspy.py`: impersonates QSPY *and* a remote (on-hardware) target over UDP, to inject protocol-level faults a real QSPY never produces (lost packets, foreign source; sequence wrap) and to model on-target behavior (reset per test, self-reset after an assertion, target not resetting). Used by scripts with `#@ fakeqspy: yes` (runner starts it on UDP port `--udp`+1). |
| `porttest/`        | `run_porttest.py`: tests of the QS/QUTest **ports** (`ports/posix-qutest`, `ports/win32-qutest`). It impersonates QSPY over TCP and talks directly to the fixture (built on the port under test): exit when QSPY goes away (code 2), graceful close at reset and after an assertion (no TCP reset), no output lost before a reset, exit time. Usage: `python3 -B porttest/run_porttest.py [<fixture-exe>]`. |
| `results/`         | Per-run report (`report.txt`) and raw outputs (not version-controlled). |

## Usage

```
make -C fixture                          # build the fixture (once)
python3 run_selftest.py                  # test ../qutest.py
python3 run_selftest.py --qutest c:/qp/qtools/qutest/qutest.py
python3 run_selftest.py -k st2 -v        # subset, show all problems
python3 run_selftest.py --strict         # KNOWN counts as failure
```

QSPY is taken from `--qspy`, else `$QTOOLS/bin` (Windows) or
`$QTOOLS/bin_linux`. The default UDP/TCP ports (7751/6751) differ from the
QSPY defaults, so a QSPY already running on 7701/6601 is not disturbed.

Every report records the path and SHA-256 of `qutest.py`, QSPY, and the
fixture executable, so each result is traceable to exact artifacts.

## Script naming and statuses

* `stNN_pass_*` / `stNN_fail_*` -- current, correct behavior (must be `OK`).
* `st2N_bug_<ID>_*` -- describe the **correct** behavior for a known defect
  `<ID>` (IDs from the qutest.py review). They report `KNOWN` until the
  defect is fixed. When a fix makes one pass, it reports `FIXED?`, which
  fails the run until the `#@ bug:` tag is removed -- i.e., declaring a
  defect fixed is a deliberate, reviewable edit.
* `#@ decision:` marks an expected behavior that is still an open design
  decision.

Statuses: `OK`, `KNOWN`, `FIXED?`, `FAIL`, `ERROR` (qutest.py hung).
The runner exits non-zero on `FAIL`, `ERROR`, or `FIXED?` (and on `KNOWN`
with `--strict`).

## Rules for fail-path scripts

A fail-path script must pin the **reason** for the failure with an
`#@ output:` expectation (e.g., the expected `exp:`/`got:` lines).
Otherwise the script can pass for an unrelated reason (this happened during
development: a test failed on an unconsumed `Trg-Done` record instead of
on the expectation under test).

Integrity rules checked on every run (independently of the script):
I1 exactly one verdict per test; I2 summary `Tests` equals tests reported;
I3 summary `Skipped` equals `SKIPPED` verdicts; I4 exit status non-zero
iff the summary reports failures; I5 no leaked host executable (see below).
A verdict outside any test (a failure of a whole group, before its first
test) must be FAIL and is allowed only as often as the summary's FAILED list
has `group:<n>` entries (part of I1).

## Leaked host executable (I5)

A host executable (the fixture) that qutest.py leaves running after a
failure lingers and interferes with subsequent tests and runs (e.g.,
"Target reset failed"). Therefore the runner checks after EVERY script that
no fixture process is left (within 2 s after qutest.py exits), fails the
script otherwise (even if it is tagged `#@ bug`), and kills the leaked
process(es). It refuses to start if a fixture process is already running.
qutest.py's output is captured to a file, not a pipe: a leaked process
inherits the output handle, and with a pipe the runner would wait for the
leaked process and hide the leak. The report also records, per script, the
maximum number of fixture processes running at the same time
(`host-exe-max-concurrent`; 2 is expected briefly after an assertion, when
the asserted target is still flushing its output while the next one starts).

## Verification of the runner itself

`check_runner.py` re-checks the runner by mutation (repeatable). It copies
the scripts to a scratch directory, first runs a control (every script used
must be `OK` unmodified), then applies one mutant at a time and verifies the
runner's status, exit code, and (where relevant) output: wrong verdict, test
count, exit status, required output, failed count, skipped count, forbidden
output, verdict and output via fakeqspy, numeric exit status, `#@ bug` on a
passing script (`FIXED?`), `KNOWN` with and without `--strict`, a leaked
host executable (I5, also when bug-tagged), and a stale fixture process
before the run (runner refuses, exit 2), an exceeded `#@ host-max`, and an
exceeded `#@ max-time` (runner 1.6.0, M19).
Result for runner 1.3.0: see the status doc (all mutants must be caught). (The first, manual check of runner 1.0.0 used 7 mutants;
two consecutive full runs produced identical statuses and problem lists.)

## Known limitations and documented behavior of qutest.py

These are properties of qutest.py that the qualification leaves unchanged on
purpose. They are listed so that the evidence can be read correctly. None of
them can turn a failure into a PASS.

* **An error ends the rest of its group.** An error raised by a DSL command
  (e.g., an invalid filter argument), a Python exception in the script, or a
  UDP/protocol error (foreign source, data discontinuity, corrupted Target
  info, "Target reset failed") fails the current test -- or the group, listed
  as `group:<n>` if no test has started -- and the remaining tests of that
  group are neither run nor reported. The run is FAIL, but the summary's
  `Tests` count is lower than the number of tests in the script.
* **On target, a target that does not reset** (no Target info after the
  reset request) makes every following test FAIL by timeout (about 2 s each).
  Unlike the host-executable mode, there is no "Target reset failed" message.
* **Output the target produces on its own after a test has ended** (e.g.,
  from a time event that fires later) is not attributed to that test. The
  end-of-test marker (A7) covers all output up to and including the
  teardown; later output is discarded by the next reset, or is seen by the
  next test if that test is NORESET.
* **Timestamps.** The timestamp counter advances for every expectation that
  starts with `@timestamp` or with a literal 10-digit timestamp. A wildcard
  expectation on a timestamped line (e.g., `"* ECHO 7"`) does not advance it,
  so a later `@timestamp` expectation does not match. This can only cause a
  false FAIL, never a false PASS.
* **Every failed test costs about 1 s**: after a failure, qutest.py discards
  the target's remaining output until the receive timeout
  (`QUTest.TIMEOUT` = 1 s, no command-line option).
* **End of a run**: since M8, `_detach()` no longer sleeps a fixed 1 s after
  detaching from QSPY (st69, `#@ max-time`). The delay protected an OLD
  Windows QSPY (8.1.3 and earlier), which could terminate when it forwarded
  output to a front-end that had closed its socket; QSPY 8.1.6 handles this.
  The "QSPY state issue across front-end sessions" (Linux) was QSPY's 100 KB
  unlicensed data limit (QSPY started without `-l`), not a state issue.
* **Not covered by the self-test suite**: debug mode (`-e debug`, target
  started manually) and interactive mode (`-o i`). Interactive mode without
  any script fails, because the global `QUTest_inst` is not set in that
  path. Neither mode is used for test evidence.
* **Leftover code (harmless)**: the branch `if QSpy.trg_QPver < 720` in
  `QSpy.receive()` dates from QP before 7.2.0 (qutest.py requires QP 8.0.0 or
  newer); it can only trigger for an assertion that arrives before the first
  Target info (`trg_QPver` still 0), where it sends a reset request. The check
  `sys.version_info >= (3,6)` at the end of the file cannot be reached on an
  older Python, which already fails to compile the f-strings. In the color
  definitions, `_COL_GRP1` is commented as yellow but is bright green.

## Coverage of the review findings (status as of creation)

| ID | Script | Status on qutest.py 8.1.6 |
|----|--------|---------------------------|
| A1 exit-status wrap-around | st34 (+ st35 guard; helper groups `scripts/st34_groups/`) | FIXED: exit status = min(failures, 255) |
| A2 skip() inside a test    | st20 (+ st16, st17 guards) | FIXED (regression test) |
| A3 wildcard swallows assertion | st27 (+ st29 guard) | FIXED: "unexpected assertion" unless the pattern contains =ASSERT= (decided) |
| A4 foreign/lost UDP packets | st37, st38, st40 (+ st36, st39 guards; fakeqspy) | FIXED: source-address and sequence checks in QSpy.receive() |
| A5 NORESET after skipped test | st26 (+ st19 guard) | FIXED: FAIL "NORESET test after SKIPPED test" (decided) |
| B1 before-test double count | st22 (+ st13 guard) | FIXED (regression test) |
| B2 ensure() state checks   | st23, st24 (+ st14, st15 guards) | FIXED (regression test) |
| B3 FAIL+PASS for one test  | st25 (+ st18 guard) | FIXED (regression test) |
| B4 no tests executed       | st28 | RESOLVED: zero tests is OK (decided) |
| C1 command() while skipping | st21 | FIXED (regression test) |
| C2 ao_filter() remove by name | st31 (+ st32, st33 guards) | FIXED (regression test) |
| A6 leaked host executable (found 2026-11-06) | st44, st45 (+ on-target guards st41, st42, st43; I5) | FIXED: Popen handle kept; exit awaited (bounded), then terminate/kill; atexit |
| A7 unchecked output after the teardown Ack (found 2026-11-06) | st46, st47, st49 (+ st48, st50 guards) | FIXED: end-of-test marker (Target-info request after on_teardown(); any earlier output FAILS the test) |
| M1 literal timestamp = 9 digits | st51 (guard) | FIXED: exactly 10 digits (could only cause a false FAIL) |
| M2 glb_filter("") IndexError | st52 | FIXED: 'invalid global filter arg=""' |
| M3 numeric filter ranges | st53 | FIXED: local IDs 0..127, global records 0..124 |
| M4 filter-group values vs qs.h | st54 | FIXED: GRP_EQ/GRP_MP and IDS_xxx have the qs.h values |
| M5 timeout message with raw @timestamp | st55 | FIXED: substituted timestamp shown |
| M6 ambiguous FAILED list | st56 (+ st34) | FIXED: group failures listed as `group:<n>` |
| M7 note() in a failed test | st57 | FIXED: no notes after the test failed |
| M8 fixed 1 s delay at the end of every session | st69 (`#@ max-time: 0.6`) | FIXED: no sleep in `_detach()` (requires QSPY 8.1.6 on Windows) |
| Further fail scenarios (added 2026-11-06) | st58 `-ox` exit on first failure; st59 host exe ends mid-test; st60 duplicated UDP packet; st61 corrupted Target info; st62 Target older than QP 8.0.0; st63 too short UDP packet; st64 unknown UDP packet type; st65 QSPY detaches; st66 QSPY not reachable; st67 host exe not found; st68 script not found | all handled correctly by qutest.py (no change needed); st65-st68 are run-level errors (`#@ summary: none`) |
