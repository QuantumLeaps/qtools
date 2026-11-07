# QUTest self-test: skip-path
#@ purpose: skip(2) issued inside a test skips exactly the next two tests
#@ verdicts: 1=PASS 2=SKIPPED 3=SKIPPED 4=PASS
#@ tests: 4
#@ failed: 0
#@ skipped: 2
#@ exit: zero

test("issues skip(2)")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
skip(2)
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("skipped 1")
expect("@timestamp THIS-WOULD-FAIL")

test("skipped 2")
expect("@timestamp THIS-WOULD-FAIL")

test("runs")
command("CMD_ECHO", 4)
expect("@timestamp ECHO 4")
expect("@timestamp Trg-Done QS_RX_COMMAND")
