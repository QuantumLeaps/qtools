# QUTest self-test: zero-test run (former review item B4)
#@ purpose: a run in which no test is executed reports OK (decided 2026-11-05: zero tests is OK)
#@ resolved: B4 -- decision: a run with zero tests is OK (not a failure)
#@ tests: 0
#@ failed: 0
#@ exit: zero

note("this script contains no test() at all")
