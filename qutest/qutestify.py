#!/usr/bin/env python

#=============================================================================
# qutestify is a utility to apply QUTest script syntax around the
# raw QSPY output
#
#                    Q u a n t u m  L e a P s
#                    ------------------------
#                    Modern Embedded Software
#
# Copyright (C) 2005-2026 Quantum Leaps, LLC. All rights reserved.
#
# SPDX-License-Identifier: LicenseRef-QL-commercial
#
# This software is licensed under the terms of the Quantum Leaps commercial
# license agreement. This header is a notice of that license, not a
# substitute for it; the signed agreement governs in case of conflict.
# Use of this software requires a valid Quantum Leaps commercial or
# evaluation license. Contact Quantum Leaps for licensing options.
#
# RESTRICTIONS
# Unless otherwise agreed in writing with Quantum Leaps, the licensee
# may NOT:
# (a) redistribute, encumber, sell, rent, lease, sublicense, or otherwise
#     transfer rights in this software to any third party,
# (b) remove, obscure, or alter any trademark, copyright, or other
#     proprietary notice, legend, symbol, or label present in this
#     software,
# (c) create derivative works, or modify or repackage this software, for
#     the purpose of concealing its origin or evading the licensing
#     obligations that apply to it,
# (d) use this software, or any portion of it, as input to train,
#     fine-tune, evaluate, or otherwise develop any artificial
#     intelligence or machine learning model, or use such a model to
#     replicate, emulate, or compete with the functionality of this
#     software, whether for internal use or in any product or service
#     offered to a third party.
#
# This software is provided "AS IS," without warranty of any kind, express
# or implied, except as expressly stated in the governing license agreement.
#
# Quantum Leaps contact information:
# <www.state-machine.com/licensing>
# <info@state-machine.com>
#=============================================================================
import os
import sys

VERSION = 816

def main():
    # process command-line arguments...
    argv = sys.argv
    argc = len(argv)
    arg  = 1 # skip the 'qutestify' argument

    if '-h' in argv or '--help' in argv or '?' in argv \
        or '--version' in argv:
        print(f"QUTest script converter {VERSION//100}."\
              f"{(VERSION//10) % 10}.{VERSION % 10}\n"\
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
        elif line.startswith('           TE'):
            lchnum += 1
            new_lines.append(f'expect("           TE{line[13:-1]}")\n')
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
    print(f"\nQUTestify utility "\
        f"{VERSION//100}.{(VERSION//10) % 10}."\
        f"{VERSION % 10}")
    print("Copyright (c) 2005-2026 Quantum Leaps, www.state-machine.com")
    if sys.version_info >= (3,6):
        main()
    else:
        print("\nERROR: QUTest requires Python 3.6 or newer")
        sys.exit(-1)
