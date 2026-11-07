# QUTest self-test: pass-path sanity (reset vs. NORESET)
#@ purpose: NORESET test sees the target state of the previous test; a normal test resets it
#@ verdicts: 1=PASS 2=PASS 3=PASS
#@ tests: 3
#@ failed: 0
#@ exit: zero

test("set variable")
command("CMD_SET_VAR", 5)
expect("@timestamp ECHO 5")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("NORESET keeps the variable", NORESET)
command("CMD_GET_VAR")
expect("@timestamp ECHO 5")
expect("@timestamp Trg-Done QS_RX_COMMAND")

test("reset clears the variable")
command("CMD_GET_VAR")
expect("@timestamp ECHO 0")
expect("@timestamp Trg-Done QS_RX_COMMAND")
