# QUTest self-test: fail-path (unknown UDP packet type; fakeqspy)
#@ purpose: a UDP packet of an unknown type from QSPY fails the test with "Unrecognized UDP packet type from QSpy"
#@ fakeqspy: yes
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: Unrecognized UDP packet type from QSpy

# fakeqspy command 10 = BAD_PKT 1: packet type 99
test("unknown packet type")
command(10, 1)
expect("@timestamp ECHO 1")
