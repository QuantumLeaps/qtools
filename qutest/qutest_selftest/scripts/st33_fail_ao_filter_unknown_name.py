# QUTest self-test: fail-path (guard for C2)
#@ purpose: ao_filter() with an unknown AO name fails the test, both for add and for remove; for remove, the "-" prefix is stripped before the name is looked up; the next test recovers
#@ verdicts: 1=FAIL 2=FAIL 3=PASS
#@ tests: 3
#@ failed: 2
#@ exit: nonzero
#@ output: add an unknown AO\n(  @.*\n)+exp: "\s+Trg-Ack  QS_RX_AO_FILTER"\ngot: "\s+<F-END> ERROR\s+Object Dictionary not found for Name=NoSuchAO"
#@ output: remove an unknown AO\n(  @.*\n)+exp: "\s+Trg-Ack  QS_RX_AO_FILTER"\ngot: "\s+<F-END> ERROR\s+Object Dictionary not found for Name=NoSuchAO"
#@ no-output: Name=-NoSuchAO

test("add an unknown AO")
ao_filter("NoSuchAO")

test("remove an unknown AO")
ao_filter("-NoSuchAO")

test("recovery")
command("CMD_AO_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
