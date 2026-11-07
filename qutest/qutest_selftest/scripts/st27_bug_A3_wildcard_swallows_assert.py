# QUTest self-test: known defect A3
#@ purpose: an assertion matched only by a wildcard expectation FAILS the test ("unexpected assertion"); the next test recovers
#@ fixed: A3 -- an assertion matched by a pattern without "=ASSERT=" now FAILS ("unexpected assertion")
#@ verdicts: 1=FAIL 2=PASS
#@ tests: 2
#@ failed: 1
#@ exit: nonzero
#@ output: unexpected assertion: "0000000001 =ASSERT= Mod=selftest_fixture,Loc=7"

test("assertion matched only by a wildcard")
command("CMD_ASSERT", 7)
expect("@timestamp *")

test("recovery")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
