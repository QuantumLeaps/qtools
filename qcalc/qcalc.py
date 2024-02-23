#!/usr/bin/env python

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
# @date Last updated on: 2023-12-18
# @version Last updated for version: 7.3.3
#
# @file
# @brief QCalc programmer's Calculator
# @ingroup qtools

# pylint: disable=broad-except
# pylint: disable=eval-used


'''
"qcalc" is a powerful, cross-platform calculator specifically designed for
embedded systems programmers. The calculator accepts whole expressions in
the **C-syntax** and displays results simultaneously in decimal, hexadecimal,
and binary without the need to explicitly convert the result to these bases.

'''

import os
import sys
import traceback

from platform import python_version

# NOTE: the following wildcard import from math is needed
# for user expressions.
#
# pylint: disable=wildcard-import
# pylint: disable=unused-wildcard-import
# pylint: disable=redefined-builtin
from math import *

# current version of QCalc
VERSION = 733

# the 'ans' global variable
ans = {}

def display(result):
    '''
    Display result of computation in decimal, hexadecimal, and binary.
    Also, handle the 32-bit and 64-bit cases.
    '''

    # pylint: disable=global-statement
    global ans
    ans = result

    if isinstance(ans, int):
        # outside the 64-bit range?
        if ans < -0xFFFFFFFFFFFFFFFF or 0xFFFFFFFFFFFFFFFF < ans:
            print("\x1b[41m\x1b[1;37m! out of range\x1b[0m")
            return

        #inside the 32-bit range?
        if -0xFFFFFFFF < ans <= 0xFFFFFFFF:
            # use 32-bit unsigned arithmetic...
            u32 = ans & 0xFFFFFFFF
            u16_0 = u32 & 0xFFFF
            u16_1 = (u32 >> 16) & 0xFFFF
            u8_0 = u32 & 0xFF
            u8_1 = (u32 >> 8) & 0xFF
            u8_2 = (u32 >> 16) & 0xFF
            u8_3 = (u32 >> 24) & 0xFF
            print(f"= {ans:d} |",
                  f"0x{u16_1:04X}'{u16_0:04X} |",
                  f"0b{u8_3:08b}'{u8_2:08b}'{u8_1:08b}'{u8_0:08b}", end="")
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
            print(f"= {ans:d} |",
                  f"0x{u16_3:04X}'{u16_2:04X}'{u16_1:04X}'{u16_0:04X}")
            print(f"= 0b{u8_7:08b}'{u8_6:08b}'{u8_5:08b}'{u8_4:08b}"\
                  f"'{u8_3:08b}'{u8_2:08b}'{u8_1:08b}'{u8_0:08b}", end="")
    else:
        print("=", ans)

#=============================================================================
# main entry point to QCalc
def main():
    '''
    Main entry point to QCalc. Process command-line parameters
    and handle separately "batch" and "interactive" modes.
    '''
    # "batch mode": expression provided in command-line arguments
    if len(sys.argv) > 1:
        expr = "".join(sys.argv[1:])
        print(expr)
        try:
            result = eval(expr)
        except Exception:
            traceback.print_exc(2)
        else:
            display(result)
        return

    # "interactive mode": expressions provided as user input
    while True:
        expr = input('> ')
        if expr:
            try:
                result = eval(expr)
            except Exception:
                traceback.print_exc(2)
            else:
                print("\x1b[47m\x1b[30m", end = "")
                display(result)
                print("\x1b[0m")
        else:
            break


#=============================================================================
if __name__ == "__main__":
    if os.name == "nt":
        os.system("color")

    print(f"\nQCalc programmer's calculator "\
        f"{VERSION//100}.{(VERSION//10) % 10}."\
        f"{VERSION % 10} running on Python {python_version()}")
    print("Copyright (c) 2005-2023 Quantum Leaps, www.state-machine.com")
    if sys.version_info >= (3,6):
        main()
    else:
        print("\nERROR: QCalc requires Python 3.6 or newer")
        sys.exit(-1)
