#=============================================================================
# QUTest Python scripting support
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
# Contact information:
# <www.state-machine.com>
# <info@state-machine.com>
#=============================================================================
##
# @date Last updated on: 2023-12-13
# @version Last updated for version: 7.3.1
#
# @file
# @brief QUTest Python scripting support (implementation)
# @ingroup qutest

import argparse
import socket
import struct
import sys
import os
if os.name == "nt":
    import msvcrt
else:
    import select

from platform import python_version

#=============================================================================
# Helper class for communication with the QSpy front-end
#
class QSpy:
    VERSION = 731

    # private class variables...
    _sock = None
    _is_attached = False
    _tx_seq = 0
    _host_udp = ["localhost", 7701] # list, to be converted to a tuple
    _local_port = 0 # let the OS decide the best local port

    # timeout value [seconds]
    _TOUT = 1.000

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

    # packets to QSpy to be "massaged" and forwarded to the Target...
    _QSPY_SEND_EVENT      = 135
    _QSPY_SEND_AO_FILTER  = 136
    _QSPY_SEND_CURR_OBJ   = 137
    _QSPY_SEND_COMMAND    = 138
    _QSPY_SEND_TEST_PROBE = 139

    @staticmethod
    def _init():
        # Create socket
        QSpy._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        QSpy._sock.settimeout(QSpy._TOUT) # timeout for blocking socket
        #bufsize = QSpy._sock.getsockopt(socket.SOL_UDP, socket.SO_RCVBUF)
        #print("SO_RCVBUF ", bufsize)
        try:
            QSpy._sock.bind(("0.0.0.0", QSpy._local_port))
            #print("bind: ", ("0.0.0.0", QSpy._local_port))
        except:
            messagebox.showerror("UDP Socket Error",
               "Can't bind the UDP socket\nto the specified local_host")
            QSpyView._gui.destroy()
            return -1
        return 0

    @staticmethod
    def _sendTo(packet, str=None):
        tx_packet = bytearray([QSpy._tx_seq])
        tx_packet.extend(packet)
        if str is not None:
            tx_packet.extend(bytes(str, "utf-8"))
            tx_packet.extend(b"\0") # zero-terminate
        QSpy._sock.sendto(tx_packet, QSpy._host_udp)
        QSpy._tx_seq = (QSpy._tx_seq + 1) & 0xFF
        #print("sendTo", QSpy._tx_seq)

#=============================================================================
# main entry point to QUTest
def main():
    # parse command-line arguments...
    parser = argparse.ArgumentParser(
        prog="python qspy_exit.py",
        description="QSPY-exit",
        epilog="More info: https://www.state-machine.com/qspy.html#qspy_exit")
    parser.add_argument('-v', '--version',
        action='version',
        version="QSPY-exit %d.%d.%d on Python %s"%(
                    QSpy.VERSION//100, (QSpy.VERSION//10) % 10,
                    QSpy.VERSION % 10,
            python_version()),
        help='Display QSPY-exit version')

    parser.add_argument('-q', '--qspy', nargs='?', default='', const='',
        help="optional qspy host, [:ud_port]")
    args = parser.parse_args()
    #print(args)

    print("\nQSPY-exit %d.%d.%d running on Python %s"%(
            QSpy.VERSION//100,
            (QSpy.VERSION//10) % 10,
             QSpy.VERSION % 10, python_version()))
    print("Copyright (c) 2005-2023 Quantum Leaps, www.state-machine.com")

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

    QSpy._sendTo(struct.pack("<BB", QSpy._QSPY_DETACH, 1))

    return 0 # report to the caller (e.g., make)

#=============================================================================
if __name__ == "__main__":
    main()
