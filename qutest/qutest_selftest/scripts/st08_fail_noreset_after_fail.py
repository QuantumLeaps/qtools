# QUTest self-test: fail-path
#@ purpose: a NORESET test that follows a failed test fails (precondition not established)
#@ verdicts: 1=FAIL 2=FAIL
#@ failed: 2
#@ exit: nonzero
#@ output: NORESET test after FAILED test

test("failing")
command("CMD_ECHO", 7)
expect("@timestamp ECHO 8")

test("NORESET after failure", NORESET)
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
