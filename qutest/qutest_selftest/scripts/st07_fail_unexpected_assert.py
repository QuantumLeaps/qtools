# QUTest self-test: fail-path
#@ purpose: an unexpected target assertion fails the test; the next test recovers
#@ verdicts: 1=FAIL 2=PASS
#@ failed: 1
#@ exit: nonzero
#@ output: got: "0000000001 =ASSERT= Mod=selftest_fixture,Loc=42"

test("unexpected assertion")
command("CMD_ASSERT", 42)
expect("@timestamp ECHO 42")

test("recovery")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
