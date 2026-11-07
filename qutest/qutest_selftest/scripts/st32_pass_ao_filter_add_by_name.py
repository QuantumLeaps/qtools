# QUTest self-test: pass-path (guard for C2)
#@ purpose: the fixture's AO filter mechanism works independently of C2: loc_filter() removes the AO's prio, ao_filter("<name>") adds it again, a reset restores it
#@ verdicts: 1=PASS 2=PASS 3=PASS
#@ tests: 3
#@ failed: 0
#@ exit: 0

test("loc_filter(-prio) suppresses, ao_filter(name) re-enables")
loc_filter(IDS_ALL, -1)   # AO_Tst has prio 1
command("CMD_AO_ECHO", 1)
# ECHO 1 is suppressed by the local filter of AO_Tst
expect("@timestamp Trg-Done QS_RX_COMMAND")
ao_filter("AO_Tst")
command("CMD_AO_ECHO", 2)
expect("@timestamp ECHO 2")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("reset restores the local filter")
loc_filter(IDS_ALL, -1)
test("after reset")
command("CMD_AO_ECHO", 3)
expect("@timestamp ECHO 3")
expect("@timestamp Trg-Done QS_RX_COMMAND")
