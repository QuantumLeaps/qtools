# QUTest self-test: known defect A1
#@ purpose: a run with 256 failures exits with a non-zero status (saturated at 255), never with 0
#@ fixed: A1 -- the exit status is min(failures, 255), so it can't wrap to 0 on POSIX (was sys.exit(_num_failed))
#@ args: st34_groups/g*.py
#@ verdicts: 1=PASS
#@ tests: 1
#@ failed: 256
#@ exit: 255
#@ output: Groups    : 257
#@ output: FAILED    : 256 \[ group:1 group:2 group:3 .* group:255 group:256 \]

# The 256 failures come from the 256 helper groups st34_groups/g000..g255.py
# (run before this script). Each helper issues a DSL command before any test,
# which fails its group at once and is counted as ONE failure (see B1).
# (A failing TEST costs ~1 s, because _fail() drains the input until the
# receive timeout, so 256 failing tests would take more than 4 minutes.)
#
# This script itself contains one passing test, to show that the target
# works and that only the 256 group failures are counted.
test("the target works")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
