# QUTest self-test: known defect A7, on target (fakeqspy as the hardware target)
#@ purpose: on target too, output after the teardown acknowledgement that on_teardown() does not expect FAILS the test; the next (reset) test passes
#@ fixed: A7 -- after on_teardown() qutest.py requests the Target info (end-of-test marker); any output arriving before it FAILS the test
#@ fakeqspy: yes
#@ verdicts: 1=FAIL 2=PASS
#@ tests: 2
#@ failed: 1
#@ exit: nonzero
#@ output: exp: end-of-test \(no more output\)\ngot: "\d{10} TEARDOWN 5"

# fakeqspy command 7 = ARM_TEARDOWN <p1>: "TEARDOWN p1" after the teardown Ack
test("on target: teardown output that nobody expects")
command(7, 5)
expect("@timestamp ECHO 5")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("next test after a reset")
command(0, 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
