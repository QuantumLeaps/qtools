# QUTest self-test: pass-path through the fake QSPY (guard for A4)
#@ purpose: the sequence number of the packets from QSPY wraps from 255 to 0 without being reported as a discontinuity
#@ fakeqspy: yes
#@ verdicts: 1=PASS 2=PASS
#@ tests: 2
#@ failed: 0
#@ exit: 0

# fakeqspy command 3 = BURST <p1>: ECHO 0..p1-1, Trg-Done (paced, no loss);
# 300 lines take the sequence number past 255 in the first test, and the
# second test continues from the wrapped value
test("burst of 300 lines (sequence wraps)")
command(3, 300)
for i in range(300):
    expect(f"@timestamp ECHO {i}")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("next test after the wrap")
command(0, 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
