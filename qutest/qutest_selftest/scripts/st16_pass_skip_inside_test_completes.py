# QUTest self-test: skip-path
#@ purpose: a test that calls skip() still completes normally (expectations and on_teardown checked); the NEXT test is skipped
#@ verdicts: 1=PASS 2=SKIPPED 3=PASS
#@ tests: 3
#@ failed: 0
#@ skipped: 1
#@ exit: zero

def on_teardown():
    expect("* TEARDOWN 9")   # must still be checked for the test calling skip()

test("calls skip() while running")
command("CMD_ARM_TEARDOWN", 9)
skip(1)
expect("@timestamp ECHO 9")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("skipped")
command("CMD_ECHO", 2)
expect("@timestamp THIS-WOULD-FAIL")

test("runs again (teardown not armed: target was reset)")
command("CMD_ARM_TEARDOWN", 9)
expect("@timestamp ECHO 9")
expect("@timestamp Trg-Done QS_RX_COMMAND")
