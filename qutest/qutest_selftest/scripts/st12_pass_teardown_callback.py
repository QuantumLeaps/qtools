# QUTest self-test: pass-path (on_teardown callback)
#@ purpose: on_teardown() can verify output the target produces during teardown
#@ verdicts: 1=PASS
#@ failed: 0
#@ exit: zero

def on_teardown():
    expect("* TEARDOWN 9")

test("teardown output checked by on_teardown")
command("CMD_ARM_TEARDOWN", 9)
expect("@timestamp ECHO 9")
expect("@timestamp Trg-Done QS_RX_COMMAND")
