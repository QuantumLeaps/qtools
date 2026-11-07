#!/usr/bin/env python3
#=============================================================================
# fakeqspy.py -- QSPY impersonator for the QUTest self-test suite
#
# Copyright (C) 2005-2026 Quantum Leaps, LLC. All rights reserved.
# SPDX-License-Identifier: LicenseRef-QL-commercial
# Licensed under the terms of the Quantum Leaps commercial license agreement.
#
# Plays the role of QSPY *and* of a QUTest target (remote-target mode, i.e.,
# qutest.py runs WITHOUT -e), so that the self-test scripts can provoke
# UDP-level faults that a real QSPY never produces: lost packets (sequence
# gaps), packets from a foreign source, and sequence-number wrap-around.
#
# Protocol subset (front-end side, as implemented by QSPY qspy_be.c):
# - every packet to the front-end starts with [seq, pkt-id]; the seq starts
#   at 1 with the attach confirmation and is incremented for every packet,
#   EXCEPT the Target-info packet, which QSPY forwards raw from the target
#   (it carries the TARGET's sequence number);
# - text lines: [seq, 0 (QS_EMPTY), rec-id, text...]
#
# Commands understood (TO_TRG_COMMAND, numeric IDs only):
#   0  ECHO        <p1>: ECHO p1, Trg-Done                 (normal)
#   1  LOSE_EXTRA  <p1>: ECHO p1, Trg-Done, then an extra line "ECHO 99"
#                        that is LOST (its sequence number is skipped)
#   2  FOREIGN     <p1>: ECHO p1 and Trg-Done are sent from a FOREIGN UDP
#                        socket (different source port), with sequence
#                        numbers that continue the series seamlessly
#   3  BURST       <p1>: p1 lines "ECHO 0".."ECHO p1-1", Trg-Done, paced
#                        (used to wrap the sequence number past 255)
#   4  ASSERT      <p1>: the target asserts ("=ASSERT= Mod=fakeqspy,Loc=p1")
#                        and then RESETS ITSELF, like a real target on
#                        hardware (new Target info + QF_RUN, unprompted)
#   5  DEAF            : ECHO 0, Trg-Done; from now on the target ignores
#                        TO_TRG_RESET (a target that doesn't reset)
#   6  RESETS          : "RESETS <n>", Trg-Done, where n is the number of
#                        TO_TRG_RESET requests received since the attach
#   7  ARM_TEARDOWN <p1>: ECHO p1, Trg-Done; at the next test teardown the
#                        target emits "TEARDOWN p1" AFTER the teardown Ack
#                        (like QS_onTestTeardown() of a real target)
#   8  DUP         <p1>: ECHO p1 sent TWICE with the same sequence number
#                        (a duplicated UDP packet), then Trg-Done
#   9  BAD_INFO    <p1>: a Target-info packet of p1 bytes instead of 20
#                        (18 = target older than QP 8.0.0, 19 = corrupted)
#  10  BAD_PKT     <p1>: p1=0: a packet of only 1 byte (too short);
#                        p1=1: a packet with an unknown packet type (99)
#  11  DETACH          : QSPY detaches the front-end (packet 129)
#
# TO_TRG_INFO is answered with the Target-info packet only (no reset, no
# change of the timestamp), like QS_RX_INFO of a real target.
#
# On-target behavior modeled: qutest.py (without -e) must send TO_TRG_RESET
# before every test that isn't NORESET, but NOT after an assertion (the
# target resets itself); RESETS lets a script verify both.
#
# usage: python3 fakeqspy.py <udp-port>
#=============================================================================
import socket
import struct
import sys
import time

PKT_TEXT_ECHO   = 0
PKT_ASSERTION   = 69  # QS_ASSERTION record ID (text packet byte 2)
PKT_TARGET_INFO = 64
PKT_ATTACH_CONF = 128
PKT_DETACH      = 129

QSPY_ATTACH     = 128
TO_TRG_INFO     = 0
TO_TRG_COMMAND  = 1
TO_TRG_RESET    = 2
TO_TRG_SETUP    = 7
TO_TRG_TEARDOWN = 8

REC_USER = 100   # QS record ID used for the text lines (not QS_ASSERTION)
TRG_SEQ  = 0xA5  # arbitrary target seq. in the Target-info packet

class Fake:
    def __init__(self, port):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("127.0.0.1", port))
        self.foreign = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.foreign.bind(("127.0.0.1", 0)) # different source port
        self.fe = None   # front-end address
        self.seq = 0     # tx sequence number (shared by both sockets)
        self.ts = 0      # target timestamp
        self.resets = 0  # TO_TRG_RESET requests received
        self.deaf = False # target ignores TO_TRG_RESET
        self.teardown_out = 0 # 0: no output at teardown; else "TEARDOWN n"

    def _next_seq(self):
        self.seq = (self.seq + 1) & 0xFF
        return self.seq

    def send_pkt(self, pkt_id, data=b""):
        self.sock.sendto(bytes([self._next_seq(), pkt_id]) + data, self.fe)

    def line(self, text, sock=None, rec=REC_USER):
        pkt = bytes([self._next_seq(), PKT_TEXT_ECHO, rec]) \
              + text.encode("utf-8")
        (sock or self.sock).sendto(pkt, self.fe)

    def tline(self, text, sock=None):  # timestamped line
        self.ts += 1
        self.line(f"{self.ts:010d} {text}", sock)

    def target_info(self):
        # same layout as forwarded by QSPY: [trg-seq, 64, 1-byte, release(4),
        # 13 bytes of target info] -> 20 bytes; QP 8.1.6, 32-bit pointers
        release = 260101 * 10000 + 816
        info = bytes([0x22, 0x41, 0x22, 0x44, 0x04, 0, 0,
                      0, 0, 12, 1, 1, 26])  # sizes..., ss,mm,hh,dd,MM,yy
        pkt = bytes([TRG_SEQ, PKT_TARGET_INFO, 0xFF]) \
              + struct.pack("<L", (~release) & 0xFFFFFFFF) + info
        assert len(pkt) == 20
        self.sock.sendto(pkt, self.fe)  # NOTE: does not use self.seq

    def boot(self):  # the target (re)starts: Target info, then QF_RUN
        self.ts = 0
        self.teardown_out = 0
        self.target_info()
        self.line("           QF_RUN") # expected by on_reset()

    def run(self):
        while True:
            pkt, addr = self.sock.recvfrom(4096)
            if len(pkt) < 2:
                continue
            rec = pkt[1]
            if rec == QSPY_ATTACH:
                self.fe = addr
                self.seq = 0
                self.send_pkt(PKT_ATTACH_CONF)
            elif self.fe is None:
                continue
            elif rec == TO_TRG_RESET:
                self.resets += 1
                if not self.deaf:
                    self.boot()
            elif rec == TO_TRG_INFO:
                self.target_info()  # no reset, timestamp unchanged
            elif rec == TO_TRG_SETUP:
                self.ts = 0
                self.line("           Trg-Ack  QS_RX_TEST_SETUP")
            elif rec == TO_TRG_TEARDOWN:
                self.line("           Trg-Ack  QS_RX_TEST_TEARDOWN")
                if self.teardown_out != 0:  # output after the Ack
                    self.tline(f"TEARDOWN {self.teardown_out}")
            elif rec == TO_TRG_COMMAND:
                cmd, p1, _p2, _p3 = struct.unpack("<BIII", pkt[2:15])
                self.line("           Trg-Ack  QS_RX_COMMAND")
                if cmd == 0:    # ECHO
                    self.tline(f"ECHO {p1}")
                    self.tline("Trg-Done QS_RX_COMMAND")
                elif cmd == 1:  # LOSE_EXTRA
                    self.tline(f"ECHO {p1}")
                    self.tline("Trg-Done QS_RX_COMMAND")
                    self._next_seq()  # the extra line "ECHO 99" is lost
                    self.ts += 1
                elif cmd == 2:  # FOREIGN
                    self.tline(f"ECHO {p1}", self.foreign)
                    self.tline("Trg-Done QS_RX_COMMAND", self.foreign)
                elif cmd == 4:  # ASSERT, then the target resets itself
                    self.ts += 1
                    self.line(f"{self.ts:010d} =ASSERT= Mod=fakeqspy,Loc={p1}",
                              rec=PKT_ASSERTION)
                    time.sleep(0.05)  # "reboot" time
                    self.boot()
                elif cmd == 5:  # DEAF
                    self.tline("ECHO 0")
                    self.tline("Trg-Done QS_RX_COMMAND")
                    self.deaf = True
                elif cmd == 7:  # ARM_TEARDOWN
                    self.teardown_out = p1
                    self.tline(f"ECHO {p1}")
                    self.tline("Trg-Done QS_RX_COMMAND")
                elif cmd == 8:  # DUP: the same packet twice
                    self.ts += 1
                    pkt = bytes([self._next_seq(), PKT_TEXT_ECHO, REC_USER]) \
                          + f"{self.ts:010d} ECHO {p1}".encode("utf-8")
                    self.sock.sendto(pkt, self.fe)
                    self.sock.sendto(pkt, self.fe)  # duplicate
                    self.tline("Trg-Done QS_RX_COMMAND")
                elif cmd == 9:  # BAD_INFO: Target info of the wrong length
                    good = bytes([TRG_SEQ, PKT_TARGET_INFO]) + bytes(18)
                    self.sock.sendto(good[:p1], self.fe)
                elif cmd == 10: # BAD_PKT
                    if p1 == 0:
                        self.sock.sendto(bytes([self._next_seq()]), self.fe)
                    else:
                        self.send_pkt(99)  # unknown packet type
                elif cmd == 11: # DETACH
                    self.send_pkt(PKT_DETACH)
                elif cmd == 6:  # RESETS
                    self.tline(f"RESETS {self.resets}")
                    self.tline("Trg-Done QS_RX_COMMAND")
                elif cmd == 3:  # BURST
                    for i in range(p1):
                        self.tline(f"ECHO {i}")
                        # pace the burst, so that no packet is REALLY lost
                        # (an unpaced burst of 300 packets overflowed the
                        # receive buffer of qutest.py even on localhost)
                        time.sleep(0.002)
                    self.tline("Trg-Done QS_RX_COMMAND")
            # all other packets (notes, detach, ...) are ignored

if __name__ == "__main__":
    Fake(int(sys.argv[1])).run()
