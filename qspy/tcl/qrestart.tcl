##
# @file
# @ingroup qpspy
# @brief Script to run a given test fixture repetitively so that
# a remote test script can reset the target, and it will be restarted.
# @usage
# tclsh qrestart.tcl [test_fixture] [host][:port]
## @cond
#-----------------------------------------------------------------------------
# Last updated for version 6.2.0
# Last updated on  2018-03-14
#
#                    Q u a n t u m     L e a P s
#                    ---------------------------
#                    innovating embedded systems
#
# Copyright (C) 2005-2018 Quantum Leaps, LLC. All rights reserved.
#
# This program is open source software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Alternatively, this program may be distributed and modified under the
# terms of Quantum Leaps commercial licenses, which expressly supersede
# the GNU General Public License and are specifically designed for
# licensees interested in retaining the proprietary status of their code.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Contact information:
# https://www.state-machine.com
# mailto:info@state-machine.com
#-----------------------------------------------------------------------------
# @endcond

set app [lindex $::argv 0]
set argc [llength $::argv]
if {$argc > 1} {
    set arg1 [lindex $::argv 1]
}

# continue re-starting the given application
set run 1
while {$run} {
    if {$argc > 1} {
        puts "(re)starting $app $arg1"
        if {[catch {exec $app $arg1} results]} {
            set run 0
        }
    } else {
        puts "(re)starting $app"
        if {[catch {exec $app} results ]} {
            set run 0
        }
    }
}
puts "User exit"
