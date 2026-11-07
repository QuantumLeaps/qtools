# QUTest self-test: skip-path (compatibility guard for A5)
#@ purpose: skipping a test together with the NORESET test that follows it does not fail anything
#@ verdicts: 1=PASS 2=SKIPPED 3=SKIPPED 4=PASS
#@ tests: 4
#@ failed: 0
#@ skipped: 2
#@ exit: zero
#@ no-output: NORESET test after SKIPPED test

test("runs")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")

skip(2)
test("skipped")
expect("@timestamp THIS-WOULD-FAIL")

test("skipped NORESET continuation", NORESET)
expect("@timestamp THIS-WOULD-FAIL")

test("runs with reset")
command("CMD_ECHO", 4)
expect("@timestamp ECHO 4")
expect("@timestamp Trg-Done QS_RX_COMMAND")
