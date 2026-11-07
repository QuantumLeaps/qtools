# QUTest self-test: known defect A7 (unchecked teardown output)
#@ purpose: target output produced after the teardown acknowledgement (by QS_onTestTeardown()) that on_teardown() does not expect FAILS the test
#@ fixed: A7 -- after on_teardown() qutest.py requests the Target info (end-of-test marker); any output arriving before it FAILS the test
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: exp: end-of-test \(no more output\)\ngot: "\d{10} TEARDOWN 9"

# CMD_ARM_TEARDOWN 9: the fixture's QS_onTestTeardown() emits "TEARDOWN 9",
# i.e., AFTER the teardown Ack; nothing in this script expects it
test("teardown output that nobody expects")
command("CMD_ARM_TEARDOWN", 9)
expect("@timestamp ECHO 9")
expect("@timestamp Trg-Done QS_RX_COMMAND")
