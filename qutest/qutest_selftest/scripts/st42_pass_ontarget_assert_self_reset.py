# QUTest self-test: on-target mode (fakeqspy as the hardware target)
#@ purpose: on target, after an (expected) assertion the target resets ITSELF; qutest.py must not send a reset for the next test, but must accept the target's own reset (Target info + QF_RUN); afterwards the normal reset per test resumes
#@ fakeqspy: yes
#@ verdicts: 1=PASS 2=PASS 3=PASS
#@ tests: 3
#@ failed: 0
#@ exit: 0

test("expected assertion; the target resets itself")
command(6)
expect("@timestamp RESETS 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
command(4, 7)   # fakeqspy command 4 = ASSERT, then self-reset
expect("@timestamp =ASSERT= Mod=fakeqspy,Loc=7")

test("after the assertion: no reset was sent")
command(6)
expect("@timestamp RESETS 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("normal reset per test resumes")
command(6)
expect("@timestamp RESETS 2")
expect("@timestamp Trg-Done QS_RX_COMMAND")
