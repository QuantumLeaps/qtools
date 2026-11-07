# QUTest self-test: fail-path (Target older than QP 8.0.0; fakeqspy)
#@ purpose: a Target-info packet in the format of QP before 8.0.0 (18 bytes) fails the test with "QP Version Error"
#@ fakeqspy: yes
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: QP Version Error, QP 8.0.0 or newer required

test("Target info of an old QP version")
command(9, 18)
expect("@timestamp ECHO 1")
