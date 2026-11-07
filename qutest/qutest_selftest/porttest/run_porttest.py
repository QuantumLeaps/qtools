#!/usr/bin/env python3
# QS/QUTest port tests (ports/posix-qutest, ports/win32-qutest)
#
# This script impersonates QSPY on the TCP side and talks DIRECTLY to a host
# test executable built on the port under test (the qutest_selftest fixture),
# so that it can observe how the executable ends its connection:
#   - graceful close (FIN) vs. abortive close (RST, which can discard data),
#   - no trace data lost at a reset or after an assertion,
#   - the executable exits when QSPY goes away (no leaked, spinning process),
#   - how long the executable takes to exit.
#
# usage: python3 -B run_porttest.py [<fixture-executable>] [-k <id-prefix>]
import sys, os, socket, struct, subprocess, time, hashlib, platform, argparse
import datetime, statistics

RUNNER_VERSION = "0.1.0"
HERE = os.path.dirname(os.path.abspath(__file__))
IS_WIN = (os.name == "nt")

# QS protocol (see qs_pkg.h, qpc_qs.h)
QS_FRAME, QS_ESC, QS_ESC_XOR = 0x7E, 0x7D, 0x20
QS_TARGET_INFO, QS_ASSERT_FAIL, QS_QF_RUN = 64, 69, 70
ECHO = 100                                # fixture user record
RX_INFO, RX_COMMAND, RX_RESET, RX_GLB_FILTER = 0, 1, 2, 10
CMD_BURST, CMD_ASSERT = 1, 3              # fixture commands

def rx_frame(seq, payload):
    """frame for the Target's QS-RX channel"""
    body = bytes([seq & 0xFF]) + payload
    ck = (~sum(body)) & 0xFF
    out = bytearray()
    for b in body + bytes([ck]):
        out += bytes([QS_ESC, b ^ QS_ESC_XOR]) if b in (QS_FRAME, QS_ESC) \
               else bytes([b])
    return bytes(out + bytes([QS_FRAME]))

class Records:
    """decoder of the Target's QS-TX stream (frames, checksum, sequence)"""
    def __init__(self):
        self.recs = []; self.bad = 0; self.gaps = 0
        self._buf = bytearray(); self._esc = False; self._seq = None
    def feed(self, data):
        for b in data:
            if self._esc:
                self._buf.append(b ^ QS_ESC_XOR); self._esc = False
            elif b == QS_ESC:
                self._esc = True
            elif b == QS_FRAME:
                f = bytes(self._buf); self._buf.clear()
                if len(f) >= 3 and (sum(f) & 0xFF) == 0xFF:
                    seq, rec = f[0], f[1]
                    if self._seq is not None and seq != ((self._seq + 1) & 0xFF):
                        self.gaps += 1
                    self._seq = seq
                    self.recs.append((rec, f[2:-1]))
                elif len(f) > 0:
                    self.bad += 1
            else:
                self._buf.append(b)
    def of(self, rec):
        return [p for (r, p) in self.recs if r == rec]

class Fixture:
    """the host executable under test, connected to our fake QSPY"""
    def __init__(self, ctx):
        self.srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.srv.bind(("127.0.0.1", 0))
        self.srv.listen(1)
        port = self.srv.getsockname()[1]
        self.proc = subprocess.Popen([ctx.exe, f"127.0.0.1:{port}"],
                                     stdin=subprocess.DEVNULL,
                                     stdout=subprocess.DEVNULL,
                                     stderr=subprocess.PIPE)
        self.srv.settimeout(5.0)
        self.sock, _ = self.srv.accept()
        self.rx = Records()
        self.tx_seq = 0
        self.end = None   # "FIN", "RST" or None
        # wait for the Target to be in its test loop (QF_RUN)
        self.read_until(lambda: self.rx.of(QS_QF_RUN), 5.0)
    def send(self, *payloads):
        data = b""
        for p in payloads:
            self.tx_seq += 1
            data += rx_frame(self.tx_seq, p)
        self.sock.sendall(data)
    def read_until(self, cond, timeout):
        """read the Target output until cond() or the end of connection"""
        t_end = time.time() + timeout
        while not cond() and self.end is None and time.time() < t_end:
            self.sock.settimeout(max(0.01, t_end - time.time()))
            try:
                d = self.sock.recv(65536)
            except socket.timeout:
                break
            except (ConnectionResetError, ConnectionAbortedError):
                self.end = "RST"; break
            if not d:
                # the Target has ended its stream: like QSPY, close our
                # side of the connection right away
                self.end = "FIN"
                try: self.sock.close()
                except OSError: pass
                break
            self.rx.feed(d)
    def read_to_end(self, timeout=5.0):
        self.read_until(lambda: False, timeout)
    def wait_exit(self, timeout=5.0):
        try:
            return self.proc.wait(timeout)
        except subprocess.TimeoutExpired:
            return None
    def close(self):
        if self.proc.poll() is None:
            self.proc.kill(); self.proc.wait()
        err = self.proc.stderr.read().decode("utf-8", "replace")
        self.proc.stderr.close()
        for s in (self.sock, self.srv):  # (may be closed already)
            try: s.close()
            except OSError: pass
        return err

def glb_all():
    return bytes([RX_GLB_FILTER, 16]) + b"\xFF" * 16
def cmd(cmd_id, p1=0, p2=0, p3=0):
    return bytes([RX_COMMAND, cmd_id]) + struct.pack("<III", p1, p2, p3)

#=============================================================================
CASES = []
def case(cid, desc):
    def deco(fn):
        CASES.append((cid, desc, fn)); return fn
    return deco

@case("PT-01", "QSPY goes away: the executable reports it and exits with "
               "code 2 (no leaked, spinning process)")
def _pt01(ctx):
    f = Fixture(ctx)
    f.sock.close()                    # QSPY closes the connection
    code = f.wait_exit(3.0)
    err = f.close()
    msg = "QSPY connection lost" in err
    ok = (code == 2) and msg
    return ok, f"exit code={code}, stderr message={msg}"

def reset_like(ctx, trigger, n_burst=0):
    """trigger + an extra (unread) frame shortly after; measure the exit"""
    f = Fixture(ctx)
    f.send(glb_all())
    f.read_until(lambda: False, 0.1)
    pre = [cmd(CMD_BURST, n_burst)] if n_burst else []
    t0 = time.perf_counter()
    f.send(*(pre + [trigger]))
    time.sleep(0.003)
    try:
        f.send(bytes([RX_INFO]))      # arrives while the Target is exiting
    except OSError:
        pass
    f.read_to_end()
    code = f.wait_exit()
    dt = (time.perf_counter() - t0) * 1000
    f.close()
    return f, code, dt

@case("PT-02", "reset: graceful close (FIN, no RST), exit code 0")
def _pt02(ctx):
    f, code, dt = reset_like(ctx, bytes([RX_RESET]))
    ok = (f.end == "FIN") and (code == 0)
    return ok, f"end={f.end}, exit code={code}, {dt:.1f} ms"

@case("PT-03", "assertion: assertion record received, graceful close (FIN), "
               "exit code 0")
def _pt03(ctx):
    f, code, dt = reset_like(ctx, cmd(CMD_ASSERT, 7))
    a = f.rx.of(QS_ASSERT_FAIL)
    ok = bool(a) and (f.end == "FIN") and (code == 0)
    return ok, (f"assert record={bool(a)}, end={f.end}, exit code={code}, "
                f"{dt:.1f} ms")

@case("PT-04", "output produced right before a reset is not lost "
               "(burst + reset in one packet)")
def _pt04(ctx):
    n = 200
    f, code, dt = reset_like(ctx, bytes([RX_RESET]), n_burst=n)
    # ECHO payload: timestamp, format byte, uint32 value (the last 4 bytes)
    vals = [struct.unpack("<I", p[-4:])[0] for p in f.rx.of(ECHO)
            if len(p) >= 5]
    ok = (vals == list(range(n))) and f.rx.gaps == 0 and f.rx.bad == 0 \
         and f.end == "FIN"
    return ok, (f"ECHO records {len(vals)}/{n}, gaps={f.rx.gaps}, "
                f"bad={f.rx.bad}, end={f.end}")

@case("PT-05", "exit time after an assertion is short (median of 10 < 50 ms)")
def _pt05(ctx):
    dts = []
    for _ in range(10):
        f, code, dt = reset_like(ctx, cmd(CMD_ASSERT, 7))
        dts.append(dt)
    med = statistics.median(dts)
    return med < 50.0, (f"median {med:.1f} ms, min {min(dts):.1f}, "
                        f"max {max(dts):.1f}")

@case("PT-06", "exit time after a reset is short (median of 10 < 50 ms)")
def _pt06(ctx):
    dts = []
    for _ in range(10):
        f, code, dt = reset_like(ctx, bytes([RX_RESET]))
        dts.append(dt)
    med = statistics.median(dts)
    return med < 50.0, (f"median {med:.1f} ms, min {min(dts):.1f}, "
                        f"max {max(dts):.1f}")

#=============================================================================
def main():
    ap = argparse.ArgumentParser()
    default_exe = os.path.join(HERE, "..", "fixture", "build",
                               "selftest_fixture" + (".exe" if IS_WIN else ""))
    ap.add_argument("exe", nargs="?", default=default_exe)
    ap.add_argument("-k", dest="select", default="")
    a = ap.parse_args()
    class Ctx: pass
    ctx = Ctx(); ctx.exe = os.path.abspath(a.exe)
    with open(ctx.exe, "rb") as fexe:
        sha = hashlib.sha256(fexe.read()).hexdigest()
    print("QS/QUTest port tests")
    print(f"runner  : run_porttest.py {RUNNER_VERSION}")
    print(f"date    : {datetime.datetime.now().strftime('%y%m%d_%H%M%S')}")
    print(f"platform: {platform.platform()} / Python {platform.python_version()}")
    print(f"fixture : {ctx.exe}")
    print(f"SHA256  : {sha}\n")
    n_ok = n_fail = 0
    for (cid, desc, fn) in CASES:
        if not cid.startswith(a.select):
            continue
        try:
            ok, detail = fn(ctx)
        except Exception as ex:
            ok, detail = False, f"EXCEPTION {type(ex).__name__}: {ex}"
        n_ok += ok; n_fail += (not ok)
        print(f"{'OK  ' if ok else 'FAIL'} {cid} {desc}\n          {detail}")
    print(f"\nSUMMARY: OK={n_ok} FAIL={n_fail} -> "
          f"{'PASS' if n_fail == 0 and n_ok > 0 else 'FAIL'}")
    sys.exit(min(n_fail, 255) if n_ok > 0 else 255)

if __name__ == "__main__":
    main()
