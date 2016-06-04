##
# @file
# @brief QSPY front-end communication services
# @ingroup qspy
# @cond
#-----------------------------------------------------------------------------
# Product: QSPY -- QSPY interface package
# Last updated for version 5.6.4
# Last updated on  2016-05-27
#
#                    Q u a n t u m     L e a P s
#                    ---------------------------
#                    innovating embedded systems
#
# Copyright (C) Quantum Leaps, LLC, All rights reserved.
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
# http://www.state-machine.com
# mailto:info@state-machine.com
#-----------------------------------------------------------------------------
# @endcond

# Usage:
# this script is to be sourced into applications, such as qspyview.tcl

package provide qspy 5.6.4

package require Tcl  8.4   ;# need at least Tcl 8.4
package require udp        ;# need the UDP sockets for Tcl

namespace eval ::qspy {
    # exported commands...
    namespace export \
        attach \
        detach \
        sendPkt \
        sendAttach \
        sendEvent

    # variables...

    # communication parameters (can be overridden in command-line)
    variable theSocket     0 ;#< UDP socket for communication with QSPY
    variable theIsAttached false ;#< is the front-end attached to QSPY?

    variable theTstamp     "Target: UNKNOWN"

    variable thePkt          ;#< the current (most recent) packet/record

    variable theRxRecCnt   0 ;#< count of records received since attach
    variable theRxRecSeq   0 ;#< the last received record sequence num
    variable theErRecCnt   0 ;#< count of record errors since attach

    variable theRxPktCnt   0 ;#< count of packets received since attach
    variable theTxPktCnt   0 ;#< count of packets transmitted since attach
    variable theRxPktSeq   0 ;#< the last received packet sequence num
    variable theTxPktSeq   0 ;#< the last trasmitted packet sequence num
    variable theErPktCnt   0 ;#< count of packet errors since attach

    variable theFmt          ;#< array of formats recevied from QSPY
    array set theFmt {
        objPtr       4
        funPtr       4
        tstamp       4
        sig          2
        evtSize      2
        queueCtr     1
        poolCtr      2
        poolBlk      2
        tevtCtr      2
    }

    ## array of packet IDs (to QSPY Back-End, see also be.c in QSPY)
    variable BE
    array set BE {
        ATTACH       128
        DETACH       129
        SAVE_DIC     130
        SCREEN_OUT   131
        BIN_OUT      132
        MATLAB_OUT   133
        MSCGEN_OUT   134
    }

    ## array of record IDs (to Target, see also enum QSpyRxRecords in qs.h)
    variable QS_RX
    array set QS_RX {
        INFO         0
        COMMAND      1
        RESET        2
        TICK         3
        PEEK         4
        POKE         5
        RESERVED7    6
        RESERVED6    7
        RESERVED5    8
        RESERVED4    9
        GLB_FILTER   10
        LOC_FILTER   11
        AO_FILTER    12
        RESERVED3    13
        RESERVED2    14
        RESERVED1    15
        EVENT        16
    }

} ;# namespace

# command procedures =========================================================
proc ::qspy::attach {host port local_port} {
    variable theSocket
    variable theIsAttached

    set theIsAttached false

    # set-up the UDP socket for communication with QSPY...
    #set theSocket [udp_open $local_port]
    if {[catch {udp_open $local_port} theSocket]} {
        return false
    } else {
        fconfigure $theSocket -remote [list $host $port]
        fconfigure $theSocket -myport
        fconfigure $theSocket -buffering none
        fconfigure $theSocket -buffersize 4096
        fconfigure $theSocket -translation binary
        fconfigure $theSocket -blocking 1

        fileevent $theSocket readable [namespace code onRecvPkt]

        sendAttach ;# send the attach packet to QSPY

        return true
    }
}
#.............................................................................
proc ::qspy::detach {} {
    variable theSocket
    variable theIsAttached

    variable BE
    sendPkt [binary format c $BE(DETACH)]

    after 300   ;# wait until the packet comes out
    set theIsAttached false
    close $theSocket
    set theSocket 0 ;# invalidate the socket
}

# receive/send from the QSPY Back-End ========================================
proc ::qspy::onRecvPkt {} { ;# socket event handler
    variable theSocket
    variable thePkt

    set thePkt [read $theSocket] ;# read the packet from QSPY
    binary scan $thePkt cc seq pktId  ;# get the sequence num and packet-ID
    set seq [expr $seq & 0xFF]        ;# remove sign-extension

    if {$pktId >= 0} {   ;# is it QS record from the Target?
        variable theRxRecCnt
        incr theRxRecCnt ;# one more record received

        variable theRxRecSeq
        set theRxRecSeq [expr ($theRxRecSeq + 1) & 0xFF]
        if {$theRxRecSeq != $seq} {   ;# do we have a sequence error?
            set theRxRecSeq $seq
            variable theErRecCnt
            incr theErRecCnt
        }

        eval {rec$pktId} ;# call the record procedure (don't catch errors)

    } else {             ;# this is QSPY packet to the Front-End
        variable theRxPktCnt
        incr theRxPktCnt ;# one more packet received

        variable theRxPktSeq
        set theRxPktSeq [expr ($theRxPktSeq + 1) & 0xFF]
        if {$theRxPktSeq != $seq} {   ;# do we have a sequence error?
            set theRxPktSeq $seq
            variable theErPktCnt
            incr theErPktCnt
        }

        set pktId [expr $pktId & 0xFF] ;# remove the sign-extension
        eval {pkt$pktId} ;# call the packet procedure (don't catch errors)
    }
}
#.............................................................................
proc ::qspy::sendPkt {pkt} {
    variable theTxPktSeq
    incr theTxPktSeq  ;# increment the transmitted packet sequence number

    variable theSocket
    puts -nonewline $theSocket [binary format c $theTxPktSeq]$pkt

    variable theTxPktCnt
    incr theTxPktCnt ;# one more packet transmitted
}
#.............................................................................
proc ::qspy::sendAttach {} {
    variable BE
    sendPkt [binary format c $BE(ATTACH)] ;# send the attach command
}
#.............................................................................
proc ::qspy::sendEvent {prio sig par} {
    variable QS_RX
    variable theFmt
    set len [string length $par]
    sendPkt [binary format cc$theFmt(sig)s $QS_RX(EVENT) $prio $sig $len]$par
}

# QSPY packet handlers =======================================================
proc ::qspy::pkt128 {} { ;# QSPY_ATTACH from QSPY
    variable theIsAttached
    set theIsAttached true

    # send the INFO request to the Target
    variable QS_RX
    sendPkt [binary format c $QS_RX(INFO)]
}
#.............................................................................
proc ::qspy::pkt129 {} { ;# QSPY_DETACH from QSPY
    detach
}

# [60] Miscellaneous QS records ==============================================
#.............................................................................
proc ::qspy::rec64 {} { ;# QS_TARGET_INFO
    variable thePkt

    variable theIsAttached
    variable theTstamp
    variable theRxRecCnt
    variable theErRecCnt
    variable theRxPktCnt
    variable theTxPktCnt
    variable theErPktCnt

    binary scan $thePkt xxcsccccccccccccc \
        isReset   \
        qpVersion \
        b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 b10 b11 b12

    variable theFmt
    set fmt [list ? c s ? i ? ? ? w]
    set theFmt(objPtr)   [lindex $fmt [expr $b3 & 0x0F]]
    set theFmt(funPtr)   [lindex $fmt [expr $b3 >> 4]]
    set theFmt(tstamp)   [lindex $fmt [expr $b4 & 0x0F]]
    set theFmt(sig)      [lindex $fmt [expr $b0 & 0x0F]]
    set theFmt(evtSize)  [lindex $fmt [expr $b0 >> 4]]
    set theFmt(queueCtr) [lindex $fmt [expr $b1 & 0x0F]]
    set theFmt(poolCtr)  [lindex $fmt [expr $b2 >> 4]]
    set theFmt(poolBlk)  [lindex $fmt [expr $b2 & 0x0F]]
    set theFmt(tevtCtr)  [lindex $fmt [expr $b1 >> 4]]

    set theTstamp [format "Target: %02d%02d%02d_%02d%02d%02d" \
           [expr $b12 & 0xFF] [expr $b11 & 0xFF] [expr $b10 & 0xFF] \
           [expr $b9  & 0xFF] [expr $b8  & 0xFF] [expr $b7  & 0xFF]]

    # reset the counters for the new connection...
    set theIsAttached true
    set theRxRecCnt   0
    set theErRecCnt   0
    set theRxPktCnt   1
    set theErPktCnt   0

    if {$isReset} {
        eval {recRESET} ;# call the record procedure (don't catch errors)
    } else {
        eval {recINFO}  ;# call the record procedure (don't catch errors)
    }

    #dispTxt "QS_TARGET_INFO $theTstamp"
}


