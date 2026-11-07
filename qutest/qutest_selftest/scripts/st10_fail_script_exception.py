# QUTest self-test: fail-path
#@ purpose: a Python error in a test script fails the test in which it occurs
#@ verdicts: 1=PASS 2=FAIL
#@ failed: 1
#@ exit: nonzero
#@ output: NameError

test("good")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("script error")
command("CMD_ECHO", 2)
expect("@timestamp ECHO 2")
undefined_name_in_script()   # raises NameError

# NOTE: the remaining tests of this script are not executed (see B4)
test("never reached")
