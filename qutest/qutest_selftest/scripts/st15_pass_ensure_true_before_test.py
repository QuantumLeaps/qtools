# QUTest self-test: pass-path
#@ purpose: a passing ensure() before any test is harmless; its return value is preserved
#@ verdicts: 1=PASS
#@ failed: 0
#@ exit: zero

if not ensure(1 == 1):
    raise RuntimeError("ensure(True) returned False")

test("runs")
command("CMD_ECHO", 1)
expect("@timestamp ECHO 1")
expect("@timestamp Trg-Done QS_RX_COMMAND")
