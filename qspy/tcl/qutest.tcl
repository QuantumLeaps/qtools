##
# @file
# @ingroup qpspy
# @brief QSPY Unit Testing (QUTEST) front-end main script.
# @description
# QUTEST is a Unit Testing framework for embedded systems.
# This script defines a small Domain Specific Language (DSL) for writing
# user tests. This script also implements a console-based ("headless")
# front-end to the QSPY back-and for running such user tests.
# @usage
# tclsh qutest.tcl [test-files] [host_exe] [host] [port] [local_port]

## @cond
#-----------------------------------------------------------------------------
# Product: QUTEST package
# Last updated for version 5.9.0
# Last updated on  2017-05-14
#
#                    Q u a n t u m     L e a P s
#                    ---------------------------
#                    innovating embedded systems
#
# Copyright (C) 2005-2017 Quantum Leaps, LLC. All rights reserved.
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
# https://state-machine.com
# mailto:info@state-machine.com
#-----------------------------------------------------------------------------
# @endcond

package provide qutest 5.9

package require Tcl  8.4  ;# need at least Tcl 8.4

# home directory of this Tcl script file
set HOME [file dirname [file normalize [info script]]]

# use the QSPY interface package
source $HOME/qspy.tcl

# this version of qutest
set VERSION 5.9.0

namespace eval ::qutest {
    variable TIMEOUT_MS 500 ;#< timeout [ms] for waiting on the QSPY response

    #.........................................................................
    ## @brief start a new test
    # @description
    # This command starts a new test and gives it a name.
    #
    # @param[in] name   name of the test
    # @param[in] args   options  {-noreset skip}
    #
    proc test {name args} {
        variable theTestCount
        variable theSkipCount
        variable theErrCount
        variable theTestRunner
        variable theCurrState
        variable theTestMs
        set theTestMs [clock clicks -milliseconds]

        #puts "test $name"

        ;# parse and validate the test options
        set reset 1
        set skip  0
        foreach opt $args {
            if {$opt == "-noreset"} {
                if {$theCurrState == {PRE}} {
                    cleanup
                    error "First test cannot have '-noreset'"
                }
                set reset 0
            } elseif {$opt == "skip"} {
                set skip 1
            } else {
                cleanup
                error "Incorrect test option '$opt'"
            }
        }

        switch $theCurrState {
            PRE -
            TEST {
                if {$theCurrState != {PRE}} { ;# any tests executed so far?
                    call_on_teardown
                    test_passed
                }

                incr theTestCount
                if {$skip} {
                    puts "$name : SKIPPED"
                    incr theSkipCount
                    tran SKIP
                } else {
                    puts -nonewline "$name : "
                    flush stdout
                    if {$reset} {
                        tran TEST  ;# to execute 'reset' in the TEST state
                        if {[reset] == 0} {  ;# timeout?
                            variable theNextMatch
                            test_failed
                            puts "Expected: \"$theNextMatch\""
                            puts "Timed-out"
                            tran FAIL
                        }
                    }
                }
            }
            FAIL -
            SKIP {
                if {$theCurrState == {SKIP}} {
                    puts "$name : SKIPPED"
                    incr theSkipCount
                }

                incr theTestCount
                if {$skip} {
                    tran SKIP
                } else {
                    ;# wait until timeout
                    while {[wait4input]} {
                    }

                    if {$reset} {
                        puts -nonewline "$name : "
                        flush stdout
                        tran TEST  ;# to execute 'reset' in the TEST state
                        if {[reset] == 0} {  ;# timeout?
                            variable theNextMatch
                            test_failed
                            puts "Expected: \"$theNextMatch\""
                            puts "Timed-out"
                            tran FAIL
                        }
                    }
                    puts -nonewline "$name : "
                    flush stdout
                    tran TEST
                }
            }
            END -
            DONE {
                after_end test
            }
            default {
                assert 0
            }
        }

        ;# (re)entering the TEST state?
        if {$theCurrState == "TEST"} {
            variable ::qspy::theHaveTarget
            if {$::qspy::theHaveTarget} {
                call_on_setup
            } else {
                test_failed
                if {$reset == 0} {
                    puts "NOTE: test should reset the Target (no -noreset)"
                } else {
                    puts "NOTE: lost Target connection"
                }
                tran FAIL
            }
        }
    }
    #.........................................................................
    ## @brief specifies the end of tests
    proc end {} {
        variable theTestCount
        variable theCurrState
        switch $theCurrState {
            PRE {
                tran END
            }
            TEST {
                # did the timeout occur? (all expected QSPY output arrived?)
                if {[wait4input] == 0} {
                    if {$theTestCount > 0} { ;# any tests executed?
                        call_on_teardown
                        test_passed
                    }
                }
                tran END
            }
            FAIL -
            SKIP {
                while {[wait4input]} { ;# NOT timeout?
                }
                tran DONE
            }
            END -
            DONE {
                after_end end
            }
            default {
                assert 0
            }
        }
    }

    #.........................................................................
    ## @brief defines an expectation for the current test
    # @description
    # This command defines a new expecation for the textual output produced
    # by QSPY. The @p match parameter is matched to the string received from
    # QSPY by means of the Tcl `[string match ...]` command.
    #
    # @param[in] match  the expected match for the QSPY output
    #
    # @sa http://www.tcl.tk/man/tcl8.4/TclCmd/string.htm#M34
    #
    proc expect {match} {
        variable theNextMatch
        variable theTimestamp

        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                if {[string first %timestamp $match] == 0} {
                    incr theTimestamp
                    set theNextMatch \
                        [format %010d $theTimestamp][string range \
                            $match 10 end]
                } elseif [string is integer [string range $match 0 9]] {
                    incr theTimestamp
                    set theNextMatch $match
                } else {
                    set theNextMatch $match
                }
                #puts $theNextMatch
                if {![wait4input]} { ;# timeout?
                    test_failed
                    puts "Expected: \"$theNextMatch\""
                    puts "Timed-out"

                    set theNextMatch ""
                    tran FAIL
                }
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default {
                assert 0
            }
        }
    }
    #.........................................................................
    ## @brief executes a given command in the Target
    # @description
    # This command causes execution of the callback QS_onCommand() inside the
    # the Target system.
    #
    # @param[in] cmdId  the command-id first argument to QS_onCommand()
    # @param[in] param1 the 'param1' argument to QS_onCommand() (optional)
    # @param[in] param2 the 'param2' argument to QS_onCommand() (optional)
    # @param[in] param3 the 'param3' argument to QS_onCommand() (optional)
    #
    proc command {cmdId {param1 0} {param2 0} {param3 0} } {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                variable ::qspy::QS_RX
                if [string is integer $cmdId] {
                    ::qspy::sendPkt \
                        [binary format cciii $::qspy::QS_RX(COMMAND) $cmdId \
                                             $param1 $param2 $param3]
                } else { ;# cmdId is a name of a user record
                    variable ::qspy::QSPY
                    variable ::qspy::theFmt
                    variable ::qspy::QS_OBJ_KIND
                    ::qspy::sendPkt \
                        [binary format cciii $::qspy::QSPY(SEND_COMMAND) \
                            0 $param1 $param2 $param3]$cmdId\0
                }
                expect "           Trg-Ack  QS_RX_COMMAND"
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default {
                assert 0
            }
        }
    }
    #.........................................................................
    ## @brief peeks data into the Target
    # @description
    # This command peeks data at the given offset from the start address
    # of the Application (AP) Current-Object inside the Target.
    #
    # @param[in] offset offset [in bytes] from the start of the current_obj AP
    # @param[in] size   size of the data items (1, 2, or 4)
    # @param[in] num    number of data items to peek
    #
    proc peek {offs size num} {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                variable ::qspy::QS_RX
                ::qspy::sendPkt \
                    [binary format cscc $::qspy::QS_RX(PEEK) $offs $size $num]
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default {
                assert 0
            }
        }
    }
    #.........................................................................
    ## @brief pokes data into the Target
    # @description
    # This command pokes provided data at the given offset from the
    # start address of the Application (AP) Current-Object inside the Target.
    #
    # @param[in] offset offset [in bytes] from the start of the current_obj AP
    # @param[in] data   binary data to send
    # @param[in] size   size of the data items (1, 2, or 4)
    #
    proc poke {offs size data} {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                variable ::qspy::QS_RX
                set len [string length $data]
                set num [expr $len / $size]
                ::qspy::sendPkt [binary format cscc $::qspy::QS_RX(POKE) \
                                 $offs $size $num]$data
                expect "           Trg-Ack  QS_RX_POKE"
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default {
                assert 0
            }
        }
    }
    #.........................................................................
    ## @brief fills data into the Target
    # @description
    # This command fills provided data at the given offset from the
    # start address of the Application (AP) Current-Object inside the Target.
    #
    # @param[in] offset offset [in bytes] from the start of the current_obj AP
    # @param[in] size   size of the data item (1, 2, or 4)
    # @param[in] num    number of data items to fill
    # @param[in] item   data item to fill with
    #
    proc fill {offs size num {item 0}} {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                variable ::qspy::QS_RX
                array set fmt {
                    1 c
                    2 s
                    4 i
                }
                ::qspy::sendPkt [binary format cscc$fmt($size) \
                                 $::qspy::QS_RX(FILL) $offs $size $num $item]
                expect "           Trg-Ack  QS_RX_FILL"
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default {
                assert 0
            }
        }
    }
    #.........................................................................
    ## @brief Set the Global-Filter in the Target
    proc glb_filter {args} {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                set filter0 0
                set filter1 0
                set filter2 0
                set filter3 0

                foreach filter $args {
                    if {$filter == {OFF}} { ;# all filters off
                        ;
                    } elseif {$filter == {ON}} { ;# all filters on
                        set filter0 0xFFFFFFFF
                        set filter1 0xFFFFFFFF
                        set filter2 0xFFFFFFFF
                        set filter3 0x1FFFFFFF
                        break ;# no point in continuing
                    } elseif {$filter == {SM}} { ;# state machines
                        set filter0 [expr $filter0 | 0x000003FE]
                        set filter1 [expr $filter1 | 0x0F800000]
                    } elseif {$filter == {AO}} { ;# active objects
                        set filter0 [expr $filter0 | 0x0003FC00]
                        set filter2 [expr $filter2 | 0x00002000]
                    } elseif {$filter == {EQ}} { ;# raw queues (for deferral)
                        set filter0 [expr $filter0 | 0x007C0000]
                        set filter2 [expr $filter2 | 0x00004000]
                    } elseif {$filter == {MP}} { ;# raw memory pools
                        set filter0 [expr $filter0 | 0x03800000]
                        set filter2 [expr $filter2 | 0x00008000]
                    } elseif {$filter == {QF}} { ;# framework
                        set filter0 [expr $filter0 | 0xFC000000]
                        set filter1 [expr $filter1 | 0x00001F80]
                    } elseif {$filter == {TE}} { ;# time events
                        set filter1 [expr $filter1 | 0x0000007F]
                    } elseif {$filter == {SC}} { ;# scheduler
                        set filter1 [expr $filter1 | 0x007C0000]
                    } elseif {$filter == {U0}} { ;# user 70-79
                        set filter2 [expr $filter2 | 0x0000FFC0]
                    } elseif {$filter == {U1}} { ;# user 80-89
                        set filter2 [expr $filter2 | 0x03FF0000]
                    } elseif {$filter == {U2}} { ;# user 90-99
                        set filter2 [expr $filter2 | 0xFC000000]
                        set filter3 [expr $filter3 | 0x0000000F]
                    } elseif {$filter == {U3}} { ;# user 100-109
                        set filter3 [expr $filter3 | 0x00003FF0]
                    } elseif {$filter == {U4}} { ;# user 110-124
                        set filter3 [expr $filter3 | 0x1FFFC000]
                    } elseif {$filter == {UA}} { ;# user 70-124 (all)
                        set filter2 [expr $filter2 | 0xFFFFFFC0]
                        set filter3 [expr $filter3 | 0x1FFFFFFF]
                    } else {
                        assert 0 ;# invalid filter group
                    }
                }

                ;# build and send the GLB_FILTER packet to the Target...
                variable ::qspy::QS_RX
                ::qspy::sendPkt \
                    [binary format cciiii $::qspy::QS_RX(GLB_FILTER) 16 \
                                          $filter0 $filter1 $filter2 $filter3]
                expect "           Trg-Ack  QS_RX_GLB_FILTER"
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default {
                assert 0
            }
        }
    }
    #.........................................................................
    ## @brief Set the Local Filter in the Target
    proc loc_filter {kind obj} {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                ::qspy::sendLocFilter $kind $obj
                expect "           Trg-Ack  QS_RX_LOC_FILTER"
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default {
                assert 0
            }
        }
    }
    #.........................................................................
    ## @brief Set the Current-Object in the Target
    proc current_obj {kind obj} {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                ::qspy::sendCurrObj $kind $obj
                expect "           Trg-Ack  QS_RX_CURR_OBJ"
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default {
                assert 0
            }
        }
    }
    #.........................................................................
    ## @brief sends a Test-Probe to the Target
    # @description
    # This function sends the Test-Probe data to the Target. The Target
    # collects these Test-Probe preserving the order in which they were sent.
    # Subsequently, whenever a given API is called inside the Target, it can
    # obtain the Test-Probe by means of the QS_TEST_PROBE_DEF() macro.
    # The QS_TEST_PROBE_DEF() macro returns the Test-Probes in the same
    # order as they were received to the Target. If there are no more Test-
    # Probes for a given API, the Test-Probe is initialized to zero.
    #
    # @param[in] fun  the name or raw address of a function (function pointer)
    # @param[in] data the data (uint32_t) for the Test-Probe
    #
    # @note
    # All Test-Probes are cleared when the Target resets and also upon the
    # start of a new test, even if this test does not reset the Target
    # (-noreset tests).
    #
    proc probe {fun data} {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                ::qspy::sendTestProbe $fun $data
                expect "           Trg-Ack  QS_RX_TEST_PROBE"
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default {
                assert 0
            }
        }
    }
    #.........................................................................
    ## @brief sends the CONTINUE packet to the Target to continue a test
    #
    proc continue {} {
        variable ::qspy::theHaveTarget
        if {$::qspy::theHaveTarget} {
            variable ::qspy::QS_RX
            ::qspy::sendPkt \
                [binary format c $::qspy::QS_RX(CONTINUE)]
            expect "           Trg-Ack  QS_RX_TEST_CONTINUE"
        }
    }
    #.........................................................................
    ## @brief trigger system clock tick in the Target
    proc tick {{rate 0}} {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                variable ::qspy::QSPY
                variable ::qspy::QS_RX
                ::qspy::sendPkt \
                    [binary format cc $::qspy::QS_RX(TICK) $rate]
                expect "           Trg-Ack  QS_RX_TICK"
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default { assert 0 }
        }
    }
    #.........................................................................
    ## @brief dispatch a given event to Current-Object in the Target
    proc dispatch {sig {par ""}} {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                ::qspy::sendEvent 255 $sig $par
                expect "           Trg-Ack  QS_RX_EVENT"
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default { assert 0 }
        }
    }
    #.........................................................................
    ## @brief take the top-most initial transition in the Current-Object
    # in the Target
    proc init {{sig 0} {par ""}} {
        variable theCurrState
        switch $theCurrState {
            PRE {
                before_test
                tran SKIP
            }
            TEST {
                #puts "init Sig=$sig,Par=$par"
                ::qspy::sendEvent 254 $sig $par
                expect "           Trg-Ack  QS_RX_EVENT"
            }
            FAIL -
            SKIP {
                ;# ignore
            }
            END -
            DONE {
                after_end
            }
            default {
                assert 0
            }
        }
    }

    #=========================================================================
    ## @brief internal proc for taking transition in the QUTEST state machine
    proc tran {state} {
        variable theCurrState
        #puts "Tran $theCurrState->$state"
        set theCurrState $state
    }

    #.........................................................................
    ## @brief internal proc to run a test-group specified in a given test file
    proc run {test_file} {
        ## safe interpreter to run user tests
        variable theTestRunner [interp create -safe]

        # test DSL -----------------------------------------------------------
        # The following set of aliases added to the test-runner forms a small
        # Domain Specific Language (DSL) for writing QUTEST tests
        #
        $theTestRunner alias test        ::qutest::test
        $theTestRunner alias end         ::qutest::end
        $theTestRunner alias expect      ::qutest::expect
        $theTestRunner alias command     ::qutest::command
        $theTestRunner alias peek        ::qutest::peek
        $theTestRunner alias poke        ::qutest::poke
        $theTestRunner alias fill        ::qutest::fill
        $theTestRunner alias glb_filter  ::qutest::glb_filter
        $theTestRunner alias loc_filter  ::qutest::loc_filter
        $theTestRunner alias current_obj ::qutest::current_obj
        $theTestRunner alias probe       ::qutest::probe
        $theTestRunner alias continue    ::qutest::continue
        $theTestRunner alias tick        ::qutest::tick
        $theTestRunner alias dispatch    ::qutest::dispatch
        $theTestRunner alias init        ::qutest::init
        $theTestRunner alias puts        puts

        variable theNextMatch  ""
        variable theTestSkip   0
        variable theEvtLoop    1
        variable theAfterId    0
        variable theCurrState  ""
        variable theTestMs     0

        set fid [open $test_file r]
        set test_script [read $fid]
        close $fid

        variable theTimestamp 0
        variable theGroupCount
        incr theGroupCount
        tran PRE

        variable theCurrGroup  $test_file
        puts "----------------------------------------------"
        puts "Group: $theCurrGroup"
        variable theTestRunner

        $theTestRunner eval $test_script

        interp delete $theTestRunner
    }
    #.........................................................................
    ## @brief internal proc to cleanup in case of an error
    proc cleanup {} {
        variable ::qspy::theHaveTarget
        if {$::qspy::theHaveTarget} {
            variable theHostExe
            if {$theHostExe != ""} { ;# host executable?
                variable ::qspy::QS_RX
                ;# sending the RESET packet terminates the executable
                ::qspy::sendPkt [binary format c $::qspy::QS_RX(RESET)]
                variable TIMEOUT_MS
                after $TIMEOUT_MS
            }
        }
    }
    #.........................................................................
    ## @brief internal proc for starting QUTEST
    proc start {{channels 0x02}} {
        # set defaults for communication with the QSPY back-end
        # NOTE (all these can be overridden by command-line options)
        set qspy_host  "localhost"
        set qspy_port  7701
        set local_port 7702

        # command-line optional arguments processing...
        # [host_exe [host [port [local_port]]]]
        #
        global ::argc ::argv
        variable theHostExe ""
        if {$::argc > 0} {  ;# argv(0) -- test-files
            set test_files [glob -nocomplain [lindex $::argv 0]]
        } else {
            set test_files [glob -nocomplain *.tcl] ;# default *.tcl
        }
        if {$::argc > 1} {  ;# argv(1) -- host_exe
            set theHostExe [lindex $::argv 1]
        }
        if {$::argc > 2} {  ;# argv(2) -- host running QSPY
            set qspy_host [lindex $::argv 2]
        }
        if {$::argc > 3} {  ;# argv(3) -- QSPY port
            set qspy_port [lindex $::argv 3]
        }
        if {$::argc > 4} {  ;# argv(4) -- local port
            set local_port [lindex $::argv 4]
        }

        global VERSION
        puts "QUTEST unit testing front-end $VERSION"
        puts "Copyright (c) 2005-2017 Quantum Leaps"
        puts "help at: https://state-machine.com/qtools/qutest.html"

        # attach to QSPY...
        variable ::qspy::theIsAttached 0
        if {[::qspy::attach $qspy_host $qspy_port $local_port $channels]} {
            puts -nonewline "Attaching to QSPY..."
            flush stdout
        } else {  ;# failed to attach
            puts "Can't open the UDP socket.\n\
                  Check if another instance\n\
                  of qutest/qspview is already running."
            exit -2
        }
        variable TIMEOUT_MS
        set id [after $TIMEOUT_MS "set ::qspy::theIsAttached 0"]
        vwait ::qspy::theIsAttached  ;#<<<<<<< wait until attached to QSPY

        if {$::qspy::theIsAttached} { ;# NO time out?
            after cancel $id
            puts "OK"
        } else { ;# timeout
            ::qspy::detach ;# detach from QSPY
            puts "FAILED!"
            exit -3
        }

        variable theGroupCount 0
        variable theTestCount  0
        variable theSkipCount  0
        variable theErrCount   0
        variable theCurrGroup  ""
        variable theCurrState  "PRE"
        variable theStartMs [clock clicks -milliseconds]
        variable theTestMs  $theStartMs
        variable theTargetTstamp ""

        return $test_files
    }
    #.........................................................................
    ## @brief internal proc for stopping QUTEST and disconnecting from QSPY
    proc finish {} {
        cleanup
        ::qspy::detach ;# detach from QSPY

        variable theGroupCount
        variable theTestCount
        variable theSkipCount
        variable theErrCount
        variable theCurrGroup
        variable theCurrState
        variable theStartMs
        variable theTargetTstamp

        switch $theCurrState {
            PRE -
            TEST -
            SKIP -
            FAIL {
                if {$theCurrGroup != ""} {
                    puts "'end' command missing in $theCurrGroup"
                } else {
                   puts "No test scripts found"
                }
                incr theErrCount
            }
            END -
            DONE {
                ;# do nothing
            }
            default {
                assert 0
            }
        }

        puts "=============================================="
        puts "$theGroupCount Groups $theTestCount Tests\
              $theErrCount Failures $theSkipCount Skipped\
              ([expr ([clock clicks -milliseconds] - $theStartMs)*0.001]s)"
        variable TIMEOUT_MS
        after $TIMEOUT_MS

        if {$theErrCount} {
            puts "FAIL! ($theTargetTstamp)"
            exit -1
        } else {
            puts "OK ($theTargetTstamp)"
        }
    }
    #.........................................................................
    ## @brief internal proc for resetting the Target
    proc reset {} {
        variable ::qspy::theHaveTarget
        variable ::qspy::QS_RX
        variable theHostExe
        variable TIMEOUT_MS

        if {$theHostExe != ""} {  ;# running a host executable?
            if {$theHaveTarget} {
                set ::qspy::theHaveTarget 0

                ;# send RESET packet to stop and exit the host executable
                ::qspy::sendPkt [binary format c $::qspy::QS_RX(RESET)]
                ;# let the Target executable finish
                after $TIMEOUT_MS
            }
            ;# lauch a new instance of the host executable
            exec $theHostExe &

        } else { ;# real Target
            if {$::qspy::theHaveTarget == 0} { ;# don't have Target yet?
                ;# wait until the real Target comes online
                after $TIMEOUT_MS
            }
            set ::qspy::theHaveTarget 0
            ::qspy::sendPkt [binary format c $::qspy::QS_RX(RESET)]
        }

        set id [after $TIMEOUT_MS {set ::qspy::theHaveTarget 0}]
        vwait ::qspy::theHaveTarget ;#<<<<<<< wait until have Target

        if {$::qspy::theHaveTarget} {  ;# NO timeout?
            after cancel $id
            call_on_reset
            return 1 ;# reset done
        } else {
            return 0 ;# reset NOT done (timeout)
        }
    }
    #.........................................................................
    ## @brief internal assertion
    proc assert {cond} {
        if {!$cond} {
            cleanup
            variable theCurrState
            error "QUSTEST assert failed State=$theCurrState"
        }
    }
    #.........................................................................
    ## @brief internal error handler for a command used before any test
    proc before_test {} {
        puts "'[lindex [info level -1] 0]' before any 'test'"
        variable theErrCount
        incr theErrCount
    }
    #.........................................................................
    ## @brief internal error handler for a command used after the end
    proc after_end {} {
        puts "'[lindex [info level -1] 0]' following the 'end'"
        variable theErrCount
        incr theErrCount
    }
    #.........................................................................
    ## @brief internal proc for calling the 'on_reset' proc (if defined)
    proc call_on_reset {} {
        variable theTimestamp 0

        variable theTestRunner
        ;# try to execute the on_reset proc
        if {[catch {$theTestRunner eval {on_reset}} msg]} {
            if [string equal $msg {invalid command name "on_reset"}] {
                ;# "on_reset" not defined, not a problem
            } else {
                ;# "on_reset" defined, but has errors
                variable theErrCount
                incr theErrCount
                cleanup
                global errorInfo
                error "The stack trace:\n$errorInfo"
            }
        }
    }
    #.........................................................................
    ## @brief internal proc for calling the 'on_setup' proc (if defined)
    proc call_on_setup {} {
        variable theTimestamp 0

        variable ::qspy::QS_RX
        ::qspy::sendPkt [binary format c $::qspy::QS_RX(TEST_SETUP)]
        expect "           Trg-Ack  QS_RX_TEST_SETUP"

        variable theTestRunner
        ;# try to execute the on_setup proc
        if {[catch {$theTestRunner eval {on_setup}} msg]} {
            if [string equal $msg {invalid command name "on_setup"}] {
                ;# "on_setup" not defined, not a problem
            } else {
                ;# "on_setup" defined, but has errors
                variable theErrCount
                incr theErrCount
                cleanup
                global errorInfo
                error "The stack trace:\n$errorInfo"
            }
        }
    }
    #.........................................................................
    ## @brief internal proc for calling the 'on_teardown' proc (if defined)
    proc call_on_teardown {} {
        variable ::qspy::theHaveTarget
        if {$::qspy::theHaveTarget} {
            variable ::qspy::QS_RX
            ::qspy::sendPkt \
                [binary format c $::qspy::QS_RX(TEST_TEARDOWN)]
            expect "           Trg-Ack  QS_RX_TEST_TEARDOWN"

            variable theTestRunner
            ;# try to execute the on_teardown proc
            if {[catch {$theTestRunner eval {on_teardown}} msg]} {
                if [string equal $msg \
                   {invalid command name "on_teardown"}] {
                    ;# "on_teardown" not defined, not a problem
                } else {
                    ;# "on_teardown" defined, but has errors
                    variable theErrCount
                    incr theErrCount
                    cleanup
                    global errorInfo
                    error "The stack trace:\n$errorInfo"
                }
            }
        }
    }
    #.........................................................................
    ## @brief internal proc for reporting a passing test
    proc test_passed {} {
        variable theTestMs
        puts "passed ([expr ([clock clicks -milliseconds] \
                             - $theTestMs)*0.001]s)"
    }
    #.........................................................................
    ## @brief internal proc for reporting a failing test
    proc test_failed {} {
        variable theErrCount
        incr theErrCount

        variable theTestMs
        puts "====> FAILED ([expr ([clock clicks -milliseconds] \
                             - $theTestMs)*0.001]s) <===="
    }
    #.........................................................................
    ## @brief internal proc for wating for the input from QSPY
    proc wait4input {{cancel 0}} {
        variable theEvtLoop
        variable theAfterId
        variable theCurrState
        variable TIMEOUT_MS

        if {$theAfterId != 0} { ;# timeout did NOT occur?
            after cancel $theAfterId
            set theAfterId 0
        }
        if {$cancel == 0} {
            #puts "wait4input State=$theCurrState"
            set theAfterId [after $TIMEOUT_MS \
                {set ::qutest::theEvtLoop 0; set ::qutest::theAfterId 0}]
            vwait ::qutest::theEvtLoop ;# <<<<<< Event-Loop
        } else {
            #puts "wait4input CANCEL State=$theCurrState"
            set theEvtLoop 1
        }

        #if {$theEvtLoop != 1} { puts Timeout}
        return $theEvtLoop ;# 1 if no timeout; 0 if timeout
    }
} ;# namespace ::qutest

# callbacks from ::qspy ======================================================
## @brief handler for the textual output from QSPY
proc ::qspy::rec0 {} {
    # the text ouput from QSPY has the following structure:
    # sequence-number : 1 byte
    # packet-ID == 0  : 1 byte
    # record-ID       : 1 byte (original QS record or -128 for QSPY Error)
    # line            : zero-terminated string (the line)
    variable thePkt
    binary scan $thePkt xxca* recId line

    #puts $line

    variable ::qutest::theCurrState
    switch $::qutest::theCurrState {
        PRE {
            variable ::qutest::theErrCount
            incr ::qutest::theErrCount
            puts "Unexpected: \"$line\""
        }
        TEST {
            variable ::qutest::theNextMatch
            if {$theNextMatch == ""} {
                ::qutest::test_failed
                puts "Unexpected: \"$line\""
                ::qutest::tran FAIL
            } elseif [string match $theNextMatch $line] {
                set theNextMatch "" ;# invalidate the next match
            } else {
                ::qutest::test_failed
                puts "Expected: \"$theNextMatch\""
                puts "Received: \"$line\""
                ::qutest::tran FAIL
            }
        }
        FAIL -
        SKIP {
            ;# ignore
        }
        END {
            ::qutest::test_failed
            puts "Unexpected: \"$line\""
            tran DONE
        }
        DONE {
            ;# ignore
        }
        default {
            assert 0
        }
    }

    if {$recId == 69} { ;# QS_ASSERT_FAIL
        variable ::qspy::theHaveTarget
        set ::qspy::theHaveTarget 0

        variable ::qutest::theHostExe
        if {$::qutest::theHostExe == ""} { ;# NOT a host executable?
            vwait ::qspy::theHaveTarget ;#<<<<< wait until have Target again
        }
    }

    ::qutest::wait4input cancel
}
#.............................................................................
## @brief handler for the `QSPY_ATTACH` reply from QSPY.
proc ::qspy::pkt128 {} {
    variable theIsAttached
    set theIsAttached 1
}
#.............................................................................
## @brief handler for the RESET confirmation coming from the Target
proc ::qspy::recRESET {} {
    variable ::qutest::theTargetTstamp
    if {$::qutest::theTargetTstamp == ""} {
        variable theTstamp
        set ::qutest::theTargetTstamp $theTstamp
    }
}
#.............................................................................
## @brief handler for the INFO confirmation coming from the Target
proc ::qspy::recINFO {} {
}

#=============================================================================
set theClockMs [clock clicks -milliseconds]

set test_files [::qutest::start]

foreach test $test_files {
    if {[catch {::qutest::run $test} msg]} {
        ::qutest::test_failed
        puts $errorInfo

        ;# wait until timeout
        while {[::qutest::wait4input]} {
        }
        ::qutest::tran DONE
    }
}

::qutest::finish
