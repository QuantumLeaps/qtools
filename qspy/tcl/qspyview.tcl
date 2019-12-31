##
# @file
# @ingroup qpspy
# @brief QSpyView front-end main script.
# @description
# QSpyView is a graphical visualization facility for embedded systems.
# This script implements a GUI-based front-end to the QSPY back-and for
# displaying the data coming from the Target system and for providing
# a GUI to the embedded Target.
#
# @usage
# wish qspyview.tcl [extension_script] [host[:port]] [local_port]

## @cond
#-----------------------------------------------------------------------------
# Product: QSPY -- GUI front-end to the QSPY host utility
# Last updated for version 6.6.0
# Last updated on  2019-11-30
#
#                    Q u a n t u m  L e a P s
#                    ------------------------
#                    Modern Embedded Software
#
# Copyright (C) 2005-2019 Quantum Leaps, LLC. All rights reserved.
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
# along with this program. If not, see <www.gnu.org/licenses/>.
#
# Contact information:
# <www.state-machine.com>
# <info@state-machine.com>
#-----------------------------------------------------------------------------
# @endcond

# this version of qspyview
set VERSION 6.6.0

package provide qspyview 6.6

package require Tcl  8.4     ;# need at least Tcl 8.4
package require Tk   8.4     ;# need at least Tk  8.4

# home directory of this Tcl script file
set HOME [file dirname [file normalize [info script]]]

source $HOME/qspy.tcl        ;# QSPY interface

# command procedures =========================================================
## @brief main entry point to the QSpyView front-end application
# @description
# This function processes the command-line arguments and attaches
# the QSpyView front-end to the indicated QSPY back-end.
proc main {} {
    # set defaults for communication with the QSPY back-end
    # NOTE (all these can be overridden by command-line options)
    set qspy_host  "localhost"
    set local_port 0 ;# the system will choose the local UDP port

    # command-line processing...
    # wish qspyview.tcl [extension_script] [host[:port]] [local_port]
    #
    global ::argc ::argv theHostExe
    if {$::argc > 1} {  ;# argv(1) -- host/port running QSPY
        set qspy_host [lindex $::argv 1]
    }
    if {$::argc > 2} {  ;# argv(2) -- local port
        set local_port [lindex $::argv 2]
    }

    # add a trace to monitor the QSPY socket counters
    trace add variable ::qspy::theSocket write traceSocket

    # add a trace to monitor the QSPY attachment state
    trace add variable ::qspy::theIsAttached write traceIsAttached

    # attach to QSPY...
    if {[::qspy::attach $qspy_host $local_port]} {
        after 200 ;# wait for the Target to respond to the info request
        update

        if {!$::qspy::theIsAttached} { ;# attached?
            onAttach ;# open the "Attach..." dialog box
        }
    } else {  ;# failed to attach
        tk_messageBox -type ok -icon error -message \
             "Can't open the UDP socket.\n\
              Check if another instance\n\
              of qspyview is already running."
        exit
    }

    # default setting of the QS Filters...
    global theGlbFilter
    set theGlbFilter \
        "0000000000000000000000000000000000000000000000000000000000000000"
    append theGlbFilter \
        "0000001111111111111111111111111111111111111111111111111111111000"

    global theLocFilter
    array set theLocFilter {
        SM   0
        AO   0
        MP   0
        EQ   0
        TE   0
        AP   0
    }

    global theCurrObj
    array set theCurrObj {
        SM   0
        AO   0
        MP   0
        EQ   0
        TE   0
        AP   0
    }

    global theAoFilter
    set theAoFilter       0 ;# filter open to all AOs

    global theCommand
    array set theCommand {
        cmdId  0
        param1 0
        param2 0
        param3 0
    }

    global theEvent
    array set theEvent {
        prio   0
        sig    0
        fmt1   ""   dat1   ""
        fmt2   ""   dat2   ""
        fmt3   ""   dat3   ""
        fmt4   ""   dat4   ""
        fmt5   ""   dat5   ""
        fmt6   ""   dat6   ""
        fmt7   ""   dat7   ""
        fmt8   ""   dat8   ""
        fmt9   ""   dat9   ""
        fmt10  ""   dat10  ""
    }

    global thePeek
    array set thePeek {
        obj  0
        offs 0
        size 1
        len  0
    }

    global thePoke
    array set thePoke {
        obj   0
        offs  0
        fmt   c
        data  0
    }
}
#.............................................................................
## @brief handler for the `QSPY_ATTACH` reply from QSPY.
proc ::qspy::pkt128 {} {
    variable theIsAttached
    set theIsAttached 1

    # send the INFO request to the Target
    variable QS_RX
    sendPkt [binary format c $QS_RX(INFO)]
}
#.............................................................................
## @brief trace handler to monitor the QSPY socket counters (see main())
proc traceSocket {varname key op} {
    upvar $varname var
    if {$var == 0} { ;# socket invalidated?
        exit
    }
}
#.............................................................................
## @brief trace handler to monitor the QSPY attachment state (see main())
proc traceIsAttached {varname key op} {
    upvar $varname var
    if {$var} {
        variable ::dialog::done
        set ::dialog::done 2  ;# will cause exit from the onAttach dialog
    }
}

#.............................................................................
## @brief cleanup before exiting the QSpyView front-end
proc onCleanup {} {
    ::qspy::detach ;# detach from QSPY
    exit
}
#.............................................................................
## @brief displays the "About" dialog box for QSpyView
proc onAbout {} {
    global argv VERSION

    set top [::dialog::create .aboutDlg "About QspyView"]
    set form $top.form

    label $form.logo -image ::img::logo128
    pack $form.logo -side left

    label $form.lbl1 -text "QSpyView front-end $VERSION"
    pack $form.lbl1 -side top
    label $form.lbl2 -text "Extension: [join $argv]"
    pack $form.lbl2 -side top
    label $form.lbl3 -text "Documentation:\nhttps://state-machine.com/qspy/"
    pack $form.lbl3 -side top
    label $form.lbl4 -text \
        "Copyright (c) 2005-2018 Quantum Leaps\nstate-machine.com"
    pack $form.lbl4 -side top

    variable ::dialog::done
    button $top.controls.btn0 -text "   OK   " -command {set ::dialog::done 0}
    pack $top.controls.btn0 -side right -expand yes
    bind $top <KeyPress-Escape> {set ::dialog::done 0}
    bind $top <KeyPress-Return> {set ::dialog::done 0}

    ::dialog::wait $top
    destroy $top
}
#.............................................................................
## @brief Attaches QSpyView front-end to the QSPY back-end
# @description
# This function uses the information provided to QSpyView in the
# command-line parameters, such as host, port and local_port
proc onAttach {} {
    set top [::dialog::create .attachDlg "Attach to QSPY"]
    set form $top.form
    label $form.lbl1 -text "Make sure that QSPY is running"
    pack $form.lbl1 -side top
    label $form.lbl2 -text "and the option -u is used."
    pack $form.lbl2 -side top
    label $form.lbl3 -text "Press OK to re-try to attach or"
    pack $form.lbl3 -side top
    label $form.lbl4 -text "Cancel to quit."
    pack $form.lbl4 -side top

    variable ::dialog::done
    button $top.controls.btn0 -text " Cancel " -command {set ::dialog::done 0}
    pack $top.controls.btn0 -side right -expand yes
    button $top.controls.btn1 -text "   OK   " -command {set ::dialog::done 1}
    pack $top.controls.btn1 -side right -expand yes
    bind $top <KeyPress-Escape> {set ::dialog::done 0}
    bind $top <KeyPress-Return> {set ::dialog::done 1}

    # perform cleanup upon closing this dialog
    wm protocol $top WM_DELETE_WINDOW onCleanup

    ::qspy::sendAttach ;# re-send the attach command

    set retry true
    while {$retry} {
        set done [::dialog::wait $top]

        if {$done == 0} { ;# Cancel pressed
            onCleanup
            set retry false
        } elseif {$done == 1} { ;# OK
            ::qspy::sendAttach ;# re-send the attach command
        } else { ;# attached to QSPY
            set retry false
        }
    }
    destroy $top
}
#.............................................................................
## @brief displays the "Global Filters" dialog box for QSpyView
proc onGlbFilters {} {

    set labels [list \
        QS_EMPTY               STATE_ENTRY       \
        STATE_EXIT             STATE_INIT        \
        INIT_TRAN              INTERN_TRAN       \
        TRAN                   IGNORED           \
        DISPATCH               UNHANDLED         \
        ACTIVE_DEFER           ACTIVE_RECALL     \
        ACTIVE_SUBSCRIBE       ACTIVE_UNSUBSCR   \
        ACTIVE_POST_FIFO       ACTIVE_POST_LIFO  \
        ACTIVE_GET             ACTIVE_GET_LAST   \
        ACTIVE_RECALL_ATMPT    EQUEUE_POST_FIFO  \
        EQUEUE_POST_LIFO       EQUEUE_GET        \
        EQUEUE_GET_LAST        RESERVED2         \
        MPOOL_GET              MPOOL_PUT         \
        QF_PUBLISH             QF_NEW_REF        \
        Q_NEW                  QF_GC_ATTEMPT     \
        QF_GC                  QF_TICK           \
        TIMEEVT_ARM            TIMEEVT_AUTO_DARM \
        TIMEEVT_DISARM_ATMPT   TIMEEVT_DISARM    \
        TIMEEVT_REARM          TIMEEVT_POST      \
        QF_DELETE_REF          QF_CRIT_ENTRY     \
        QF_CRIT_EXIT           QF_ISR_ENTRY      \
        QF_ISR_EXIT            QF_INT_DISABLE    \
        QF_INT_ENABLE          ACTIVE_POST_ATMPT \
        EQUEUE_POST_ATMPT      MPOOL_GET_ATMPT   \
        MUTEX_LOCK             MUTEX_UNLOCK      \
        SCHED_LOCK             SCHED_UNLOCK      \
        SCHED_NEXT             SCHED_IDLE        \
        SCHED_RESUME           TRAN_HIST         \
        TRAN_EP                TRAN_XP           \
        TEST_PAUSED            TEST_PROBE_GET    \
        QS_SIG_DICT            QS_OBJ_DICT       \
        QS_FUN_DICT            QS_USR_DICT       \
        QS_TARGET_INFO         QS_TARGET_DONE    \
        QS_RX_STATUS           QS_MSC_RESERVED1  \
        QS_PEEK_DATA           QS_ASSERT_FAIL    \
    ]
    global theGlbFilter
    set top [::dialog::create .glbFilter "Global Filters"]
    set form $top.form

    # scrollable content...
    scrollbar $form.sbar -command "$form.vport yview"
    pack $form.sbar -side right -fill y
    canvas $form.vport -height 410 -yscrollcommand "$form.sbar set"
    pack $form.vport -side top -fill both -expand true

    set pane [frame $form.vport.pane]
    $form.vport create window 0 0 -anchor nw -window $form.vport.pane
    bind $form.vport.pane <Configure> "filterDlg_resize $form"

    global filter
    for {set i 1} {$i < 60} {incr i} {
        checkbutton $pane.f$i -text [lindex $labels $i] -variable filter($i)
        if {[string index $theGlbFilter $i]} {
            $pane.f$i select
        }
    }
    for {set i 100} {$i < 125} {incr i} {
        checkbutton $pane.f$i -variable filter($i) \
            -text "QS_USER($i)"
        if {[string index $theGlbFilter $i]} {
            $pane.f$i select
        }
    }

    label $pane.std -text "===== Standard QS recrods ====="
    label $pane.sm  -text "SM records..."
    label $pane.ao  -text "AO records..."
    label $pane.qf  -text "QF records..."
    label $pane.te  -text "TE records..."
    label $pane.eq  -text "EQ records..."
    label $pane.mp  -text "MP records..."
    label $pane.sc  -text "SC records..."
    label $pane.tst -text "TEST records..."
    grid $pane.std -columnspan 6
    grid $pane.sm  $pane.ao  $pane.qf  $pane.eq  $pane.te
    grid $pane.f1  $pane.f10 $pane.f26 $pane.f19 $pane.f32
    grid $pane.f2  $pane.f11 $pane.f27 $pane.f20 $pane.f33
    grid $pane.f3  $pane.f12 $pane.f28 $pane.f21 $pane.f34
    grid $pane.f4  $pane.f13 $pane.f29 $pane.f22 $pane.f35
    grid $pane.f5  $pane.f14 $pane.f30 $pane.f46 $pane.f36
    grid $pane.f6  $pane.f15 $pane.f31 $pane.sc  $pane.f37
    grid $pane.f7  $pane.f16 $pane.f38 $pane.f48 $pane.mp
    grid $pane.f8  $pane.f17 $pane.f39 $pane.f49 $pane.f24
    grid $pane.f9  $pane.f18 $pane.f40 $pane.f50 $pane.f25
    grid $pane.f55 $pane.f45 $pane.f41 $pane.f51 $pane.f47
    grid $pane.f56 x         $pane.f42 $pane.f52 $pane.tst
    grid $pane.f57 $pane.f23 $pane.f43 $pane.f53 $pane.f58
    grid x         x $pane.f44 $pane.f54 $pane.f59

    label $pane.ua -text "===== User QS recrods ====="
    label $pane.u0  -text "U0 records..."
    label $pane.u1  -text "U1 records..."
    label $pane.u2  -text "U2 records..."
    label $pane.u3  -text "U3 records..."
    label $pane.u4  -text "U4 records..."
    grid $pane.ua -columnspan 5
    grid $pane.u0   $pane.u1  $pane.u2  $pane.u3   $pane.u4
    grid $pane.f100 $pane.f105 $pane.f110 $pane.f115 $pane.f120
    grid $pane.f101 $pane.f106 $pane.f111 $pane.f116 $pane.f121
    grid $pane.f102 $pane.f107 $pane.f112 $pane.f117 $pane.f122
    grid $pane.f103 $pane.f108 $pane.f113 $pane.f118 $pane.f123
    grid $pane.f104 $pane.f109 $pane.f114 $pane.f119 $pane.f124

    for {set i 1} {$i < 60} {incr i} {
        grid configure $pane.f$i -sticky w
    }
    for {set i 100} {$i < 125} {incr i} {
        grid configure $pane.f$i -sticky w
    }
    grid configure $pane.sm -sticky w
    grid configure $pane.ao -sticky w
    grid configure $pane.qf -sticky w
    grid configure $pane.te -sticky w
    grid configure $pane.eq -sticky w
    grid configure $pane.mp -sticky w
    grid configure $pane.sc -sticky w
    grid configure $pane.tst -sticky w

    grid configure $pane.u0 -sticky w
    grid configure $pane.u1 -sticky w
    grid configure $pane.u2 -sticky w
    grid configure $pane.u3 -sticky w
    grid configure $pane.u4 -sticky w

    variable ::dialog::done
    button $top.controls.btn0 -text " Cancel " -command {set ::dialog::done 0}
    pack $top.controls.btn0 -side right -expand yes
    button $top.controls.btn1 -text "   OK   " -command {set ::dialog::done 1}
    pack $top.controls.btn1 -side right -expand yes
    bind $top <KeyPress-Escape> {set ::dialog::done 0}
    bind $top <KeyPress-Return> {set ::dialog::done 1}

    # wait for the user selection
    set done [::dialog::wait $top]

    if {$done == 1} { ;# dialog accepted?
        set theGlbFilter ""
        for {set i 0} {$i < 128} {incr i} {
            if [info exists filter($i)] {
                if {$filter($i)} {
                    append theGlbFilter 1
                } else {
                    append theGlbFilter 0
                }
            } else {
                append theGlbFilter 0
            }
        }
        variable ::qspy::QS_RX
        ::qspy::sendPkt \
            [binary format ccb128 $::qspy::QS_RX(GLB_FILTER) \
                    16 $theGlbFilter]
    }
    destroy $top
}
#.............................................................................
## @brief helper proc for the scrollable Filters dialog box
proc filterDlg_resize {win} {
    set bbox [$win.vport bbox all]
    set wid [winfo width $win.vport.pane]
    $win.vport configure -width $wid \
        -scrollregion $bbox -yscrollincrement 0.2i
}

#.............................................................................
## @brief displays the "Local Filters" dialog box for QSpyView
proc onLocFilters {} {
    set top [::dialog::create .locFilters "Local Filters"]
    set form $top.form

    global theLocFilter
    ::dialog::addEntry $top "QS_FILTER_SM_OBJ:" 16 $theLocFilter(SM)
    ::dialog::addEntry $top "QS_FILTER_AO_OBJ:" 16 $theLocFilter(AO)
    ::dialog::addEntry $top "QS_FILTER_MP_OBJ:" 16 $theLocFilter(MP)
    ::dialog::addEntry $top "QS_FILTER_EQ_OBJ:" 16 $theLocFilter(EQ)
    ::dialog::addEntry $top "QS_FILTER_TE_OBJ:" 16 $theLocFilter(TE)
    ::dialog::addEntry $top "QS_FILTER_AP_OBJ:" 16 $theLocFilter(AP)

    variable ::dialog::done
    button $top.controls.btn0 -text " Cancel " -command {set ::dialog::done 0}
    pack $top.controls.btn0 -side right -expand yes
    button $top.controls.btn1 -text "   OK   " -command {set ::dialog::done 1}
    pack $top.controls.btn1 -side right -expand yes
    bind $top <KeyPress-Escape> {set ::dialog::done 0}
    bind $top <KeyPress-Return> {set ::dialog::done 1}

    set done [::dialog::wait $top]

    if {$done == 1} { ;# dialog accepted?
        set theLocFilter(SM) [$form.entry1 get]
        set theLocFilter(AO) [$form.entry2 get]
        set theLocFilter(MP) [$form.entry3 get]
        set theLocFilter(EQ) [$form.entry4 get]
        set theLocFilter(TE) [$form.entry5 get]
        set theLocFilter(AP) [$form.entry6 get]

        ::qspy::sendLocFilter SM $theLocFilter(SM)
        ::qspy::sendLocFilter AO $theLocFilter(AO)
        ::qspy::sendLocFilter MP $theLocFilter(MP)
        ::qspy::sendLocFilter EQ $theLocFilter(EQ)
        ::qspy::sendLocFilter TE $theLocFilter(TE)
        ::qspy::sendLocFilter AP $theLocFilter(AP)
    }
    destroy $top
}
#.............................................................................
## @brief displays the "Current Objects" dialog box for QSpyView
proc onCurrObjs {} {
    set top [::dialog::create .currObj "Current Objects"]
    set form $top.form

    global theCurrObj
    ::dialog::addEntry $top "SM:" 16 $theCurrObj(SM)
    ::dialog::addEntry $top "AO:" 16 $theCurrObj(AO)
    ::dialog::addEntry $top "MP:" 16 $theCurrObj(MP)
    ::dialog::addEntry $top "EQ:" 16 $theCurrObj(EQ)
    ::dialog::addEntry $top "TE:" 16 $theCurrObj(TE)
    ::dialog::addEntry $top "AP:" 16 $theCurrObj(AP)

    variable ::dialog::done
    button $top.controls.btn0 -text " Cancel " -command {set ::dialog::done 0}
    pack $top.controls.btn0 -side right -expand yes
    button $top.controls.btn1 -text "   OK   " -command {set ::dialog::done 1}
    pack $top.controls.btn1 -side right -expand yes
    bind $top <KeyPress-Escape> {set ::dialog::done 0}
    bind $top <KeyPress-Return> {set ::dialog::done 1}

    set done [::dialog::wait $top]

    if {$done == 1} { ;# dialog accepted?
        set theCurrObj(SM) [$form.entry1 get]
        set theCurrObj(AO) [$form.entry2 get]
        set theCurrObj(MP) [$form.entry3 get]
        set theCurrObj(EQ) [$form.entry4 get]
        set theCurrObj(TE) [$form.entry5 get]
        set theCurrObj(AP) [$form.entry6 get]

        ::qspy::sendCurrObj SM $theCurrObj(SM)
        ::qspy::sendCurrObj AO $theCurrObj(AO)
        ::qspy::sendCurrObj MP $theCurrObj(MP)
        ::qspy::sendCurrObj EQ $theCurrObj(EQ)
        ::qspy::sendCurrObj TE $theCurrObj(TE)
        ::qspy::sendCurrObj AP $theCurrObj(AP)
    }
    destroy $top
}
#.............................................................................
## @brief displays the "Active-Object Filter" dialog box for QSpyView
proc onAoFilter {} {
    set top [::dialog::create .aoFilter "AO Filter"]
    set form $top.form

    global theAoFilter
    ::dialog::addEntry  $top "prio:" 3 $theAoFilter

    variable ::dialog::done
    button $top.controls.btn0 -text " Cancel " -command {set ::dialog::done 0}
    pack $top.controls.btn0 -side right -expand yes
    button $top.controls.btn1 -text "   OK   " -command {set ::dialog::done 1}
    pack $top.controls.btn1 -side right -expand yes
    bind $top <KeyPress-Escape> {set ::dialog::done 0}
    bind $top <KeyPress-Return> {set ::dialog::done 1}

    focus $form.entry1 ;# so that the user can immediately edit prio

    set done [::dialog::wait $top]

    if {$done == 1} { ;# dialog accepted?
        set theAoFilter [$form.entry1 get]

        variable ::qspy::QS_RX
        ::qspy::sendPkt \
            [binary format cc $::qspy::QS_RX(AO_FILTER) $theAoFilter]
    }
    destroy $top
}
#.............................................................................
## @brief displays the "Command to Target" dialog box for QSpyView
proc onCommand {} {
    set top [::dialog::create .command "Command"]
    set form $top.form

    global theCommand
    ::dialog::addEntry  $top "cmdId :" 6  $theCommand(cmdId)
    ::dialog::addEntry  $top "param1:" 16 $theCommand(param1)
    ::dialog::addEntry  $top "param2:" 16 $theCommand(param2)
    ::dialog::addEntry  $top "param3:" 16 $theCommand(param3)

    variable ::dialog::done
    button $top.controls.btn0 -text " Cancel " -command {set ::dialog::done 0}
    pack $top.controls.btn0 -side right -expand yes
    button $top.controls.btn1 -text "   OK   " -command {set ::dialog::done 1}
    pack $top.controls.btn1 -side right -expand yes
    bind $top <KeyPress-Escape> {set ::dialog::done 0}
    bind $top <KeyPress-Return> {set ::dialog::done 1}

    focus $form.entry1 ;# so that the user can immediately edit cmdId

    set done [::dialog::wait $top]

    if {$done == 1} { ;# dialog accepted?
        set theCommand(cmdId)  [$form.entry1 get]
        set theCommand(param1) [$form.entry2 get]
        set theCommand(param2) [$form.entry3 get]
        set theCommand(param3) [$form.entry4 get]

        variable ::qspy::QS_RX
        ::qspy::sendPkt [binary format cciii \
            $::qspy::QS_RX(COMMAND) $theCommand(cmdId) \
            $theCommand(param1) $theCommand(param2) $theCommand(param3)]
    }
    destroy $top
}
#.............................................................................
## @brief displays the "Peek" dialog box for QSpyView
proc onPeek {} {
    set top [::dialog::create .peek "Peek"]
    set form $top.form

    global thePeek
    ::dialog::addEntry  $top "obj/addr:"    16 $thePeek(obj)
    ::dialog::addEntry  $top "offset:"      16 $thePeek(offs)
    ::dialog::addEntry  $top "size(1/2/4):"  6 $thePeek(size)
    ::dialog::addEntry  $top "n-units:"      6 $thePeek(len)

    variable ::dialog::done
    button $top.controls.btn0 -text " Cancel " -command {set ::dialog::done 0}
    pack $top.controls.btn0 -side right -expand yes
    button $top.controls.btn1 -text "   OK   " -command {set ::dialog::done 1}
    pack $top.controls.btn1 -side right -expand yes
    bind $top <KeyPress-Escape> {set ::dialog::done 0}
    bind $top <KeyPress-Return> {set ::dialog::done 1}

    focus $form.entry1 ;# so that the user can immediately edit the obj/addr

    set done [::dialog::wait $top]

    if {$done == 1} { ;# dialog accepted?
        set thePeek(obj)  [$form.entry1 get]
        set thePeek(offs) [$form.entry2 get]
        set thePeek(size) [$form.entry3 get]
        set thePeek(len)  [$form.entry4 get]

        ::qspy::sendCurrObj AP $thePeek(obj)
        variable ::qspy::QS_RX
        ::qspy::sendPkt \
            [binary format cscc $::qspy::QS_RX(PEEK) \
            $thePeek(offs) $thePeek(size) $thePeek(len)]
    }
    destroy $top
}
#.............................................................................
## @brief displays the "Poke" dialog box for QSpyView
proc onPoke {} {
    set top [::dialog::create .poke "Poke"]
    set form $top.form

    global thePoke
    ::dialog::addEntry  $top "obj/addr:"    16 $thePoke(obj)
    ::dialog::addEntry  $top "offset:"      16 $thePoke(offs)
    ::dialog::addEntry  $top "fmt(c/s/S/i/I):" 6 $thePoke(fmt)
    ::dialog::addEntry  $top "data:"        16 $thePoke(data)

    variable ::dialog::done
    button $top.controls.btn0 -text " Cancel " -command {set ::dialog::done 0}
    pack $top.controls.btn0 -side right -expand yes
    button $top.controls.btn1 -text "   OK   " -command {set ::dialog::done 1}
    pack $top.controls.btn1 -side right -expand yes
    bind $top <KeyPress-Escape> {set ::dialog::done 0}
    bind $top <KeyPress-Return> {set ::dialog::done 1}

    focus $form.entry1  ;# so that the user can immediately edit the addr

    set done [::dialog::wait $top]

    if {$done == 1} { ;# dialog accepted?
        set thePoke(obj)  [$form.entry1 get]
        set thePoke(offs) [$form.entry2 get]
        set thePoke(fmt)  [$form.entry3 get]
        set thePoke(data) [$form.entry4 get]

        ::qspy::sendCurrObj AP $thePoke(obj)
        variable ::qspy::QS_RX
        set data [binary format $thePoke(fmt) $thePoke(data)]
        set size [string length $data]
        ::qspy::sendPkt [binary format cscc $::qspy::QS_RX(POKE) \
                         $thePoke(offs) $size 1]$data
    }
    destroy $top
}
#.............................................................................
## @brief handler for the Tick command (trigger a clock tick inside Target)
proc onTick {rate} {
    variable ::qspy::QS_RX
    ::qspy::sendPkt \
        [binary format cc $::qspy::QS_RX(TICK) $rate]
}
#.............................................................................
## @brief handler for the Target-Reset command (trigger reset inside Target)
proc onTargetReset {} {
    variable ::qspy::QS_RX
    ::qspy::sendPkt [binary format c $::qspy::QS_RX(RESET)]
}
#.............................................................................
## @brief handler for the Target-Info command (trigger sending Target info)
proc onTargetInfo {} {
    variable ::qspy::QS_RX
    ::qspy::sendPkt [binary format c $::qspy::QS_RX(INFO)]
}
#.............................................................................
## @brief handler for the Save-Dictionary command
# (make QSPY save dictionaries)
proc onSaveDict {} {
    variable ::qspy::QSPY
    ::qspy::sendPkt [binary format c $::qspy::QSPY(SAVE_DICT)]
}
#.............................................................................
## @brief handler for the Screen-Out command
proc onScreenOut {} {
    variable ::qspy::QSPY
    ::qspy::sendPkt [binary format c $::qspy::QSPY(SCREEN_OUT)]
}
#.............................................................................
## @brief handler for the Binary-Out command
# (toggle saving binary file in QSPY)
proc onBinaryOut {} {
    variable ::qspy::QSPY
    ::qspy::sendPkt [binary format c $::qspy::QSPY(BIN_OUT)]
}
#.............................................................................
## @brief handler for the Matlab-Out command
# (toggle saving Matlab file in QSPY)
proc onMatlabOut {} {
    variable ::qspy::QSPY
    ::qspy::sendPkt [binary format c $::qspy::QSPY(MATLAB_OUT)]
}
#.............................................................................
## @brief handler for the MscGen-Out command
# (toggle saving MscGen file in QSPY)
proc onMscGenOut  {} {
    variable ::qspy::QSPY
    ::qspy::sendPkt [binary format c $::qspy::QSPY(MSCGEN_OUT)]
}

#.............................................................................
## @brief displays the "Generate Event" dialog box for QSpyView
proc onGenEvent {} {
    set top [::dialog::create .genEvent "Generate Event"]
    set form $top.form

    global theEvent

    label $form.label1 -text "prio:" -anchor e
    entry $form.entry1 -width 8
    $form.entry1 insert 0 $theEvent(prio)
    grid $form.label1 $form.entry1 -

    grid configure $form.label1 -sticky e
    grid configure $form.entry1 -sticky w

    label $form.label2 -text "sig:" -anchor e
    entry $form.entry2 -width 21
    $form.entry2 insert 0 $theEvent(sig)
    grid $form.label2 $form.entry2 -

    grid configure $form.label2 -sticky e
    grid configure $form.entry2 -sticky w

    label $top.form.params -text "===== Event Parameters ====="
    grid $top.form.params -columnspan 3

    for {set i 1} {$i <= 10} {incr i} {
        label $form.par$i -text "par$i:" -anchor e
        entry $form.fmt$i -width 4
        entry $form.dat$i -width 16
        $form.fmt$i insert 0 $theEvent(fmt$i)
        $form.dat$i insert 0 $theEvent(dat$i)
        grid $form.par$i $form.fmt$i $form.dat$i

        grid configure $form.par$i -sticky e
        grid configure $form.fmt$i -sticky w
        grid configure $form.dat$i -sticky w
    }

    variable ::dialog::done
    button $top.controls.btn0 -text " Cancel " -command {set ::dialog::done 0}
    pack $top.controls.btn0 -side right -expand yes
    button $top.controls.btn1 -text "   OK   " -command {set ::dialog::done 1}
    pack $top.controls.btn1 -side right -expand yes
    bind $top <KeyPress-Escape> {set ::dialog::done 0}
    bind $top <KeyPress-Return> {set ::dialog::done 1}

    focus $form.entry1 ;# so that the user can immediately type prio

    set done [::dialog::wait $top]

    if {$done == 1} { ;# dialog accepted?

        set theEvent(prio) [$form.entry1 get]
        set theEvent(sig)  [$form.entry2 get]

        set par ""
        for {set i 1} {$i <= 10} {incr i} {
            set theEvent(fmt$i) [$form.fmt$i get]
            set theEvent(dat$i) [$form.dat$i get]

            if {$theEvent(fmt$i) != ""} {
                append par [binary format $theEvent(fmt$i) $theEvent(dat$i)]
            }
        }

        ::qspy::sendEvent $theEvent(prio) $theEvent(sig) $par
    }
    destroy $top
}

# GUI ========================================================================
# graphics defaults ..........................................................
#option add *ctrlFont -*-courier-medium-r-normal--*-140-* userDefault
option add *textFont -*-courier-medium-r-semicondensed--*-110-* userDefault

set theTextFont [option get . textFont TextFont] ;# font used for text

option add *t.font $theTextFont userDefault

# embedded images ............................................................
image create photo ::img::logo32    -file $HOME/img/logo_qspy32.gif
image create photo ::img::logo32off -file $HOME/img/logo_qp32.gif
image create photo ::img::logo64    -file $HOME/img/logo_qspy64.gif
image create photo ::img::logo128   -file $HOME/img/logo_qspy128.gif

# configure the top-level window .............................................
wm protocol . WM_DELETE_WINDOW onCleanup

# Main menu ==================================================================
. configure -menu .mbar
menu .mbar
.mbar add cascade -label "File"     -menu .mbar.file
.mbar add cascade -label "View"     -menu .mbar.view
.mbar add cascade -label "Filters"  -menu .mbar.filters
.mbar add cascade -label "Commands" -menu .mbar.commands
.mbar add cascade -label "Events"   -menu .mbar.events
.mbar add cascade -label "Custom"   -menu .mbar.cust
.mbar add cascade -label "Help"     -menu .mbar.help

# sub-menu File --------------------------------------------------------------
menu .mbar.file -tearoff 0
# 0
.mbar.file add command -label "Store Dictionaries" -accelerator Ctrl+d \
    -command onSaveDict
bind . <Control-d> { .mbar.file invoke 0 }
# 1
.mbar.file add command -label "Screen Output" -accelerator Ctrl+o \
    -command onScreenOut
bind . <Control-o> { .mbar.file invoke 1 }
# 2
.mbar.file add command -label "Save QS Binary" -accelerator Ctrl+s \
    -command onBinaryOut
bind . <Control-s> { .mbar.file invoke 2 }
# 3
.mbar.file add command -label "Matlab Output" \
    -command onMatlabOut
# 3
.mbar.file add command -label "MscGen Output" \
    -command onMscGenOut
# 4
.mbar.file add separator
# 5
.mbar.file add command -label "Exit" -accelerator Alt+F4 -command onCleanup
bind . <Alt-F4> { .mbar.file invoke 5 }

# sub-menu View --------------------------------------------------------------
set theSigView 0
menu .mbar.view -tearoff 0
# 0
.mbar.view add checkbutton -label "Text" -variable theTextView \
    -command onTextView
# 1
.mbar.view add checkbutton -label "Canvas" -variable theCanvView \
    -command onCanvView

# sub-menu Filters -----------------------------------------------------------
menu .mbar.filters -tearoff 0
# 0
.mbar.filters add command -label "Globl Filters..." -accelerator Ctrl+g \
    -command onGlbFilters
bind . <Control-g> { .mbar.filters invoke 0 }
# 1
.mbar.filters add command -label "Local Filters..." -accelerator Ctrl+l \
    -command onLocFilters
bind . <Control-l> { .mbar.filters invoke 1 }
# 2
.mbar.filters add command -label "AO Filter..." -command onAoFilter

# sub-menu Commands ----------------------------------------------------------
menu .mbar.commands -tearoff 0
# 0
.mbar.commands add command -label "Reset Target" -accelerator Ctrl+r \
    -command onTargetReset
bind . <Control-r> { .mbar.commands invoke 0 }
# 1
.mbar.commands add command -label "Query Target Info" -accelerator Ctrl+i \
    -command onTargetInfo
bind . <Control-i> { .mbar.commands invoke 1 }
# 2
.mbar.commands add command -label "Tick\[0\]" -accelerator Ctrl+t \
    -command { onTick 0 }
bind . <Control-t> { .mbar.commands invoke 2 }
# 3
.mbar.commands add command -label "Tick\[1\]" -accelerator Ctrl+u \
    -command { onTick 1 }
bind . <Control-u> { .mbar.commands invoke 3 }
# 4
.mbar.commands add command -label "User Command..." -accelerator Ctrl+c \
    -command onCommand
bind . <Control-c> { .mbar.commands invoke 4 }
# 5
.mbar.commands add command -label "Peek Obj/Addr..." -command onPeek
# 6
.mbar.commands add command -label "Poke Obj/Addr..." -command onPoke
# 7
.mbar.commands add separator

# sub-menu Events ------------------------------------------------------------
# 0
menu .mbar.events -tearoff 0
.mbar.events add command -label "Current Objects..." -accelerator Ctrl+o \
    -command onCurrObjs
bind . <Control-l> { .mbar.events invoke 0 }
# 1
.mbar.events add command -label "Generate Event..." -accelerator Ctrl+e \
    -command onGenEvent
bind . <Control-e> { .mbar.events invoke 1 }


# sub-menu Cust --------------------------------------------------------------
menu .mbar.cust -tearoff 0

# sub-menu Help --------------------------------------------------------------
menu .mbar.help -tearoff 0

# 0
.mbar.help add command -label "Online Help" -accelerator F1 -command {
    openURL "https://state-machine.com/qspy/"
}
bind . <F1> { .mbar.help invoke 0 }

# 1
.mbar.help add separator
# 2
.mbar.help add command -label "About ..." -command onAbout

# status bar =================================================================
frame .status
label .status.tstamp -text $::qspy::theTstamp -anchor w
label .status.erlb1 -text "Err" -anchor w -borderwidth 1
label .status.erlb2 -text "Err" -anchor w -borderwidth 1
label .status.er1   -anchor w -width 4 -relief sunken -borderwidth 1
label .status.er2   -anchor w -width 4 -relief sunken -borderwidth 1
label .status.rxlb1 -text "Rx" -anchor w -borderwidth 1
label .status.rxlb2 -text "Rx" -anchor w -borderwidth 1
label .status.rx1   -anchor w -width 7 -relief sunken -borderwidth 1
label .status.rx2   -anchor w -width 5 -relief sunken -borderwidth 1
label .status.txlb  -text "Tx" -anchor w -borderwidth 1
label .status.tx    -anchor w -width 5 -relief sunken -borderwidth 1
checkbutton .status.scroll -justify right \
    -text "scroll"  -variable theScrollFlag
label .status.logo  -image ::img::logo32
frame .status.sep   -width 2 -height 24 -borderwidth 1 -relief sunken

pack .status.logo   -padx 0 -side right -anchor s
pack .status.tstamp -padx 2 -pady 0 -side left
pack .status.er1    -padx 2 -pady 0 -side right
pack .status.erlb1  -padx 2 -pady 0 -side right
pack .status.rx1    -padx 2 -pady 0 -side right
pack .status.rxlb1  -padx 2 -pady 0 -side right
pack .status.sep    -padx 8         -side right
pack .status.er2    -padx 2 -pady 0 -side right
pack .status.erlb2  -padx 2 -pady 0 -side right
pack .status.rx2    -padx 2 -pady 0 -side right
pack .status.rxlb2  -padx 2 -pady 0 -side right
pack .status.tx     -padx 2 -pady 0 -side right
pack .status.txlb   -padx 2 -pady 0 -side right
pack .status -side bottom -fill x  -pady 0 ;# pack status before text

# add traces for the packet counters to show them in the status bar...
trace add variable ::qspy::theTstamp   write traceTstamp
trace add variable ::qspy::theErRecCnt write traceErRecCnt
trace add variable ::qspy::theRxRecCnt write traceRxRecCnt
trace add variable ::qspy::theTxPktCnt write traceTxPktCnt
trace add variable ::qspy::theRxPktCnt write traceRxPktCnt
trace add variable ::qspy::theErPktCnt write traceErPktCnt

## @brief trace handler to monitor the Time-Stamp variable
proc traceTstamp {varname key op} {
    upvar $varname var
    .status.tstamp configure -text $var
}
## @brief trace handler to monitor the Error Count variable
proc traceErRecCnt {varname key op} {
    upvar $varname var
    .status.er1 configure -text $var
}
## @brief trace handler to monitor the Received-Records Count variable
proc traceRxRecCnt {varname key op} {
    upvar $varname var
    .status.rx1 configure -text $var
}
## @brief trace handler to monitor the Transmitted-Packets Count variable
proc traceTxPktCnt {varname key op} {
    upvar $varname var
    .status.tx configure -text $var
}
## @brief trace handler to monitor the Received-Packets Count variable
proc traceRxPktCnt {varname key op} {
    upvar $varname var
    .status.rx2 configure -text $var
}
## @brief trace handler to monitor the Error-Packets Count variable
proc traceErPktCnt {varname key op} {
    upvar $varname var
    .status.er2 configure -text $var
}

# Text View ==================================================================
frame .txt
text .txt.t -wrap none \
     -xscrollcommand {.txt.x set } -yscrollcommand {.txt.y set }
scrollbar .txt.x -orient horizontal -command {.txt.t xview}
scrollbar .txt.y -orient vertical   -command {.txt.t yview}

#.txt.t configure -state disabled
grid .txt.t .txt.y -sticky nsew
grid .txt.x -sticky nsew
grid columnconfigure .txt 0 -weight 1
grid rowconfigure    .txt 0 -weight 1
set theScrollFlag 1

#.............................................................................
## @brief handler for the Text-View command
proc onTextView {} {
    global theTextView theScrollFlag
    if {$theTextView == 1} {
        pack .txt -side bottom -expand yes -fill both -padx 2 -pady 2
        pack .status.scroll -before .status.tstamp -padx 2 -pady 0 -side right
        wm geometry . =640x300
    } else {
        pack forget .status.scroll
        pack forget .txt
        wm geometry . =640x40
#        .txt.t configure -state normal
        .txt.t delete 1.0 end
#        .txt.t configure -state disabled
    }
}
set theTextView 1
onTextView

# Canvas View ================================================================
toplevel .canv
wm title .canv "Canvas"
wm group .canv .
wm protocol .canv WM_DELETE_WINDOW onCanvClose
canvas .canv.c
pack .canv.c

## @brief handler for the Canvas-View command
proc onCanvView {} {
    global theCanvView
    if {$theCanvView} {
        wm state .canv normal
    } else {
        wm withdraw .canv
    }
}
## @brief handler for the Canvas-Close command
proc onCanvClose {} {
    global theCanvView
    set theCanvView 0
    onCanvView
}

onCanvView

#.............................................................................
## @brief helper function to display a string in the text-area
proc dispTxt {str} {
    global theTextView theScrollFlag

    if {$theTextView == 0} {
        return
    }

    .txt.t insert end \n$str

    # prevent unlimited growth of the text beyong the specified num of lines
    if {[lindex [split [.txt.t index end] .] 0] > 1000} {
        .txt.t delete 1.0 "end - 1000 lines"
    }
    if {$theScrollFlag} {
        .txt.t yview moveto 1.0
    }
}

# simple modal dialog facility ===============================================
## @brief modal dialog facilities
namespace eval ::dialog {
    # exported commands...
    namespace export \
        create \
        wait

    # variables...
    variable done  0
    variable line  0
}

## @brief create a modal dialog
proc ::dialog::create {me class} {
    toplevel $me -class $class

    # make the dialog "transient" of top-level.
    # this removes the minimize/maximize decorations from the window bar
    wm transient $me .

    frame $me.form
    pack $me.form -expand yes -fill both -padx 4 -pady 4

    frame $me.sep -height 2 -borderwidth 1 -relief sunken
    pack $me.sep -fill x -pady 4

    frame $me.controls
    pack $me.controls -side left -expand yes -fill x -padx 4 -pady 4
    label $me.logo -image ::img::logo32
    pack $me.logo -side left -expand yes -anchor se

    wm title $me $class
    wm group $me .

    # clear the numeber of lines in the dialog
    variable line
    set line 0

    after idle [format {
        update idletasks
        wm minsize %s [winfo reqwidth %s] [winfo reqheight %s]
    } $me $me $me]

    return $me
}
#.............................................................................
## @brief wait in a modal dialog
proc ::dialog::wait {me} {
    safeguard $me

    focus $me

    set x [expr [winfo rootx .]+50]
    set y [expr [winfo rooty .]+20]
    wm geometry $me "+$x+$y"

    wm deiconify $me
    grab set $me

    variable ::dialog::done
    vwait ::dialog::done

    grab release $me
    focus -force .   ;# added MMS
    wm withdraw $me

    return $::dialog::done
}
#.............................................................................
bind modalDialog <ButtonPress> {
    wm deiconify %W
    raise %W
}
#.............................................................................
## @brief helper function to safeguard the waiting modal dialog (see wait())
proc ::dialog::safeguard {me} {
    if {[lsearch [bindtags $me] modalDialog] < 0} {
        bindtags $me [linsert [bindtags $me] 0 modalDialog]
    }
}
#.............................................................................
## @brief add an entry to a modal dialog
proc ::dialog::addEntry {me label wid dflt} {
    variable line
    incr line

    set form $me.form

    label $form.label$line -text $label -anchor e
    entry $form.entry$line -width $wid
    $form.entry$line insert 0 $dflt
    grid $form.label$line $form.entry$line

    grid configure $form.label$line -sticky e
    grid configure $form.entry$line -sticky w
}

#.............................................................................
## @brief helper function to open a given URL
proc openURL {url} {
    global tcl_platform

    if {$tcl_platform(platform) == "windows"} {
        if {[catch {eval exec [auto_execok start] $url}] == 0} return
    } elseif {$tcl_platform(os) == "Linux"} {
        if {[catch {exec xdg-open   $url &}] == 0} return
        if {[catch {exec kde-open   $url &}] == 0} return
        if {[catch {exec gnome-open $url &}] == 0} return
    } else {
        # some other platform (needs code...)
    }
}


#=============================================================================
if {[llength $argv] > 0} {   ;# argv(0) -- extension script
    wm title . "QSpyView $VERSION -- [lindex $argv 0]"
    source [lindex $argv 0]  ;# source the provided extension script
} else {
    wm title . "QSpyView $VERSION -- default.tcl"
    source $HOME/default.tcl ;# source the default extension script
}
main

