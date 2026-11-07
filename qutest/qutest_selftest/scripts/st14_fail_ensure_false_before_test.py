# QUTest self-test: fail-path
#@ purpose: a failing ensure() before any test is reported as an error before any test, counted once
#@ tests: 0
#@ failed: 1
#@ exit: nonzero
#@ output: "ensure" before any test

ensure(1 == 2)

test("not reached")
