# QUTest self-test: known defect A7 (misattributed teardown output)
#@ purpose: unexpected teardown output fails the test that PRODUCED it; a following NORESET test then fails as "NORESET test after FAILED test", not on the leftover output
#@ fixed: A7 -- after on_teardown() qutest.py requests the Target info (end-of-test marker); any output arriving before it FAILS the test
#@ verdicts: 1=FAIL 2=FAIL
#@ tests: 2
#@ failed: 2
#@ exit: nonzero
#@ output: exp: end-of-test \(no more output\)\ngot: "\d{10} TEARDOWN 9"
#@ output: NORESET test after it\n(  @.*\n)*NORESET test after FAILED test

test("teardown output that nobody expects")
command("CMD_ARM_TEARDOWN", 9)
expect("@timestamp ECHO 9")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("NORESET test after it", NORESET)
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
