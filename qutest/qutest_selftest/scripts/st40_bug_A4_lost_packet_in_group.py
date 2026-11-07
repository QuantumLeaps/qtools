# QUTest self-test: known defect A4 (lost packet, not the last test)
#@ purpose: a lost packet detected when the NEXT test starts fails the test with the loss (counted once); like every UDP protocol error, it ends the group
#@ fixed: A4 -- receive() checks the continuity of QSPY's sequence numbers (Target-info exempt); a gap raises "UDP data discontinuity"
#@ fakeqspy: yes
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: UDP data discontinuity from QSPY Seq=\d+->\d+ \(lost packets\)
#@ no-output: second test

test("unexpected output lost after the last expectation")
command(1, 8)
expect("@timestamp ECHO 8")
expect("@timestamp Trg-Done QS_RX_COMMAND")

# the gap is seen at the teardown of the first test, i.e., when this test()
# starts; the rest of the group is not executed (RuntimeError)
test("second test")
command(0, 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
