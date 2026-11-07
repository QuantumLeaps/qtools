# QUTest self-test: pass-path (compatibility guard for A3)
#@ purpose: an explicit =ASSERT= expectation with wildcards (as used in the QP suites) still passes
#@ verdicts: 1=PASS 2=PASS
#@ tests: 2
#@ failed: 0
#@ exit: zero
#@ no-output: unexpected assertion

test("explicit assertion, wildcard module")
command("CMD_ASSERT", 7)
expect("@timestamp =ASSERT= Mod=*")

test("explicit assertion, wildcard location")
command("CMD_ASSERT", 8)
expect("@timestamp =ASSERT= Mod=selftest_fixture,Loc=*")
