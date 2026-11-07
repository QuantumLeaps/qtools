# QUTest self-test: fail-path
#@ purpose: output that differs from the expectation fails the test; the next test recovers
#@ verdicts: 1=FAIL 2=PASS
#@ tests: 2
#@ failed: 1
#@ exit: nonzero
#@ output: exp: "0000000001 ECHO 8"
#@ output: got: "0000000001 ECHO 7"

test("mismatch")
command("CMD_ECHO", 7)
expect("@timestamp ECHO 8")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("recovery")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
