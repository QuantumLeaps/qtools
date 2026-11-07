# QUTest self-test: fail-path (guard for A1)
#@ purpose: below the saturation limit, the exit status equals the number of failed tests
#@ verdicts: 1=FAIL 2=FAIL 3=FAIL 4=PASS
#@ tests: 4
#@ failed: 3
#@ exit: 3
#@ output: FAILED    : 3 \[

test("failure 1")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 2")

test("failure 2", NORESET)

test("failure 3", NORESET)

test("recovery")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
