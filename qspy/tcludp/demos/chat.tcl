# chat.tcl - Copyright (C) 2004 Pat Thoyts <patthoyts@users.sourceforge.net>
#
# This is a sample application from TclUDP.
#
# This illustrates the use of multicast UDP messages to implement a
# primitive chat application.
#
# $Id: chat.tcl,v 1.2 2007/04/10 23:36:14 patthoyts Exp $

package require Tk  8.4
package require udp 1.0.6

variable Address  224.5.1.21
variable Port     7771

proc Receive {sock} {
    set pkt [read $sock]
    set peer [fconfigure $sock -peer]
    AddMessage $peer $pkt
    return
}

proc Start {addr port} {
    set s [udp_open $port]
    fconfigure $s -blocking 0 -buffering none -translation binary \
        -mcastadd $addr -remote [list $addr $port]
    fileevent $s readable [list ::Receive $s]
    return $s
}

proc CreateGui {socket} {
    text .t -yscrollcommand {.s set}
    scrollbar .s -command {.t yview}
    frame .f -border 0
    entry .f.e -textvariable ::_msg
    button .f.ok -text Send -underline 0 \
        -command "SendMessage $socket \$::_msg"
    button .f.ex -text Exit -underline 1 -command {destroy .}
    pack .f.ex .f.ok -side right
    pack .f.e -side left -expand 1 -fill x
    grid .t .s -sticky news
    grid .f -  -sticky ew
    grid columnconfigure . 0 -weight 1
    grid rowconfigure . 0 -weight 1
    bind .f.e <Return> {.f.ok invoke}
    .t tag configure CLNT -foreground red
    .t configure -tabs {90}
}

proc SendMessage {sock msg} {
    puts -nonewline $sock $msg
}

proc AddMessage {client msg} {
    set msg [string map [list "\r\n" "" "\r" "" "\n" ""] $msg]
    set client [lindex $client 0]
    if {[string length $msg] > 0} {
        .t insert end "$client\t" CLNT "$msg\n" MSG
        .t see end
    }
}

proc Main {} {
    variable Address
    variable Port
    variable sock
    set sock [Start $Address $Port]
    CreateGui $sock
    after idle [list SendMessage $sock \
                    "$::tcl_platform(user)@[info hostname] connected"]
    tkwait window .
    close $sock
}

if {!$tcl_interactive} {
    set r [catch [linsert $argv 0 Main] err]
    if {$r} {puts $::errorInfo} else {puts $err}
    exit 0
}

