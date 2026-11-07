# QUTest self-test: known defect A2
#@ purpose: skip() must not suppress the remaining expectations of the CURRENT test
#@ fixed: A2 -- skip() inside a running test now takes effect at the next test()
#@ verdicts: 1=FAIL 2=SKIPPED
#@ tests: 2
#@ failed: 1
#@ skipped: 1
#@ exit: nonzero
#@ output: @st20_bug_A2_skip_inside_test.py:17\n.*\nexp: "\S+ ECHO 99"

test("skip() issued while this test is running")
command("CMD_ECHO", 7)
expect("@timestamp ECHO 7")
expect("@timestamp Trg-Done QS_RX_COMMAND")
skip(1)
# the following expectation can never be met and must still be checked
expect("@timestamp ECHO 99")

test("skipped")
command("CMD_ECHO", 1)
