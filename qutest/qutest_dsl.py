#=============================================================================
# QUTest Python scripting support
# Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
#
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
#
# This software is dual-licensed under the terms of the open source GNU
# General Public License version 3 (or any later version), or alternatively,
# under the terms of one of the closed source Quantum Leaps commercial
# licenses.
#
# The terms of the open source GNU General Public License version 3
# can be found at: <www.gnu.org/licenses/gpl-3.0>
#
# The terms of the closed source Quantum Leaps commercial licenses
# can be found at: <www.state-machine.com/licensing>
#
# Redistributions in source code must retain this top-level comment block.
# Plagiarizing this software to sidestep the license obligations is illegal.
#
# Contact information:
# <www.state-machine.com>
# <info@state-machine.com>
#=============================================================================
##
# @date Last updated on: 2023-01-24
# @version Last updated for version: 7.2.1
#
# @file
# @brief QUTest Python scripting support (documentation)

## @brief current version of the Python QUTest interface
VERSION = 721

## @brief include python code in a test script
# @description
# This @ref qutest_dsl-preamble "preamble command" includes python code
# in a specified file into the test script. The included file can contain
# any code that you would put into test scripts (see Example below).
#
# @param[in] fname  name of the file to include. May contain a path
#                   **relative** to the test script.
#
# @usage
# @code{.py}
# include("test_include.pyi") # file in the same directory as the script
# ~ ~ ~
# include("../my_include/test_include.pyi") # relative directory
# @endcode
#
# __Example__<br>
# file to be included:<br>
# @include inc_file.py
#
# test script calling `include()`:<br>
# @include inc_test.py
#
def include(fname):

## @brief get the test file name with path
# @description
# This command returns a string containing the file name of the currently
# executed test script ("test group").
#
# @usage
# @code{.py}
# file_name = test_file()
# @endcode
def test_file():

## @brief get the test directory (relative to the current directory)
# @description
# This @ref qutest_complex "complex command" returns a string containing
# the directory name of the currently executed test script ("test group").
#
# @usage
# @code{.py}
# dir_name = test_dir()
# @endcode
def test_dir():

## @brief start a new test
# @description
# This @ref qutest_complex "complex command" starts a new test
# and gives it a name.
#
# @param[in] title  title of the test
# @param[in] opt    options  {0=default, NORESET}
#
# @usage
# @code{.py}
# test("my first test") # test with title and with full target reset
# ~ ~ ~
# test("my second test", NORESET) # test without target reset
# ~ ~ ~
# @endcode
#
# @sa skip()
#
def test(title, opt=0):

## @brief start a new scenario
# @description
# This is an alias for the test() command for the BDD-style
# testing.
#
def scenario(title, opt=0):

## @brief skip the tests following this command.
#
# @param[in] nTests number of tests to skip (default-all remaining tests)
#            e.g., skip(1) will skip one test following this command.
# @note
# The skipped tests are not executed, but they **are** checked for syntax
# errors, such as commands and parameters coded in the skipped tests.
#
# @usage
# @code{.py}
# test("my first test")
# ~ ~ ~
# skip(1) # skip one subsequent test
# test("my second test")
# ~ ~ ~
# skip() # skip all subsequent tests
# test("my second test")
# ~ ~ ~
# @endcode
#
# @sa
# test()
#
def skip(nTests=9999):

## @brief defines an expectation for the current test
#
# @description
# This command defines a new expecation for the textual output produced
# by QSPY.
#
# @param[in] match  the expected match for the QSPY output
#           The @p match string can contain special characters, such as:
#           `*`, `?` and `[chars]`, which are matched according to the
#           Python command
#           [fnmatch.fnmatchcase()](https://docs.python.org/2/library/fnmatch.html)
# @note
# The @p match string can be the
# [printf-style %-subsitution string](https://docs.python.org/2/library/stdtypes.html#string-formatting-operations)
# (compatible with Python 2 and Python 3), or the new
# ["f-string"](https://realpython.com/python-f-strings/)
# (compatible only with Python 3).
#
# @note
# The special string "@timestamp" (or "%timestamp") at the beginning
# of the @p match parameter will be automatically replaced with the current
# numerical value of the test sequence-counter.
#
# @usage
# @include expect.py
#
def expect(match):

## @brief ensures that the provided Boolean expression is true
#
# @description
# This command performs checking of a condition, which is believed to be true
# in order for a test to pass. If the provided Boolean expression evaluates
# to false, the test is failed in the usual way.
#
# @param[in] bool_expr  the Boolean expression to check
#
# @usage
# @include ensure.py
#
def ensure(bool_expr):

## @brief Send the QS Global Filter to the Target
#
# @description
# This @ref qutest_simple "simple command" sends the complete
# @ref qs_global "QS Global Filter" to the Target.
# Any existing Global Filter setting inside the Target will be overwritten.
#
# @param[in] args  list of Record-Type groups or individual Record-Types
# to set or clear. A given filter-group or an individual filter is set when
# it is positive, and cleared with it is preceded with the minus (`-`) sign.
# <br>
# The filter list can contain the following:
# @code{.py}
# GRP_ALL # all Record-Types
# GRP_SM # State Machine Record-Types
# GRP_AO # Active Object Record-Types
# GRP_MP # Memory Pool Record-Types
# GRP_EQ # Event Queue Record-Types
# GRP_TE # Time Events Record-Types
# GRP_QF # Framework Record-Types (e.g., post/publish/..)
# GRP_SC # Scheduler Record-Types (e.g., scheduler lock/unlock)
# GRP_SEM # Semaphore Record-Types (e.g., Semaphore take/signal)
# GRP_MTX # Mutex Record-Types (e.g., Mutex lock/unlock)
# GRP_U0 # User group 0 (Record-Types 100-104)
# GRP_U1 # User group 1 (Record-Types 105-109)
# GRP_U2 # User group 2 (Record-Types 110-114)
# GRP_U3 # User group 3 (Record-Types 115-119)
# GRP_U4 # User group 0 (Record-Types 120-124)
# GRP_UA # All user records (Record-Types 100-124)
# <num>  # Specific QS trace Record-Type in the range 0..127
# @endcode
#
# @returns
# The 128-bit filter bitmask sent to the target. For each enabled filter
# with the QS record-ID `recID` the bitmask has a '1' in the position
# (`1 << recID`).
#
# @usage
# @include glb_filter.py
#
# @sa
# loc_filter()
#
def glb_filter(*args):

## @brief Send the Local Filter to the Target
#
# @description
# This @ref qutest_simple "simple command" sends the complete
# @ref qs_local "QS Local Filter" to the Target.
# Any existing Local Filter setting inside the Target will be overwritten.
#
# @param[in] args  list of QS-ID groups or individual QS-IDs to set or clear.
# A given filter-group or an individual filter is set when it is positive, and
# cleared with it is preceded with the minus (`-`) sign.<br>
#
# This parameter can take one of the following values:
# @code{.py}
# IDS_ALL # all QS-IDs
# IDS_AO # Active Object QS-IDs (1..64)
# IDS_EP # Event Pool QS-IDs (65-80)
# IDS_EQ # Event Queue QS-IDs (81-96)
# IDS_AP # Application-Specific QS-IDs (97-127)
# @endcode
#
# @returns
# The 128-bit filter bitmask sent to the target. For each enabled filter
# with the QS-ID `qsID` the bitmask has a '1' in the position
# (`1 << qsID`).
#
# @usage
# @include loc_filter.py
#
# @sa
# glb_filter()
#
def loc_filter(*args):

## @brief Updates the Local Filter for a given AO in the Target
#
# @description
# This @ref qutest_simple "simple command" sets or clears the
# @ref qs_local "QS Local Filter" corresponding to the given AO in the Target.
# Unlike loc_filter(), this facility changes **only** the QS-ID
# (AO's priority) of the given AO in the Target.
# All other Local Filters will be left unchanged.
#
# @param[in] obj_id  active object to set/clear the local filter
#                    for in the Target<br>
#
# This parameter can be either a string (name of the AO) or the AO's priority.
# Also, it can be either positive (to set) or negative (to clear) the QS
# local filter.
#
# @usage
# @include ao_filter.py
#
# @sa
# loc_filter()
#
def ao_filter(obj_id):

## @brief Set the Current-Object in the Target
#
# @description
# This @ref qutest_simple "simple command" sets the "current object"
# in the Target.
#
# @param[in] obj_kind  Kind of object to set<br>
#
# This parameter can take one of the following values:
# @code{.py}
# OBJ_SM # State Machine object
# OBJ_AO # Active Object object
# OBJ_MP # Memory Pool object
# OBJ_EQ # Event Queue object
# OBJ_TE # Time Event object
# OBJ_AP # Application-Specific object
# OBJ_SM_AO # Both, State Machine and Active Object
# @endcode
#
# @param[in] obj_id  Name or addres of the object
#
# @usage
# @include current_obj.py
#
# @sa
# - query_curr()
# - init()
# - dispatch()
#
def current_obj(obj_kind, obj_id):

## @brief query the @ref current_obj() "current object" in the Target
#
# @description
# This @ref qutest_complex "complex command" queries the current object
# in the Target.
#
# @param[in] obj_kind  Kind of object to query
#
# This parameter can take one of the following values:
# @code{.py}
# OBJ_SM # State Machine object
# OBJ_AO # Active Object object
# OBJ_MP # Memory Pool object
# OBJ_EQ # Event Queue object
# OBJ_TE # Time Event object
# @endcode
#
# @usage
# The queries for various objects generate the following QS trace records
# from the Target
# @code{.py}
# query_curr(OBJ_SM)
# "@timestamp Query-SM Obj=<obj-name>,State=<state-name>"
# query_curr(OBJ_AO)
# "@timestamp Query-AO Obj=<obj-name>,Queue<Free=<n>,Min=<m>>"
# query_curr(OBJ_EQ)
# "@timestamp Query-EQ Obj=<obj-name>,Queue<Free=<n>,Min=<m>>"
# query_curr(OBJ_MP)
# "@timestamp Query-MP Obj=<obj-name>,Free=<n>,Min=<m>"
# query_curr(OBJ_TE)
# "@timestamp Query-TE Obj=<obj-name>,Rate=<r>,Sig=<s>,Tim=<n>,Int=<m>,Flags=<f>"
# @endcode
#
# @sa
# current_obj()
#
def query_curr(obj_kind):

## @brief trigger system clock tick in the Target
#
# @description
# This @ref qutest_complex "complex command" triggers the following actions
# in the Target:<br>
# 1. If the @ref current_obj() "current TE object" is defined and
#    the TE is armed, the TE is disarmed (if one-shot) and then
#     posted to the recipient AO.
# 2. The linked-list of all armed Time Events is updated.
#
# @param[in] tick_rate  the tick rate (0..QF_MAX_TICK_RATE)
#
# @usage
# @include tick.py
#
# @sa
# current_obj()
#
def tick(tick_rate=0):

## @brief defines expectation for a Test Pause
#
# @description
# This is a special expectation that must match the macro
# QS_TEST_PAUSE() inside the test fixture.
#
# @note
# If QS_TEST_PAUSE() is called before QF_run(), the `expect_pause()`
# expectation must be placed in the on_reset() callback.
#
# @usage
# @include expect_pause.py
#
# @sa
# continue_test()
#
def expect_pause():

## @brief defines expectation for calling QF_run()/QF::run()
#
# @description
# This is a special expectation for the target calling the QF_run()/QF::run()
# function.
#
# @note
# This expectation must be placed at the right place in the
# on_reset() callback.
#
# @usage
# @include expect_run.py
#
# @sa
# on_reset()
#
def expect_run():

## @brief sends the CONTINUE packet to the Target to continue a test
#
# @description
# This command continues the test after QS_TEST_PAUSE().
#
# @usage
# @include continue_test.py
#
# @sa
# expect_pause()
#
def continue_test():

## @brief executes a given command in the Target
# @description
# This @ref qutest_complex "complex command" causes execution of the
# @ref QS_rx::QS_onCommand() "QS_onCommand()" callback in the test fixture.
#
# @param[in] cmdId  the first `cmdId` argument for the
#                   @ref QS_rx::QS_onCommand() "QS_onCommand()" callback
#                   function in the test fixture.
# @note
# The `cmdId` parameter could be either the raw number or a name
# that is delivered by QS_ENUM_DICTIONARY(enum, group), where the
# second `group` argument is ::QS_CMD (numerical value 7).
#
# @param[in] param1 the "param1" argument to `QS_onCommand()` (optional)
# @param[in] param2 the "param2" argument to `QS_onCommand()` (optional)
# @param[in] param3 the "param3" argument to `QS_onCommand()` (optional)
#
# @usage
# @include command.py
#
def command(cmdId, param1=0, param2=0, param3=0):

## @brief take the top-most initial transition in the
# current SM object in the Target
#
# @description
# This command takes the top-most initial transition in the
# @ref current_obj() "current SM object" in the Target.
#
# @param[in] signal  the event signal of the "initialization event"
# @param[in] params  the parameters of the "initialization event"
#
# @usage
# @code{.py}
# init()
# init("MY_SIG")
# init("MY_SIG", pack("<B", 2))
# @endcode
#
def init(signal=0, params=None):

## @brief dispatch a given event in the current SM object in the Target
#
# @description
# This @ref qutest_complex "complex command" dispatches a given event in the
# @ref current_obj() "current SM object" in the Target.
#
# @param[in] signal  the event signal of the event to be dispatched
# @param[in] params  the parameters of the event to be dispatched
#
# @usage
# @code{.py}
# dispatch("MY_SIG")
# dispatch("MY_SIG", pack("<B", 2))
# @endcode
#
def dispatch(signal, params=None):

## @brief post a given event to the current AO object in the Target
#
# @description
# This command posts a given event to the
# @ref current_obj() "current AO object" in the Target.
#
# @param[in] signal  the event signal of the event to be posted
# @param[in] params  the parameters of the event to be posted
#
# @usage
# @code{.py}
# `post("MY_SIG")
# `post("MY_SIG", pack("<B", 2))
# @endcode
#
def post(signal, params=None):

## @brief publish a given event to subscribers in the Target
#
# @description
# This @ref qutest_complex "complex command" publishes a given event
# in the Target.
#
# @param[in] signal  the event signal of the event to be posted
# @param[in] params  the parameters of the event to be posted
#
# @usage
# @code{.py}
# publish("MY_SIG")
# publish("MY_SIG", pack("<B", 2))
# @endcode
#
def publish(signal, params=None):

## @brief sends a Test-Probe to the Target
# @description
# This @ref qutest_simple "simple command" sends the Test Probe data
# to the Target. The Target collects these Test Probes preserving the
# order in which they were sent. Subsequently, whenever a given API is
# called inside the Target, it can obtain the Test-Probe by means of the
# QS_TEST_PROBE_DEF() macro.
# The QS_TEST_PROBE_DEF() macro returns the Test-Probes in the same
# order as they were received to the Target. If there are no more Test-
# Probes for a given API, the Test-Probe is initialized to zero.
#
# @param[in] func the name or raw address of a function
# @param[in] data the data (uint32_t) for the Test-Probe
#
# @note
# All Test-Probes are cleared when the Target resets and also upon the
# start of a new test, even if this test does not reset the Target
# (NORESET tests).
#
# @usage
# @code{.py}
# probe("myFunction", 123)
# @endcode
#
def probe(func, data):

## @brief peeks data in the Target
#
# @description
# This @ref qutest_complex "complex command" peeks data at the given offset
# from the start address of the current_obj() inside the Target.
#
# @param[in] offset offset [in bytes] from the start of the current_obj()
# @param[in] size   size of the data items (1, 2, or 4)
# @param[in] num    number of data items to peek
#
# @usage
# @code{.py}
# peek(0, 1, 10)
# peek(8, 2, 4)
# peek(4, 4, 2)
# @endcode
#
def peek(offset, size, num):

## @brief pokes data into the Target.
# @description
# This @ref qutest_simple "simple command" pokes provided data at the
# given offset from the start address of the
# @ref current_obj() "current AP object" inside the Target.
#
# @param[in] offset offset [in bytes] from the start of the current_obj()
# @param[in] size   size of the data items (1, 2, or 4)
# @param[in] data   binary data to send
#
# @usage
# @code{.py}
# poke(4,4,pack("<II",0xB4C4D4E4,0xB5C5D5E5))
# poke(0, 1, bytearray("dec=%d\0", "ascii"))
# poke(0, 1, bytes("Hello World!\0","ascii"))
# @endcode
#
def poke(offset, size, data):

## @brief fills data into the Target.
# @description
# This @ref qutest_simple "simple command" fills provided data at the
# given offset from the start address of the current_obj() inside the Target.
#
# @param[in] offset offset [in bytes] from the start of the current_obj()
# @param[in] size   size of the data item (1, 2, or 4)
# @param[in] num    number of data items to fill
# @param[in] item   data item to fill with
#
# @usage
# @code{.py}
# fill(0, 1, 100, 0x1A)
# fill(0, 2, 50, 0x2A2B)
# fill(0, 4, 25, 0x4A4B4C4D)
# @endcode
#
def fill(offset, size, num, item=0):

## @brief display a note in the QUTest output and in QSPY output.
# @description
# This command allows the test script to output a note (text) both
# to the QUTest output (text/log) and the QSPY output (text/log).
# This command can be also used for commenting the test scripts.
#
# @param[in] msg    text message
# @param[in] dest   destination (SCREEN, TRACE), default both
# @usage
# @code{.py}
# note("This is a short note")
#
# note('''
# This test group checks the MPU (Memory Protection Unit)
# by reading/writing from/to various memory addresses
# in the target
# ''', SCREEN)
# @endcode
#
def note(msg, dest=(SCREEN | TRACE)):

## @brief display a tag in the QUTest output and in QSPY output.
# @description
# This is an alias for the note() command for the BDD-style
# testing.
#
def tag(msg, dest=(SCREEN | TRACE)):

## @brief packs data into binary string to be sent to QSPY.
# @description
# This command corresponds to Python
# [struct.pack()](https://docs.python.org/3/library/struct.html).
# It returns a bytes object containing the values v1, v2,... packed
# according to the format string @p format. The arguments must match
# the values required by the
# [format](https://docs.python.org/3/library/struct.html#format-strings") exactly.
# The pack() command is typically used inside other QUTest commands
# to pack the binary event parameters or binary data for poke() and fill().
#
# @param[in] format string
# @param[in] v1 one or more data elements requried by format
# @param[in] v2 one or more data elements requried by format
#
# @usage
# @code{.py}
# dispatch("MY_SIG", pack("<B", 2))
# poke(2, 2, pack("<HH", 0xB2C2, 0xD2E2))
# @endcode
#
def pack(format, v1, v2, ...):

## @brief returns last record received from the target as string.
#
# @usage
# @code{.py}
# command("COMMAND_B", 123, 23456, 3456789) # generate record (if needed)
# expect("@timestamp COMMAND_B *") # expect the record from the target
# last = last_rec().split() # <-- obtain the last record and split it
# p1 = int(last[2])         # extract the expected parameter p1 (int)
# s1 = last[3]              # extract the expected string s1
# p2 = int(last[4])         # extract the expected parameter p2 (int)
# s2 = last[5]              # extract the expected string s2
# p3 = int(last[6])         # extract the expected parameter p3 (int)
# p4 = float(last[7])       # extract the expected parameter p4 (float)
# @endcode
#
def last_rec():

## @brief
# callback function invoked after each Target reset
def on_reset():

## @brief
# callback function invoked at the beginning of each test
def on_setup():

## @brief
# callback function invoked at the end of each test
def on_teardown():
