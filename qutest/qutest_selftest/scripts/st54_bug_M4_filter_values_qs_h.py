# QUTest self-test: minor defect M4
#@ purpose: the filter-group values are the same as in qs.h, so loc_filter(-0xF1) (QS_IDS_AO in qs.h) removes the AO IDs exactly like loc_filter(-IDS_AO)
#@ fixed: M4 -- GRP_EQ/GRP_MP and IDS_AO/EP/EQ/AP have the qs.h values (QS_GRP_xxx, QS_IDS_xxx)
#@ verdicts: 1=PASS 2=PASS
#@ tests: 2
#@ failed: 0
#@ exit: zero

# AO_Tst has prio 1 (an AO ID); CMD_AO_ECHO is subject to its local filter
test("by name: IDS_AO")
loc_filter(IDS_ALL, -IDS_AO)
command("CMD_AO_ECHO", 1)
expect("@timestamp Trg-Done QS_RX_COMMAND")   # ECHO 1 suppressed

test("by the qs.h value: 0xF1 (QS_IDS_AO)")
loc_filter(IDS_ALL, -0xF1)
command("CMD_AO_ECHO", 2)
expect("@timestamp Trg-Done QS_RX_COMMAND")   # ECHO 2 suppressed
