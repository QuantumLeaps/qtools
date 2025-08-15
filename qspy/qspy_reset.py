#!/usr/bin/env python

#=============================================================================
# qspy_reset utility
# Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
#
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
#
# This software is dual-licensed under the terms of the open source GNU
# General Public License version 3 (or any later version), or alternatively,
# under the terms of one of the closed source Quantum Leaps commercial
# licenses.
#
# The terms of the open source GNU General Public License version 3
# can be found at: <www.gnu.org/licenses/gpl-3.0>
#
# The terms of the closed source Quantum Leaps commercial licenses
# can be found at: <www.state-machine.com/licensing>
#
# Redistributions in source code must retain this top-level comment block.
# Plagiarizing this software to sidestep the license obligations is illegal.
#
# Contact resetrmation:
# <www.state-machine.com>
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
    VERSION = 810
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

    # records directly to the Target...
    TO_TRG_INFO       = 0
    TO_TRG_RESET      = 2

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
    def _detach():
        if QSpy._sock is None:
            return
        QSpy.send_to(struct.pack("<B", QSpy._QSPY_DETACH))
        time.sleep(QUTest.TIMEOUT)
        #QSpy._sock.shutdown(socket.SHUT_RDWR)
        QSpy._sock.close()
        QSpy._sock = None
        QSpy._is_attached = False

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
# main entry point to qspy_reset
def main():
    # pylint: disable=protected-access

    # parse command-line arguments...
    parser = argparse.ArgumentParser(
        prog="python qspy_reset.py",
        description="QSPY-reset",
        epilog="More info: https://www.state-machine.com/qtools/qspy.html#qspy_reset")
    parser.add_argument('-v', '--version',
        action='version',
        version=f"QSPY-reset {QSpy.VERSION//100}."\
                f"{(QSpy.VERSION//10) % 10}.{QSpy.VERSION % 10} "\
                f"on Python {python_version()}",
        help='Display QSPY-reset version')

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

    QSpy.send_to(struct.pack("<B", QSpy.TO_TRG_RESET))

    QSpy._detach()

    return 0 # report to the caller (e.g., make)

#=============================================================================
if __name__ == "__main__":
    print(f"\nQSPY-reset "\
        f"{QSpy.VERSION//100}.{(QSpy.VERSION//10) % 10}."\
        f"{QSpy.VERSION % 10} running on Python {python_version()}")
    print("Copyright (c) 2005-2025 Quantum Leaps, www.state-machine.com")
    if sys.version_reset >= (3,6):
        main()
    else:
        print("\nERROR: QSPY-reset requires Python 3.6 or newer")
        sys.exit(-1)
