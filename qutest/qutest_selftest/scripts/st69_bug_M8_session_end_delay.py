# QUTest self-test: no fixed delay at the end of a session (M8)
#@ purpose: a one-test session ends without a fixed delay (_detach() used to sleep 1 s)
#@ fixed: M8
#@ verdicts: 1=PASS
#@ tests: 1
#@ failed: 0
#@ skipped: 0
#@ exit: zero
#@ max-time: 0.6

test("echo")
command("CMD_ECHO", 7)
expect("@timestamp ECHO 7")
expect("@timestamp Trg-Done QS_RX_COMMAND")
