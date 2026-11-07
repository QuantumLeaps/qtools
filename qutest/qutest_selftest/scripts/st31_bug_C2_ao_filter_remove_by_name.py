# QUTest self-test: known defect C2
#@ purpose: ao_filter("-<name>") removes the local filter of the named AO, so the AO's records are suppressed until ao_filter("<name>") adds it again
#@ fixed: C2 -- a string obj_id starting with "-" now means remove (obj_id.startswith('-')), the name is sent without the "-"
#@ verdicts: 1=PASS
#@ tests: 1
#@ failed: 0
#@ exit: 0
#@ no-output: Object Dictionary not found

test("ao_filter() remove and add by name")
command("CMD_AO_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
ao_filter("-AO_Tst")
command("CMD_AO_ECHO", 2)
# ECHO 2 is suppressed by the local filter of AO_Tst
expect("@timestamp Trg-Done QS_RX_COMMAND")
ao_filter("AO_Tst")
command("CMD_AO_ECHO", 3)
expect("@timestamp ECHO 3")
expect("@timestamp Trg-Done QS_RX_COMMAND")
