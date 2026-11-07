# QUTest self-test: known defect B2
#@ purpose: ensure() inside a skipped test is ignored
#@ fixed: B2 -- ensure() now checks the state: ignored while skipping and in failed/skipped tests
#@ verdicts: 1=SKIPPED 2=PASS
#@ tests: 2
#@ failed: 0
#@ skipped: 1
#@ exit: zero
#@ no-output: ensure\(False\)

skip(1)
test("skipped")
ensure(False)

test("runs")
command("CMD_ECHO", 2)
expect("@timestamp ECHO 2")
expect("@timestamp Trg-Done QS_RX_COMMAND")
