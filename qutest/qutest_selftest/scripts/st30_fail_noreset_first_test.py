# QUTest self-test: fail-path
#@ purpose: a NORESET test as the first test of a group fails (target was never reset)
#@ verdicts: 1=FAIL 2=PASS
#@ tests: 2
#@ failed: 1
#@ exit: nonzero
#@ output: NORESET test needs reset

test("first test, but NORESET", NORESET)
command("CMD_ECHO", 1)

test("normal test")
command("CMD_ECHO", 2)
expect("@timestamp ECHO 2")
expect("@timestamp Trg-Done QS_RX_COMMAND")
