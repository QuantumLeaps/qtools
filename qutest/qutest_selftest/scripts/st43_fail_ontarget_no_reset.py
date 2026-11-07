# QUTest self-test: on-target mode (fakeqspy as the hardware target)
#@ purpose: on target, a target that does not respond to the reset (no Target info) fails every following test, without hanging
#@ fakeqspy: yes
#@ verdicts: 1=PASS 2=FAIL 3=FAIL
#@ tests: 3
#@ failed: 2
#@ exit: nonzero
#@ output: needs a reset: the target doesn't respond\n(  @.*\n)+exp: "           QF_RUN"\ngot: "" \(timeout\)

test("the target stops responding to resets")
command(5)      # fakeqspy command 5 = DEAF
expect("@timestamp ECHO 0")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("needs a reset: the target doesn't respond")
command(6)
expect("@timestamp RESETS 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("still no reset")
command(6)
expect("@timestamp RESETS 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
