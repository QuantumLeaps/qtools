#=============================================================================
# QCalc programmer's Calculator
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
# @date Last updated on: 2022-01-27
# @version Last updated for version: 7.0.0
#
# @file
# @brief QCalc programmer's Calculator
# @ingroup qtools

import os
import traceback

from math import *
from platform import python_version
from sys import argv

# the 'ans' global variable
ans = 0

class QCalc:
    ## current version of QCalc
    VERSION = 700

    @staticmethod
    def _print(result):
        # the 'ans' global variable
        global ans
        ans = result

        if isinstance(ans, int):
            # outside the 64-bit range?
            if ans < -0xFFFFFFFFFFFFFFFF or 0xFFFFFFFFFFFFFFFF < ans:
                print("\x1b[41m\x1b[1;37m! out of range\x1b[0m")
                return

            #inside the 32-bit range?
            if -0xFFFFFFFF < ans and ans <= 0xFFFFFFFF:
                # use 32-bit unsigned arithmetic...
                u32 = ans & 0xFFFFFFFF
                u16_0 = u32 & 0xFFFF
                u16_1 = (u32 >> 16) & 0xFFFF
                u8_0 = u32 & 0xFF
                u8_1 = (u32 >> 8) & 0xFF
                u8_2 = (u32 >> 16) & 0xFF
                u8_3 = (u32 >> 24) & 0xFF
                print("= {0:d} |".format(ans),
                       "0x{0:04X}'{1:04X} |".format(u16_1, u16_0),
                       "0b{0:08b}'{1:08b}'{2:08b}'{3:08b}".format(
                           u8_3, u8_2, u8_1, u8_0))
            else:
                # use 64-bit unsigned arithmetic...
                u64 = ans & 0xFFFFFFFFFFFFFFFF
                u16_0 = u64 & 0xFFFF
                u16_1 = (u64 >> 16) & 0xFFFF
                u16_2 = (u64 >> 32) & 0xFFFF
                u16_3 = (u64 >> 48) & 0xFFFF
                u8_0 = u64 & 0xFF
                u8_1 = (u64 >> 8) & 0xFF
                u8_2 = (u64 >> 16) & 0xFF
                u8_3 = (u64 >> 24) & 0xFF
                u8_4 = (u64 >> 32) & 0xFF
                u8_5 = (u64 >> 40) & 0xFF
                u8_6 = (u64 >> 48) & 0xFF
                u8_7 = (u64 >> 56) & 0xFF
                print("= {0:d} |".format(ans),
                      "0x{0:04X}'{1:04X}'{2:04X}'{3:04X}".format(
                      u16_3, u16_2, u16_1, u16_0))
                print("= 0b{0:08b}'{1:08b}'{2:08b}'{3:08b}"
                         "'{4:08b}'{5:08b}'{6:08b}'{7:08b}".format(
                           u8_7, u8_6, u8_5, u8_4, u8_3, u8_2, u8_1, u8_0))
        else:
            print("=", ans)

#=============================================================================
# main entry point to QCalc
def main():
    if os.name == "nt":
        os.system("color")

    print("QCalc Programmer's Calculator {0:d}.{1:d}.{2:d} " \
          "running on Python {3}".format(
              QCalc.VERSION//100,
              (QCalc.VERSION//10) % 10,
              QCalc.VERSION % 10,
              python_version()))
    print("(c) 2005-2022 Quantum Leaps, www.state-machine.com\n")

    # "batch mode": expression provided in command-line arguments
    if len(argv) > 1:
        expr = "".join(argv[1:])
        print(expr)
        try:
            result = eval(expr)
        except:
            traceback.print_exc(2)
        else:
            QCalc._print(result)
        return

    # "interactive mode": expressions provided as user input
    while True:
        expr = input('> ')
        if expr:
            try:
                result = eval(expr)
            except:
                traceback.print_exc(2)
            else:
                print("\x1b[47m\x1b[30m", end = "")
                QCalc._print(result)
                print("\x1b[0m", end = "")
        else:
            break


#=============================================================================
if __name__ == "__main__":
    main()
