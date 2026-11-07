# QUTest self-test: skip-path sanity
#@ purpose: skip(1) between tests skips exactly the next test, whose (wrong) expectations are not run
#@ verdicts: 1=PASS 2=SKIPPED 3=PASS
#@ tests: 3
#@ failed: 0
#@ skipped: 1
#@ exit: zero

test("before the skip")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")

skip(1)
test("skipped")
command("CMD_ECHO", 2)
expect("@timestamp THIS-WOULD-FAIL")

test("after the skip")
command("CMD_ECHO", 3)
expect("@timestamp ECHO 3")
expect("@timestamp Trg-Done QS_RX_COMMAND")
