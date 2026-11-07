# QUTest self-test: known defect A6 (host executable after an assertion)
#@ purpose: after an expected assertion, the asserted host executable has exited (it exits by itself) BEFORE qutest.py starts the next one: never two host executables at the same time
#@ fixed: A6 -- after an assertion qutest.py waits for the asserted host exe to exit (it exits by itself) before starting the next one
#@ verdicts: 1=PASS 2=PASS 3=PASS
#@ tests: 3
#@ failed: 0
#@ exit: zero
#@ host-max: 1

test("expected assertion")
command("CMD_ASSERT", 42)
expect("@timestamp =ASSERT= Mod=selftest_fixture,Loc=42")

test("expected assertion again")
command("CMD_ASSERT", 43)
expect("@timestamp =ASSERT= Mod=selftest_fixture,Loc=43")

test("target recovered after the assertions")
command("CMD_ECHO", 3)
expect("@timestamp ECHO 3")
expect("@timestamp Trg-Done QS_RX_COMMAND")
