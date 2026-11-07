# QUTest self-test: pass-path (expected assertion)
#@ purpose: an explicitly expected target assertion passes; the next test gets a fresh target
#@ verdicts: 1=PASS 2=PASS
#@ failed: 0
#@ exit: zero

test("expected assertion")
command("CMD_ASSERT", 42)
expect("@timestamp =ASSERT= Mod=selftest_fixture,Loc=42")

test("target recovered after assertion")
command("CMD_ECHO", 3)
expect("@timestamp ECHO 3")
expect("@timestamp Trg-Done QS_RX_COMMAND")
