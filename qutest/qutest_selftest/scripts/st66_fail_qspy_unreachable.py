# QUTest self-test: run-level error (QSPY not reachable)
#@ purpose: when no QSPY answers at the given address, qutest.py reports the failed attach and ends with a non-zero exit status without running any test (no SUMMARY); no host executable is started
#@ args: -qlocalhost:7799:6799
#@ summary: none
#@ exit: nonzero
#@ host-max: 0
#@ output: Attaching to QSpy \(localhost:7799\)\.\.\. FAILED

# the "-q" in #@ args (no QSPY at port 7799) overrides the runner's -q
test("never runs")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
