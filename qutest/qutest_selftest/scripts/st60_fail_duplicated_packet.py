# QUTest self-test: fail-path (duplicated UDP packet; fakeqspy)
#@ purpose: a duplicated UDP packet from QSPY (the same sequence number twice) is detected by the sequence check and fails the test
#@ fakeqspy: yes
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: UDP data discontinuity from QSPY Seq=\d+->\d+ \(lost packets\)

# fakeqspy command 8 = DUP <p1>: ECHO p1 twice with the same sequence number
test("duplicated packet")
command(8, 7)
expect("@timestamp ECHO 7")
expect("@timestamp Trg-Done QS_RX_COMMAND")
