# QUTest self-test: fail-path (host executable ends in the middle of a test)
#@ purpose: a host executable that ends unexpectedly in the middle of a test fails that test (timeout); the next test starts a new instance and passes; nothing is left running
#@ verdicts: 1=FAIL 2=PASS
#@ tests: 2
#@ failed: 1
#@ exit: nonzero
#@ host-max: 1
#@ output: host exe ends during the test\n(  @.*\n)+exp: "           Trg-Ack  QS_RX_COMMAND"\ngot: "" \(timeout\)

test("host exe ends during the test")
command("CMD_EXIT", 3)       # the fixture calls exit(3) at once
expect("@timestamp ECHO 1")

test("next test")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
