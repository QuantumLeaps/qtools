# QUTest self-test: minor defect M7
#@ purpose: after a test has failed, its remaining notes (e.g., THEN steps that are no longer executed) are not printed; notes of the next test are
#@ fixed: M7 -- note() does nothing in a failed test
#@ verdicts: 1=FAIL 2=PASS
#@ tests: 2
#@ failed: 1
#@ exit: nonzero
#@ no-output: step that is not executed
#@ output: note of the next test

test("fails, then a note")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 2")
note("THEN: step that is not executed")

test("next test")
note("note of the next test")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
