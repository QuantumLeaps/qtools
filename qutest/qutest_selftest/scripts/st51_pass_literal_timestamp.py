# QUTest self-test: pass-path (guard for M1)
#@ purpose: an expectation with a literal 10-digit timestamp advances the timestamp counter like @timestamp, so later @timestamp expectations stay in step
#@ verdicts: 1=PASS
#@ tests: 1
#@ failed: 0
#@ exit: zero

test("literal timestamp, then @timestamp")
command("CMD_ECHO", 7)
expect("0000000001 ECHO 7")
expect("@timestamp Trg-Done QS_RX_COMMAND")
