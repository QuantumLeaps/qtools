# QUTest self-test: known defect A4 (lost packet)
#@ purpose: a packet lost between QSPY and qutest.py (a gap in QSPY's sequence numbers) fails the test, even when the lost packet was unexpected output after the last expectation
#@ fixed: A4 -- receive() checks the continuity of QSPY's sequence numbers (Target-info exempt); a gap raises "UDP data discontinuity"
#@ fakeqspy: yes
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: UDP data discontinuity from QSPY Seq=\d+->\d+ \(lost packets\)

# fakeqspy command 1 = LOSE_EXTRA <p1>: ECHO p1, Trg-Done, then an extra
# (unexpected) line whose packet is LOST. Without a sequence check the test
# would PASS, although the target produced output that was never verified.
# The gap is seen at the teardown of this (last) test of the group.
test("unexpected output lost after the last expectation")
command(1, 8)
expect("@timestamp ECHO 8")
expect("@timestamp Trg-Done QS_RX_COMMAND")
