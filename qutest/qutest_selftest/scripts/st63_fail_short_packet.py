# QUTest self-test: fail-path (too short UDP packet; fakeqspy)
#@ purpose: a UDP packet of only 1 byte from QSPY fails the test with "UDP packet from QSpy too short"
#@ fakeqspy: yes
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: UDP packet from QSpy too short

# fakeqspy command 10 = BAD_PKT 0: a 1-byte packet
test("too short packet")
command(10, 0)
expect("@timestamp ECHO 1")
