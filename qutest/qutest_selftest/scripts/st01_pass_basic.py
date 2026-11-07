# QUTest self-test: pass-path sanity
#@ purpose: a command whose output matches the expectations passes
#@ verdicts: 1=PASS
#@ tests: 1
#@ failed: 0
#@ skipped: 0
#@ exit: zero

test("echo")
command("CMD_ECHO", 7)
expect("@timestamp ECHO 7")
expect("@timestamp Trg-Done QS_RX_COMMAND")
