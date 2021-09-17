test("Ensure")
command("COMMAND_B", 123, 23456, 3456789)
expect("@timestamp COMMAND_B *")
last = last_rec().split()
p1 = int(last[2])
p2 = int(last[4])

ensure(123 == p1)      # <---
ensure(3456789 == p2)  # <---

