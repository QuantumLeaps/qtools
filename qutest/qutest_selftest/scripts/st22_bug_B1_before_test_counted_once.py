# QUTest self-test: known defect B1
#@ purpose: a DSL command before any test is reported and counted as ONE failure
#@ fixed: B1 -- _before_test() no longer counts the failure (the SyntaxError handler counts it once)
#@ tests: 0
#@ failed: 1
#@ exit: nonzero
#@ output: "command" before any test

command("CMD_ECHO", 1)

test("not reached")
