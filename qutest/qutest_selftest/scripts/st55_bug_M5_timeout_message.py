# QUTest self-test: minor defect M5
#@ purpose: the timeout message shows the expected line with the substituted timestamp (as the mismatch message does), not the raw "@timestamp"
#@ fixed: M5 -- the timeout message shows the substituted timestamp
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: exp: "0000000003 NEVER"\ngot: "" \(timeout\)

test("timeout on a timestamped expectation")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
expect("@timestamp NEVER")    # nothing more comes -> timeout (3rd timestamp)
