# QUTest self-test: pass-path (guard for A7)
#@ purpose: teardown output that on_teardown() expects passes, also for a NORESET test that follows (the end-of-test check must not disturb the NORESET chain)
#@ verdicts: 1=PASS 2=PASS
#@ tests: 2
#@ failed: 0
#@ exit: zero

def on_teardown():
    expect("@timestamp TEARDOWN 9")

test("expected teardown output")
command("CMD_ARM_TEARDOWN", 9)      # stays armed for the NORESET test
expect("@timestamp ECHO 9")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("NORESET test with expected teardown output", NORESET)
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
