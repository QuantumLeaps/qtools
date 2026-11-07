# QUTest self-test: known defect B3
#@ purpose: a failure inside on_teardown() yields exactly one verdict (FAIL), not FAIL followed by PASS
#@ fixed: B3 -- _test_end() no longer reports PASS after on_teardown() failed the test
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: exp: "\* TEARDOWN 9"

def on_teardown():
    expect("* TEARDOWN 9")   # never produced (teardown output not armed)

test("teardown expectation fails")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
