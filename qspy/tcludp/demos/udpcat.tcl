# udpsend.tcl - Copyright (C) 2004 Pat Thoyts <patthoyts@users.sf.net>
#
# Demo application - cat data from stdin via a UDP socket.
#
# $Id: udpcat.tcl,v 1.1 2004/11/22 23:48:47 patthoyts Exp $

package require udp 1.0.6

proc Event {sock} {
    global forever
    set pkt [read $sock]
    set peer [fconfigure $sock -peer]
    puts "Received [string length $pkt] from $peer\n$pkt"
    set forever 1
    return
}

proc Send {host port {msg {}}} {
    set s [udp_open]
    fconfigure $s -blocking 0 -buffering none -translation binary \
        -remote [list $host $port]
    fileevent $s readable [list Event $s]
    if {$msg == {}} {
        fcopy stdin $s
    } else {
        puts -nonewline $s $msg
    }

    after 2000
    close $s
}

proc Listen {port} {
    set s [udp_open $port]
    fconfigure $s -blocking 0 -buffering none -translation binary
    fileevent $s readable [list Event $s]
    return $s
}

# -------------------------------------------------------------------------
# Runtime
# udpsend listen -port N -blocking 0
# udpsend send host port message
# -------------------------------------------------------------------------
set forever 0

if {! $tcl_interactive} {
    switch -exact -- [set cmd [lindex $argv 0]] {
        send {
            eval [list Send] [lrange $argv 1 end]
        }
        listen {
            set s [Listen [lindex $argv 1]]
            vwait ::forever
            close $s
        }
        default {
            puts "usage: udpcat send host port ?message?\
                \n       udpcat listen port"
        }
    }
}


