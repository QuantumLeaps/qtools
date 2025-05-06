#!/usr/bin/env python

#=============================================================================
# qutestify is a utility to apply QUTest script syntax around the
# raw QSPY output
#
#                    Q u a n t u m  L e a P s
#                    ------------------------
#                    Modern Embedded Software
#
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
import os
import sys

VERSION = 804

def main():
    # process command-line arguments...
    argv = sys.argv
    argc = len(argv)
    arg  = 1 # skip the 'qutestify' argument

    if '-h' in argv or '--help' in argv or '?' in argv \
        or '--version' in argv:
        print(f"QUTest script converter {VERSION//100}."\
              f"{(VERSION//10) % 10}.{VERSION % 10}"\
              f"Usage: python qutestify.py <test-script>")
        sys.exit(0)

    fname = ''
    if arg < argc:
        # is the next argument a test script?
        fname = argv[arg]

    # parse the provided spexyfile as json data
    try:
        f = open(fname, encoding="utf-8")
    except OSError:
        print("File not found", fname)
        return
    with f:
        lines = f.readlines()

    print("qutestifying:", fname)
    lnum = 0
    lchnum = 0
    new_lines = []
    for line in lines:
        lnum += 1
        if line.startswith('0000'):
            lchnum += 1
            new_lines.append(f'expect("@timestamp{line[10:-1]}")\n')
        elif line.startswith('===RTC===>'):
            lchnum += 1
            new_lines.append(f'expect("{line[:-1]}")\n')
        elif line.startswith('           Tick'):
            lchnum += 1
            new_lines.append(f'expect("           Tick{line[15:-1]}")\n')
        else:
            new_lines.append(line)

    if lchnum > 0:
        with open(fname, "w", newline="\n") as file:
            file.writelines(new_lines)
        print(f"changed lines: {lchnum}.")
    else:
        print("no changes.")

#=============================================================================
if __name__ == "__main__":
    print(f"\nQUTestestify utility "\
        f"{VERSION//100}.{(VERSION//10) % 10}."\
        f"{VERSION % 10}")
    print("Copyright (c) 2005-2025 Quantum Leaps, www.state-machine.com")
    if sys.version_info >= (3,6):
        main()
    else:
        print("\nERROR: QUTest requires Python 3.6 or newer")
        sys.exit(-1)
