# QUTest self-test: fail-path
#@ purpose: an expectation that is never satisfied (no output) fails with a timeout
#@ verdicts: 1=FAIL
#@ failed: 1
#@ exit: nonzero
#@ output: got: "" \(timeout\)

test("missing output")
command("CMD_SILENT")
expect("@timestamp Trg-Done QS_RX_COMMAND")
expect("@timestamp ECHO 1")
