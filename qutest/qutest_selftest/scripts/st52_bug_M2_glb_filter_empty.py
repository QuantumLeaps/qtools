# QUTest self-test: minor defect M2
#@ purpose: glb_filter("") fails the test with a clear "invalid global filter" message (not a Python IndexError traceback); like every error raised by a DSL command, it ends the group
#@ fixed: M2 -- glb_filter("") fails with 'invalid global filter arg=""'
#@ verdicts: 1=FAIL
#@ tests: 1
#@ failed: 1
#@ exit: nonzero
#@ output: invalid global filter arg=""
#@ no-output: IndexError

test("empty global filter name")
glb_filter("")

