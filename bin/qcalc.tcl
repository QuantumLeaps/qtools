#-----------------------------------------------------------------------------
# Product: QCalc -- Desktop Calculator with C syntax and hex/binary displays
# Last updated for version 6.6.0
# Last updated on  2019-07-30
#
#                    Q u a n t u m     L e a P s
#                    ---------------------------
#                    innovating embedded systems
#
# Copyright (C) 2005-2019 Quantum Leaps, LLC. All rights reserved.
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
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Documentation:
# https://www.state-machine.com/qtools/qcalc.html
#
# Contact information:
# www.state-machine.com
# info@state-machine.com
#-----------------------------------------------------------------------------

# the input and answer variables
set input 0
set ans   0
set var   x

# history of inputs...
array set history {
    0 ""
    1 ""
    2 ""
    3 ""
    4 ""
    5 ""
    6 ""
    7 ""
}
set history_max 8
set history_len 0
set history_idx 0

# binary constants...
set 0000 0
set 0001 1
set 0010 2
set 0011 3
set 0100 4
set 0101 5
set 0110 6
set 0111 7
set 1000 8
set 1001 9
set 1010 10
set 1011 11
set 1100 12
set 1101 13
set 1110 14
set 1111 15
set 0    0
set 1    1
set 10   2
set 11   3
set 100  4
set 101  5
set 110  6
set 111  7

# the pi constant
set pi  [expr acos(0)*2.0]

#-----------------------------------------------------------------------------
# callbacks...
proc onEnter {} {
   global input ans var

   if {[string first {=} $input] == 0} { ;# variable assignment?
       if {$ans == ""} {
           report_error "no value for a variable"
       } else {
           set var [string range $input 1 end]
           set var [string trim $var]
           if {[regexp {^[_a-zA-Z][_a-zA-Z0-9]*$} $var]} {
               if {[catch {uplevel 1 {set $var $ans}} msg]} {
                   report_error $msg
               } else {
                   history_add $input
               }
           } else {
               report_error "incorrect variable name"
           }
       }
   } elseif {[catch {uplevel 1 {expr $input}} ans]} { ;# expression
       report_error $ans
   } else {
       history_add $input

       if {[catch {expr int($ans) & 0xFFFFFFFF} ans_int]} {
           .ans.hex configure -text "> MAX_INT"
           .ans_bin configure -text "> MAX_INT"
       } elseif {$ans == [expr floor($ans)]} {
           set ans $ans_int
           set int0 [expr $ans_int& 0xFFFF]
           set int1 [expr ($ans_int>> 16) & 0xFFFF]
           .ans.hex configure -text [format "0x%04X,%04X" $int1 $int0]

           set byte0 [dec2bin [expr $ans_int         & 0xFF]]
           set byte1 [dec2bin [expr ($ans_int >>  8) & 0xFF]]
           set byte2 [dec2bin [expr ($ans_int >> 16) & 0xFF]]
           set byte3 [dec2bin [expr ($ans_int >> 24) & 0xFF]]
           .ans_bin configure -text "0b$byte3,$byte2,$byte1,$byte0"
       } else {
           .ans.hex configure -text ""
           .ans_bin configure -text ""
       }
   }

   .ans.dec configure -text $ans
}
#.............................................................................
proc onUp {} {
    global input history history_len history_idx
    if {$history_len > 0} {
        incr history_idx
        if {$history_idx > [expr $history_len - 1]} {
           set history_idx [expr $history_len - 1]
        }
        set input $history($history_idx)
        #.ans.dec configure -text ""
        #.ans.hex configure -text ""
        #.ans_bin configure -text ""
    }
}
#.............................................................................
proc onDown {} {
    global input history history_len history_idx
    if {$history_len > 0} {
        incr history_idx -1
        if {$history_idx < 0} {
           set history_idx 0
        }
        set input $history($history_idx)
        #.ans.dec configure -text ""
        #.ans.hex configure -text ""
        #.ans_bin configure -text ""
    }
}

#-----------------------------------------------------------------------------
# helpers...

proc history_add {inp} {
    global input history history_len history_idx history_max
    if {$history($history_idx) != $inp} {
        incr history_len
        if {$history_len > $history_max} {
            set history_len $history_max
        }
        for {set i [expr $history_len - 1]} {$i > 0} {incr i -1} {
            set history($i) $history([expr $i - 1])
        }
        set history(0) $inp
        set history_idx 0
    }
}
#.............................................................................
proc dec2bin {int} {
    set binRep [binary format c $int]
    binary scan $binRep B* binStr
    return $binStr
}
#.............................................................................
proc report_error {msg} {
    global ans
    set ans ""
    .ans.dec configure -text Error
    .ans.hex configure -text ""
    .ans_bin configure -text $msg
}

#-----------------------------------------------------------------------------
# QCalc GUI...

wm title . "state-machine.com/qtools/qcalc.html"
wm iconname . "QCalc"

option add *inxFont -*-courier-medium-r-normal--*-160-* startupFile
set inx_font [option get . inxFont InxFont]
option add *Label*font $inx_font startupFile
option add *Button*font $inx_font startupFile
option add *Entry.font $inx_font startupFile
option add *Entry.background white startupFile

frame .entry -borderwidth 3 -relief raised
entry .entry.inp -xscrollcommand ".entry.scrl set" -textvariable input

scrollbar .entry.scrl -relief sunken -orient horiz \
-command ".entry.inp xview"
bind .entry.inp <KeyPress-Return> { onEnter }
bind .entry.inp <KeyPress-Up>     { onUp }
bind .entry.inp <KeyPress-Down>   { onDown }
pack .entry.inp -side top -fill x -expand 1
pack .entry.scrl -side top -fill x -expand 1
pack .entry -side top -fill x -expand 1

frame .ans -borderwidth 1
label .ans.dec -relief sunken -width 25 -anchor w -text ""
label .ans.hex -relief sunken -width 11 -anchor e -text ""
pack .ans.dec -side left -padx 1 -pady 1 -fill x -expand 1
pack .ans.hex -side left -padx 1 -pady 1 -fill x -expand 1
pack .ans -side top -padx 1 -pady 1 -fill x -expand 1

label .ans_bin  -relief sunken -width 36 -anchor e -text ""
pack .ans_bin -side top -padx 2 -pady 2 -fill x -expand 1

focus .entry.inp

onEnter

update idletasks
wm minsize . [winfo reqwidth .] [winfo reqheight .]
wm resizable . true false
