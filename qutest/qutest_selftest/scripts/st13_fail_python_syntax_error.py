# QUTest self-test: fail-path
#@ purpose: a Python syntax error in a test script is reported and counted as exactly ONE failure
#@ tests: 0
#@ failed: 1
#@ exit: nonzero
#@ output: SyntaxError

test("never compiled")
command("CMD_ECHO", 1
