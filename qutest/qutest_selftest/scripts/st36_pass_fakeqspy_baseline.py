# QUTest self-test: pass-path through the fake QSPY (baseline for A4)
#@ purpose: qutest.py works against fakeqspy (remote target): normal tests pass; the Target-info packet (which carries the TARGET's sequence number) does not break the sequence check
#@ fakeqspy: yes
#@ verdicts: 1=PASS 2=PASS 3=PASS
#@ tests: 3
#@ failed: 0
#@ exit: 0

# fakeqspy command 0 = ECHO <p1>
test("echo")
command(0, 7)
expect("@timestamp ECHO 7")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("echo after another reset (another Target-info packet)")
command(0, 8)
expect("@timestamp ECHO 8")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("NORESET echo", NORESET)
command(0, 9)
expect("@timestamp ECHO 9")
expect("@timestamp Trg-Done QS_RX_COMMAND")
