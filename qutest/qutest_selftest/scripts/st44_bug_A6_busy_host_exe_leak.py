# QUTest self-test: known defect A6 (leaked host executable)
#@ purpose: a host executable that does not respond to the reset (busy) is terminated by qutest.py before the next one is started: it does not leak and does not disturb the next test
#@ fixed: A6 -- qutest.py keeps the Popen handle and waits for the host exe to exit (bounded), then terminates/kills it; no leak, no interference
#@ verdicts: 1=FAIL 2=PASS
#@ tests: 2
#@ failed: 1
#@ exit: nonzero
#@ host-max: 1
#@ output: target busy for 30 s -> timeout\n(  @.*\n)+exp: "           Trg-Ack  QS_RX_COMMAND"\ngot: "" \(timeout\)
#@ no-output: Target reset failed

# NOTE: a leak of the host executable is also caught by the runner's
# integrity rule I5, which makes this script FAIL (not KNOWN) until fixed.
test("target busy for 30 s -> timeout")
command("CMD_DELAY_ECHO", 30000, 1)   # the fixture sleeps 30 s (busy)
expect("@timestamp ECHO 1")

test("next test: the busy exe must be gone, the new one must work")
command("CMD_ECHO", 2)
expect("@timestamp ECHO 2")
expect("@timestamp Trg-Done QS_RX_COMMAND")
