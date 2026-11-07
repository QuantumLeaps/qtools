# QUTest self-test: known defect A4 (foreign source)
#@ purpose: a UDP packet that does not come from QSPY (foreign source address/port) fails the test instead of being taken as target output
#@ fixed: A4 -- receive() accepts packets only from the resolved QSPY address; others raise "UDP packet from a foreign source"
#@ fakeqspy: yes
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: UDP packet from a foreign source 127\.0\.0\.1:\d+ \(QSPY is 127\.0\.0\.1:\d+\)

# fakeqspy command 2 = FOREIGN <p1>: the expected lines ECHO p1 and Trg-Done
# are sent from a DIFFERENT UDP socket (with seamless sequence numbers, so
# only the source check can detect them). Without the check the test PASSES
# on output that did not come from QSPY.
test("expected output from a foreign source")
command(2, 9)
expect("@timestamp ECHO 9")
expect("@timestamp Trg-Done QS_RX_COMMAND")
