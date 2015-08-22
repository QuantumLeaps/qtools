# multicast.tcl - Copyright (C) 2004 Pat Thoyts <patthoyts@users.sf.net>
#
# Demonstrate the use of IPv4 multicast UDP sockets.
#
# You can send to ths using netcat:
#  echo HELLO | nc -u 224.5.1.21 7771
#
# $Id: multicast.tcl,v 1.3 2007/04/10 23:49:38 patthoyts Exp $

package require udp 1.0.6

proc udpEvent {chan} {
    set data [read $chan]
    set peer [fconfigure $chan -peer]
    set group [lindex [fconfigure $chan -remote] 0]
    puts "$peer ($group) [string length $data] '$data' {[fconfigure $chan]}"
    if {[string match "QUIT*" $data]} {
        close $chan
        set ::forever 1
    }
    return
}

# Select a multicast group and the port number.
#
# We have two groups here to show that it's possible.
#
set group1 224.5.1.21
set group2 224.5.2.21
set port   7771

# Create a listening socket and configure for sending too.
set s [udp_open $port]
fconfigure $s -buffering none -blocking 0
fconfigure $s -mcastadd $group2 -remote [list $group2 $port]
fconfigure $s -mcastadd $group1 -remote [list $group1 $port]
fileevent $s readable [list udpEvent $s]

# Announce our presence and run
puts -nonewline $s "hello, world"
set forever 0
vwait ::forever

exit
