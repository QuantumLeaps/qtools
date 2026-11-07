# QUTest self-test: fail-path (on_teardown)
#@ purpose: a test failed by on_teardown() gets one FAIL verdict and the next test runs and passes normally
#@ verdicts: 1=FAIL 2=PASS
#@ tests: 2
#@ failed: 1
#@ exit: nonzero
#@ output: exp: "\* TEARDOWN 9"

def on_teardown():
    expect("* TEARDOWN 9")

test("teardown output missing (not armed)")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("teardown output present")
command("CMD_ARM_TEARDOWN", 9)
expect("@timestamp ECHO 9")
expect("@timestamp Trg-Done QS_RX_COMMAND")
