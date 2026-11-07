# QUTest self-test: run-level error (QSPY detaches; fakeqspy)
#@ purpose: when QSPY detaches the front-end during a run, qutest.py reports "QSPY detached" and ends the run immediately with a non-zero exit status (no SUMMARY)
#@ fakeqspy: yes
#@ summary: none
#@ exit: nonzero
#@ output: QSPY detached
#@ no-output: after the detach

# fakeqspy command 11 = DETACH
test("QSPY detaches")
command(11)
expect("@timestamp ECHO 1")

test("after the detach")
command(0, 1)
expect("@timestamp ECHO 1")
