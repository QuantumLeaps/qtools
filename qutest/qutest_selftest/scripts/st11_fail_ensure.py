# QUTest self-test: fail-path
#@ purpose: ensure() with a false condition fails the test and reports the condition
#@ verdicts: 1=PASS 2=FAIL
#@ failed: 1
#@ exit: nonzero
#@ output: ensure\(last_rec\(\)\.endswith\(" 99"\)\)

test("ensure true")
command("CMD_ECHO", 7)
expect("@timestamp ECHO 7")
ensure(last_rec().endswith(" 7"))
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("ensure false")
command("CMD_ECHO", 7)
expect("@timestamp ECHO 7")
ensure(last_rec().endswith(" 99"))
