# QUTest self-test: fail-path (option -ox, exit on first failure)
#@ purpose: with -ox, qutest.py stops after the first failed test: the rest is not run, the run is FAIL (exit status non-zero), and no host executable is left running
#@ args: -ox
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ host-max: 1
#@ output: Exiting after first failure \(-x\)
#@ no-output: never runs

test("fails")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 2")

test("never runs")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
