# QUTest self-test: known defect C1
#@ purpose: command() is ignored while skipping, like every other DSL command
#@ fixed: C1 -- command() now ignored while skipping (qutest.py line 602: if -> elif)
#@ verdicts: 1=SKIPPED 2=PASS
#@ tests: 2
#@ failed: 0
#@ skipped: 1
#@ exit: zero

skip(1)
command("CMD_ECHO", 1)   # ignored while skipping (not "before any test")

test("skipped")
command("CMD_ECHO", 1)

test("runs")
command("CMD_ECHO", 2)
expect("@timestamp ECHO 2")
expect("@timestamp Trg-Done QS_RX_COMMAND")
