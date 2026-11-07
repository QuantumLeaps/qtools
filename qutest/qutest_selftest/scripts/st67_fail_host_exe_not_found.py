# QUTest self-test: run-level error (host executable not found)
#@ purpose: a host executable that does not exist is reported and the run ends with a non-zero exit status, without attaching to QSPY or running any test (no SUMMARY)
#@ args: -eno_such_fixture
#@ summary: none
#@ exit: nonzero
#@ host-max: 0
#@ output: Provided test executable 'no_such_fixture' not found
#@ no-output: Attaching to QSpy

test("never runs")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
