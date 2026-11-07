# QUTest self-test: on-target mode (fakeqspy as the hardware target)
#@ purpose: on target (no -e), qutest.py sends exactly one TO_TRG_RESET before every test that is not NORESET, and none before a NORESET test
#@ fakeqspy: yes
#@ verdicts: 1=PASS 2=PASS 3=PASS 4=PASS
#@ tests: 4
#@ failed: 0
#@ exit: 0

# fakeqspy command 6 = RESETS: reports the number of TO_TRG_RESET requests
test("first test: one reset")
command(6)
expect("@timestamp RESETS 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("second test: one more reset")
command(6)
expect("@timestamp RESETS 2")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("NORESET test: no reset", NORESET)
command(6)
expect("@timestamp RESETS 2")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("normal test again: one more reset")
command(6)
expect("@timestamp RESETS 3")
expect("@timestamp Trg-Done QS_RX_COMMAND")
