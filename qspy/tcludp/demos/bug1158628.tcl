# bug1158628.tcl - Copyright (C) 2005 Pat Thoyts <patthoyts@users.sf.net>
#
# "On windows XP, I have a GUI that has an exit buttons which when
# pressed does: {set done 1; destroy .;exit} If there is an open UDP
# channel with a fileevent on it, the program will not exit --
# i.e. task manager still shows it. Also if I have the console up, the
# console goes away when the exit button is invoked, but the program
# does not exit.  NOTE -- all windows are correctly destroyed (or at
# least withdrawn)"
#
# The fault is calling Tcl_UnregisterChannel in the udpClose function.
# We must let tcl handle this itself. Solved by Reinhard Max.
#
# This script demonstrates the problem. Using udp 1.0.6 the program hangs
# after printing "Exiting...". With the fix applied it properly exits.
#
# $Id: bug1158628.tcl,v 1.2 2005/05/19 20:46:23 patthoyts Exp $

#load [file join [file dirname [info script]] .. win Release udp107.dll]
#load [file join [file dirname [info script]] .. i386-unknown-openbsd3.6 libudp107.so]
package require udp

variable forever 0

proc Event {sock} {
    variable forever
    set pkt [read $sock]
    set peer [fconfigure $sock -peer]
    puts "Recieved [string length $pkt] from $peer\n$pkt"
    #set forever 1
    return
}

proc Listen {port} {
    set s [udp_open $port]
    fconfigure $s -blocking 0 -buffering none -translation binary
    fileevent $s readable [list Event $s]
    return $s
}

proc Exit {sock} {
    puts "Exiting"
    exit 0
}

if {!$tcl_interactive} {
    puts "Bug #1158628 - hangs in exit if open udp channels"
    puts "  Using a buggy version, this will hang after printing Exiting..."
    puts ""
    set sock [Listen 10245]
    puts "Wait 1 sec..."
    after 1000 [list Exit $sock]
    vwait forever
    close $sock
}
