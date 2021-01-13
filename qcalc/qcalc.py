#-----------------------------------------------------------------------------
# Product: QCalc in Python (requires Python 3.3+)
# Last updated for version 6.9.2
# Last updated on  2021-01-10
#
#                    Q u a n t u m  L e a P s
#                    ------------------------
#                    Modern Embedded Software
#
# Copyright (C) 2005-2021 Quantum Leaps, LLC. All rights reserved.
#
# This program is open source software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <www.gnu.org/licenses/>.
#
# Contact information:
# <www.state-machine.com>
# <info@state-machine.com>
#-----------------------------------------------------------------------------

import traceback

from math import *
from platform import python_version

class QCalc:
    ## current version of QCalc
    VERSION = 692

    @staticmethod
    def _main():
        print("QCalc Programmer's Calculator {0:d}.{1:d}.{2:d} " \
              "running on Python {3}".format(
                  QCalc.VERSION//100,
                  (QCalc.VERSION//10) % 10,
                  QCalc.VERSION % 10,
                  python_version()))
        print("(c) 2005-2021 Quantum Leaps, www.state-machine.com\n")

        # main loop for processing user input...
        while True:
            expr = input('> ')
            if expr:
                try:
                    result = eval(expr)
                except:
                    traceback.print_exc(2)
                else:
                    QCalc._print(result)
            else:
                break

    @staticmethod
    def _print(result):
        # the 'ans' global variable
        global ans
        ans = result

        if isinstance(ans, int):
            # outside the 64-bit range?
            if ans < -0xFFFFFFFFFFFFFFFF or 0xFFFFFFFFFFFFFFFF < ans:
                print("! out of range")
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

# the 'ans' global variable
ans = 0

#=============================================================================
if __name__ == "__main__":
    QCalc._main()
