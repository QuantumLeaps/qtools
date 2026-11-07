# QUTest self-test: minor defect M6
#@ purpose: the FAILED list in the summary is unambiguous: failures of a whole group (before any test) appear as "group:<n>", failed tests by their (run-wide unique) test number
#@ fixed: M6 -- group failures are listed as "group:<n>" in the FAILED list
#@ args: st56_groups/g*.py
#@ verdicts: 1=PASS 2=FAIL
#@ tests: 2
#@ failed: 3
#@ exit: nonzero
#@ output: FAILED    : 3 \[ group:1 group:2 2 \]

# groups 1 and 2 are the helpers st56_groups/g1.py (DSL command before any
# test) and st56_groups/g2.py (Python error before any test); this is group 3
test("passes")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("fails")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 2")
