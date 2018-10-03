##
# @file
# @ingroup qpspy
# @brief Communication services with the QSPY back-end.
# @usage
# This script is to be sourced into QSPY front-ends, such as qutest.tcl or
# qspyview.tcl

## @cond
#-----------------------------------------------------------------------------
# Product: QSPY -- QSPY interface package
# Last updated for version 6.3.6
# Last updated on  2018-10-03
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

package provide qspy 6.3

package require Tcl  8.4   ;# need at least Tcl 8.4
package require udp        ;# need the UDP sockets for Tcl

## @brief facilities for communication with the @ref qspy "QSPY" back-end
#
# @note
# The QSPY back-end distinguishes between two types of messages:@n
# 1. messages intended for QSPY only (e.g., attach/detach a front-end).
# These messages are called **packets**.
# 2. messages intended for the embedded Target, which the QSPY back-end
# merely passes to the QS component running inside the Target.
# These messages are called **records**.
#
namespace eval ::qspy {
    # exported commands...
    namespace export \
        attach \
        detach \
        udp_port \
        udp_remote \
        sendPkt \
        sendAttach \
        sendEvent

    # variables...

    # communication parameters (can be overridden in command-line)
    variable theSocket     0 ;#< UDP socket for communication with QSPY
    variable theIsAttached 0 ;#< is the front-end attached to QSPY?

    variable theHaveTarget 0 ;#< have target info
    variable theTstamp     "Target: UNKNOWN" ;#< build time stamp from Target

    variable thePkt          ;#< the current (most recent) packet/record

    variable theRxRecCnt   0 ;#< count of records received since attach
    variable theRxRecSeq   0 ;#< the last received record sequence num
    variable theErRecCnt   0 ;#< count of record errors since attach

    variable theRxPktCnt   0 ;#< count of packets received since attach
    variable theTxPktCnt   0 ;#< count of packets transmitted since attach
    variable theRxPktSeq   0 ;#< the last received packet sequence num
    variable theTxPktSeq   0 ;#< the last trasmitted packet sequence num
    variable theErPktCnt   0 ;#< count of packet errors since attach

    ## array of formats recevied from QS via QSPY
    variable theFmt
    array set theFmt {
        objPtr          i
        funPtr          i
        tstamp          i
        sig             s
        evtSize         s
        queueCtr        c
        poolCtr         s
        poolBlk         s
        tevtCtr         s
    }

    ## @brief array of record IDs (to Target).
    # @sa enum QSpyRxRecords in qs.h/qs_copy.h
    variable QS_RX
    array set QS_RX {
        INFO             0
        COMMAND          1
        RESET            2
        TICK             3
        PEEK             4
        POKE             5
        FILL             6
        TEST_SETUP       7
        TEST_TEARDOWN    8
        TEST_PROBE       9
        GLB_FILTER       10
        LOC_FILTER       11
        AO_FILTER        12
        CURR_OBJ         13
        CONTINUE         14
        QUERY_CURR       15
        EVENT            16
    }

    ## @brief array of QS object kinds (to Target).
    # @sa enum QSpyObjKind in qs_copy.h
    variable QS_OBJ_KIND
    array set QS_OBJ_KIND {
        SM               0
        AO               1
        MP               2
        EQ               3
        TE               4
        AP               5
        SM_AO            6
    }

    ## @brief array of packet IDs (to QSPY Back-End).
    # @sa enum QSpyCommands in qspy.h
    variable QSPY
    array set QSPY {
        ATTACH           128
        DETACH           129
        SAVE_DICT        130
        SCREEN_OUT       131
        BIN_OUT          132
        MATLAB_OUT       133
        MSCGEN_OUT       134
        SEND_EVENT       135
        SEND_LOC_FILTER  136
        SEND_CURR_OBJ    137
        SEND_COMMAND     138
        SEND_TEST_PROBE  139
    }

} ;# namespace eval

# command procedures =========================================================

## @brief attach to the QSPY back-end
#
# @description
# This function "attaches" a given script (front-end) to the QSPY back-end.
# If the UDP socket could open successfully, the function configures the
# socket and installs the onRecvPkt() procedure as the event handler for
# reading from the UDP socket.
#
# @param[in] host name or IP address of the host running QSPY back-end.
#            Optionally, the host name might contain the port number
#            provided immediately after ':', e.g.: 192.168.1.24:7705
# @param[in] local_port local UDP port at which to open connection to QSPY.
#            The default value of 0 means that the system will choose a port.
# @param[in] channels QSPY data channels (binary or/and text) to connect to.
#
# @returns
# true if the connection has been established or false if not.
#
# @sa onRecvPkt()
#
proc ::qspy::attach {host {local_port 0} {channels 1}} {
    variable theSocket
    variable theIsAttached

    set theIsAttached 0

    # open the UDP socket for communication with QSPY...
    if {$local_port} { ;# local_port specificed explicitly?
        set err [catch {udp_open $local_port} theSocket]
    } else { ;# local_port not specified--let the system choose a port
        set err [catch {udp_open} theSocket]
    }

    if {$err} {
        return false
    } else {
        # parse the host name and optional port number...
        set host_port [split $host :]
        if {[lindex $host_port 1] == ""} {
            lappend host_port 7701 ;# the default UDP port of QSPY
        }

        # configure the UDP socket...
        fconfigure $theSocket -remote $host_port
        fconfigure $theSocket -myport
        fconfigure $theSocket -buffering none
        fconfigure $theSocket -buffersize 4096
        fconfigure $theSocket -translation binary
        fconfigure $theSocket -blocking 1

        fileevent $theSocket readable [namespace code onRecvPkt]

        sendAttach $channels ;# send the attach packet to QSPY

        #puts "UDP local port [udp_conf $theSocket -myport]"

        return true
    }
}
#.............................................................................
## @brief detach from the QSPY back-end
#
# @description
# This function should be called to free up the connection to the QSPY
# back-end.
#
# @sa onCleanup()
#
proc ::qspy::detach {} {
    variable theSocket
    variable theIsAttached
    variable QSPY
    sendPkt [binary format c $QSPY(DETACH)]
    after 300   ;# wait until the packet comes out
    set theIsAttached 0
    close $theSocket
    set theSocket 0 ;# invalidate the socket
}
#.............................................................................
## @brief obtain the UDP port number
#
# @sa http://tcludp.sourceforge.net/#3
#
proc ::qspy::udp_port {} {
    variable theSocket
    return [udp_conf $theSocket -myport]
}
#.............................................................................
## @brief obtain the remote host and port number
#
# @sa http://tcludp.sourceforge.net/#3
#
proc ::qspy::udp_remote {} {
    variable theSocket
    return [udp_conf $theSocket -remote]
}


# receive/send from the QSPY Back-End ========================================
## @brief fileevent handler for the UDP socket to the QSPY back-end
#
# @description
# This function is called once per each UDP packet received from QSPY.
#
# @sa proc attach(), line:@n
# `fileevent $theSocket readable [namespace code onRecvPkt]`
proc ::qspy::onRecvPkt {} { ;# socket event handler
    variable theSocket
    variable thePkt

    set thePkt [read $theSocket] ;# read the packet from QSPY
    binary scan $thePkt cc seq pktId ;# get the sequence num and packet-ID
    set seq [expr $seq & 0xFF] ;# remove sign-extension

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
## @brief send a UDP packet to the QSPY back-end
#
# @description
# The function increments the sequence number of transmitted packets
# and pre-pends it in front of the binary packet data to form the
# complete UDP packet as expected by the QSPY back-end. This allows
# the QSPY back-end to detect any dropped packets (gaps in the sequence).
#
# @param[in] pkt packet binary data (created by `binary format ...`)
#
proc ::qspy::sendPkt {pkt} {
    variable theTxPktSeq
    incr theTxPktSeq  ;# increment the transmitted packet sequence number

    variable theSocket
    puts -nonewline $theSocket [binary format c $theTxPktSeq]$pkt

    variable theTxPktCnt
    incr theTxPktCnt ;# one more packet transmitted
}
#.............................................................................
## @brief send the `ATTACH` packet to the QSPY back-end
proc ::qspy::sendAttach {{channels 1}} {
    variable QSPY
    sendPkt [binary format cc $QSPY(ATTACH) $channels]
}
#.............................................................................
## @brief send the local-filter packet to the Target/QSPY
proc ::qspy::sendLocFilter {kind obj} {
    variable QS_RX
    variable QSPY
    variable theFmt
    variable QS_OBJ_KIND
    if [string is integer $obj] { ;# send directly to Target
        sendPkt [binary format cc$theFmt(objPtr) \
             $QS_RX(LOC_FILTER) $QS_OBJ_KIND($kind) $obj]
    } else { ;# set to QSPY to provide 'obj' from Obj Dictionary
        sendPkt [binary format cc$theFmt(objPtr) \
             $QSPY(SEND_LOC_FILTER) $QS_OBJ_KIND($kind) 0]$obj\0
    }
}
#.............................................................................
## @brief send the current-object packet to the Target/QSPY
proc ::qspy::sendCurrObj {kind obj} {
    variable QS_RX
    variable QSPY
    variable theFmt
    variable QS_OBJ_KIND
    if [string is integer $obj] { ;# send directly to Target
        sendPkt [binary format cc$theFmt(objPtr) \
             $QS_RX(CURR_OBJ) $QS_OBJ_KIND($kind) $obj]
    } else { ;# set to QSPY to provide 'obj' from Obj Dictionary
        sendPkt [binary format cc$theFmt(objPtr) \
             $QSPY(SEND_CURR_OBJ) $QS_OBJ_KIND($kind) 0]$obj\0
    }
}
#.............................................................................
## @brief send the query-current packet to the Target/QSPY
proc ::qspy::sendQueryCurr {kind} {
    variable QS_RX
    variable QSPY
    variable QS_OBJ_KIND
    sendPkt [binary format cc $QS_RX(QUERY_CURR) $QS_OBJ_KIND($kind)]
}

#.............................................................................
## @brief send the current-object packet to the Target/QSPY
proc ::qspy::sendTestProbe {fun data} {
    variable QS_RX
    variable QSPY
    variable theFmt
    if [string is integer $fun] { ;# set directly to Target
        sendPkt [binary format ci$theFmt(funPtr) \
                 $QS_RX(TEST_PROBE) $data $fun]
    } else { ;# set to QSPY to provide 'fun' from Fun Dictionary
        sendPkt [binary format ci$theFmt(funPtr) \
                 $QSPY(SEND_TEST_PROBE) $data 0]$fun\0
    }
}
#.............................................................................
## @brief send an event to the Target (via the QSPY back-end)
#
# @description
# The function sends a event to the embedded Target running QS-RX.
# The event is dynamically allocated inside the Target (with `Q_NEW()`),
# filled with the provided @p sig and @p par parameters, and posted
# to the AO object with the given priority @p prio if it is range
# (1..QF_MAX_ACTIVE). Additionally, the following special values of the
# @p prio parameter are as follows:@n
# - 255 dispatch event to the Current Object(SM)
# - 254 take the top-most initial transition in the Current Object (SM)
# - 253 post event to the Current Object (AO)
#
# @param[in] prio priority of the AO to receive the event or a special value
# @param[in] sig  signal of the event, either number or string from dictionary
# @param[in] par  event parameters (created by `binary format ...`)
#
# @note
# If provided, the optional @p par parameter must be provided in the native
# binary format expected by the spcific Target system. Specifically, this
# formatting must use the same endianness as the Target and must include any
# padding that the Target compiler might apply.
#
proc ::qspy::sendEvent {prio sig {par ""}} {
    variable QS_RX
    variable QSPY
    variable theFmt
    set len [string length $par]

    #puts "event Sig=$sig,Len=$len"
    if [string is integer $sig] { ;# send directly to Target
        sendPkt [binary format cc$theFmt(sig)s \
                 $QS_RX(EVENT) $prio $sig $len]$par
    } else {
        sendPkt [binary format cc$theFmt(sig)s \
            $QSPY(SEND_EVENT) $prio 0 $len]$par$sig\0
    }
}

# QSPY packet handlers =======================================================
#.............................................................................
## @brief handler for the `QSPY_DETACH` reply from QSPY.
proc ::qspy::pkt129 {} {
    detach
}

# [60] Miscellaneous QS records ==============================================
#.............................................................................
## @brief handler for the `QS_TARGET_INFO` record from the Target.
proc ::qspy::rec64 {} {
    variable thePkt

    variable theIsAttached
    variable theHaveTarget
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
    set theFmt(funPtr)   [lindex $fmt [expr ($b3 & 0xFF) >> 4]]
    set theFmt(tstamp)   [lindex $fmt [expr $b4 & 0x0F]]
    set theFmt(sig)      [lindex $fmt [expr $b0 & 0x0F]]
    set theFmt(evtSize)  [lindex $fmt [expr ($b0 & 0xFF) >> 4]]
    set theFmt(queueCtr) [lindex $fmt [expr $b1 & 0x0F]]
    set theFmt(poolCtr)  [lindex $fmt [expr ($b2 & 0xFF) >> 4]]
    set theFmt(poolBlk)  [lindex $fmt [expr $b2 & 0x0F]]
    set theFmt(tevtCtr)  [lindex $fmt [expr ($b1 & 0xFF) >> 4]]

    # reset the variables for the new connection...
    set theRxRecCnt   0
    set theErRecCnt   0
    set theRxPktCnt   1
    set theErPktCnt   0
    set theIsAttached 1
    set theHaveTarget 1
    set theTstamp [format "Target: %02d%02d%02d_%02d%02d%02d" \
           [expr $b12 & 0xFF] [expr $b11 & 0xFF] [expr $b10 & 0xFF] \
           [expr $b9  & 0xFF] [expr $b8  & 0xFF] [expr $b7  & 0xFF]]

    if {$isReset} {
        eval {recRESET} ;# call the record procedure (don't catch errors)
    } else {
        eval {recINFO}  ;# call the record procedure (don't catch errors)
    }
}
