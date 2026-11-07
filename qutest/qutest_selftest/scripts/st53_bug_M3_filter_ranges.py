# QUTest self-test: minor defect M3
#@ purpose: numeric filter ranges: local QS-ID 127 is accepted (IDs 0..127); global record 125 (beyond QS_USER+24) is rejected
#@ fixed: M3 -- numeric ranges: local QS-IDs 0..127, global records 0..124 (others rejected)
#@ verdicts: 1=PASS 2=FAIL
#@ tests: 2
#@ failed: 1
#@ exit: nonzero
#@ output: invalid global filter arg=0x7D

test("local filter ID 127 (last ID of the AP range)")
loc_filter(IDS_ALL, -127)

test("global filter record 125 does not exist")
glb_filter(125)

# (an error raised by a DSL command ends the group, so no recovery test)
