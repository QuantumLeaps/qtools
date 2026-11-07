# QUTest self-test: known defect A5
#@ purpose: a NORESET test that follows a SKIPPED test FAILS (precondition not established; decided 2026-11-05)
#@ fixed: A5 -- a NORESET test after a SKIPPED test now FAILS ("NORESET test after SKIPPED test")
#@ output: NORESET test after SKIPPED test
#@ failed: 1
#@ exit: nonzero
#@ verdicts: 1=PASS 2=SKIPPED 3=FAIL
#@ tests: 3

test("set variable")
command("CMD_SET_VAR", 5)
expect("@timestamp ECHO 5")
expect("@timestamp Trg-Done QS_RX_COMMAND")

skip(1)
test("skipped (would have changed the variable)")
command("CMD_SET_VAR", 6)
expect("@timestamp ECHO 6")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("NORESET after a skipped test", NORESET)
command("CMD_GET_VAR")
expect("@timestamp ECHO 5")
expect("@timestamp Trg-Done QS_RX_COMMAND")
