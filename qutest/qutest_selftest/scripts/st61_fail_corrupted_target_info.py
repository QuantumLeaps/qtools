# QUTest self-test: fail-path (corrupted Target info; fakeqspy)
#@ purpose: a Target-info packet of the wrong length (19 bytes) fails the test with "Corrupted Target info"
#@ fakeqspy: yes
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: Corrupted Target info

# fakeqspy command 9 = BAD_INFO <p1>: Target info of p1 bytes (instead of 20)
test("corrupted Target info")
command(9, 19)
expect("@timestamp ECHO 1")
