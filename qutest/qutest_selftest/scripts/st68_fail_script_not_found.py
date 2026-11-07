# QUTest self-test: run-level error (test script not found)
#@ purpose: a test script that does not exist is reported and the run ends with a non-zero exit status, without running any test (no SUMMARY)
#@ args: no_such_script.py
#@ summary: none
#@ exit: nonzero
#@ host-max: 0
#@ output: Test script not found: no_such_script\.py

test("never runs")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
