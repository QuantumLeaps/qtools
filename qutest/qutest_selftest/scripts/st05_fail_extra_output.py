# QUTest self-test: fail-path
#@ purpose: output that is not expected by the end of the test fails the test
#@ verdicts: 1=FAIL
#@ failed: 1
#@ exit: nonzero
#@ output: exp: end-of-test

test("unexpected extra output")
command("CMD_BURST", 2)
expect("@timestamp ECHO 0")
# ECHO 1 and Trg-Done are produced but never expected
