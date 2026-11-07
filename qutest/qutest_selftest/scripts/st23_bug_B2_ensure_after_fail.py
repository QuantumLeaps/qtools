# QUTest self-test: known defect B2
#@ purpose: ensure() in an already failed test does not fail (and count) the test again
#@ fixed: B2 -- ensure() now checks the state: ignored while skipping and in failed/skipped tests
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: exp: "0000000001 ECHO 8"

test("fails, then ensure(False)")
command("CMD_ECHO", 7)
expect("@timestamp ECHO 8")
ensure(False)
