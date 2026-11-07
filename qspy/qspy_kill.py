#!/usr/bin/env python

#=============================================================================
# qspy_kill utility (part of QSPY)
#
#                    Q u a n t u m  L e a P s
#                    ------------------------
#                    Modern Embedded Software
#
# Copyright (C) 2005-2026 Quantum Leaps, LLC. All rights reserved.
#
# SPDX-License-Identifier: LicenseRef-QL-commercial
#
# This software is licensed under the terms of the Quantum Leaps
# QSPY SOFTWARE TRACING HOST UTILITY SOFTWARE END USER LICENSE.
# Please see the file LICENSE-qspy.txt for the complete license text.
#
# Quantum Leaps contact information:
# <www.state-machine.com/licensing>
# <info@state-machine.com>
#=============================================================================

# pylint: disable=missing-module-docstring,
# pylint: disable=missing-class-docstring,
# pylint: disable=missing-function-docstring
# pylint: disable=broad-except

from platform import python_version

import argparse
import socket
import struct
import sys

#=============================================================================
# Helper class for communication with the QSpy front-end
#
class QSpy:

    # public class constants
    VERSION = 816
    TIMEOUT = 1.000 # timeout value [seconds]

    # private class variables...
    _sock = None
    _is_attached = False
    _tx_seq = 0
    _host_udp = ["localhost", 7701] # list, to be converted to a tuple
    _local_port = 0 # let the OS decide the best local port

    # packets to QSpy only...
    _QSPY_ATTACH          = 128
    _QSPY_DETACH          = 129
    _QSPY_SAVE_DICT       = 130
    _QSPY_TEXT_OUT        = 131
    _QSPY_BIN_OUT         = 132
    _QSPY_MATLAB_OUT      = 133
    _QSPY_SEQUENCE_OUT    = 134
    _QSPY_CLEAR_SCREEN    = 140
    _QSPY_SHOW_NOTE       = 141

    @staticmethod
    def _init():
        # Create socket
        QSpy._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        QSpy._sock.settimeout(QSpy.TIMEOUT) # timeout for blocking socket
        #bufsize = QSpy._sock.getsockopt(socket.SOL_UDP, socket.SO_RCVBUF)
        #print("SO_RCVBUF ", bufsize)
        try:
            QSpy._sock.bind(("0.0.0.0", QSpy._local_port))
            #print("bind: ", ("0.0.0.0", QSpy._local_port))
        except Exception:
            print("UDP Socket Error"\
                  "Can't bind the UDP socket\nto the specified local_host")
            sys.exit(-1)
        return 0

    @staticmethod
    def send_to(packet, payload=None):
        tx_packet = bytearray([QSpy._tx_seq])
        tx_packet.extend(packet)
        if payload is not None:
            tx_packet.extend(bytes(payload, "utf-8"))
            tx_packet.extend(b"\0") # zero-terminate
        QSpy._sock.sendto(tx_packet, QSpy._host_udp)
        QSpy._tx_seq = (QSpy._tx_seq + 1) & 0xFF
        #print("sendTo", QSpy._tx_seq)

#=============================================================================
# main entry point to qspy_kill
def main():
    # pylint: disable=protected-access

    # parse command-line arguments...
    parser = argparse.ArgumentParser(
        prog="python qspy_kill.py",
        description="QSPY-kill",
        epilog="More info: https://www.state-machine.com/qtools/qspy.html#qspy_kill")
    parser.add_argument('-v', '--version',
        action='version',
        version=f"QSPY-kill {QSpy.VERSION//100}."\
                f"{(QSpy.VERSION//10) % 10}.{QSpy.VERSION % 10} "\
                f"on Python {python_version()}",
        help='Display QSPY-kill version')

    parser.add_argument('-q', '--qspy', nargs='?', default='', const='',
        help="optional qspy host, [:ud_port]")
    args = parser.parse_args()
    #print(args)

    # process command-line argumens...
    if args.qspy != '':
        qspy_conf = args.qspy.split(":")
        if len(qspy_conf) > 0 and not qspy_conf[0] == '':
            QSpy._host_udp[0] = qspy_conf[0]
        if len(qspy_conf) > 1 and not qspy_conf[1] == '':
            QSpy._host_udp[1] = int(qspy_conf[1])

    #print("host_udp:", QSpy._host_udp)
    #return 0

    # convert to immutable tuple
    QSpy._host_udp = tuple(QSpy._host_udp)

    # init QSpy socket
    err = QSpy._init()
    if err:
        return sys.exit(err)

    # QSPY accepts commands only from an attached Front-End (QSPY 8.1.6),
    # so attach first (binary channel only) and wait for the confirmation
    QSpy.send_to(struct.pack("<BB", QSpy._QSPY_ATTACH, 0x01))
    try:
        packet = QSpy._sock.recv(4096)
    except (socket.timeout, OSError):
        packet = b""
    if len(packet) < 2 or packet[1] != QSpy._QSPY_ATTACH:
        print(f"ERROR: QSPY at {QSpy._host_udp[0]}:{QSpy._host_udp[1]} "
              "did not respond (not running?)")
        return 1

    QSpy.send_to(struct.pack("<BB", QSpy._QSPY_DETACH, 1)) # detach + exit
    print("QSPY exit requested")
    return 0 # report to the caller (e.g., make)

#=============================================================================
if __name__ == "__main__":
    print(f"\nQSPY-kill "\
        f"{QSpy.VERSION//100}.{(QSpy.VERSION//10) % 10}."\
        f"{QSpy.VERSION % 10} running on Python {python_version()}")
    print("Copyright (c) 2005-2026 Quantum Leaps, www.state-machine.com")
    if sys.version_info >= (3,6):
        sys.exit(main())
    else:
        print("\nERROR: QSPY-kill requires Python 3.6 or newer")
        sys.exit(-1)
