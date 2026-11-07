# QUTest self-test: on-target pass-path (guard for A7)
#@ purpose: on target, teardown output that on_teardown() expects passes (the end-of-test Target-info request is answered after it)
#@ fakeqspy: yes
#@ verdicts: 1=PASS
#@ tests: 1
#@ failed: 0
#@ exit: zero

def on_teardown():
    expect("@timestamp TEARDOWN 5")

test("on target: expected teardown output")
command(7, 5)
expect("@timestamp ECHO 5")
expect("@timestamp Trg-Done QS_RX_COMMAND")
