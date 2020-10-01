##
# @file
# @ingroup qutest
# @brief QUTest unit testing harness support for Python scripting.
#
# @description
# The Python module "qutest.py" defines a small Domain Specific Language
# (DSL) for writing test scripts in Python. This script also implements a
# console-based ("headless") front-end to the QSPY back-and for running
# such user tests.
#
# @usage
# `python <path>/qutest.py [-x] [test-scripts] [host_exe] [host[:udp_port]] [tcp_port]`
#
# or
#
# `qutest [-x] [test-scripts] [host_exe] [host[:udp_port]] [tcp_port]`
#
#-----------------------------------------------------------------------------
# Product: QUTest Python scripting (requires Python 3.3+)
# Last updated for version 6.9.1
# Last updated on  2020-09-12
#
#                    Q u a n t u m  L e a P s
#                    ------------------------
#                    Modern Embedded Software
#
# Copyright (C) 2005-2020 Quantum Leaps, LLC. All rights reserved.
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
# <www.state-machine.com/licensing>
# <info@state-machine.com>
#-----------------------------------------------------------------------------

## @brief current version of the Python QUTest interface
VERSION = 691

## @brief start a new test
# @description
# This command starts a new test and gives it a name.
#
# @param[in] title  title of the test
# @param[in] opt    options  {0=default, NORESET}
#
# @usage
# `test("my first test") # test with title and with full target reset`@n
# `~ ~ ~`@n
# `test("my second test", NORESET) # test without target reset`@n
# `~ ~ ~`
#
# @sa skip()
#
def test(title, opt = 0):

## @brief skip the tests following this command.
#
# @description
# @param[in] nTests number of tests to skip (default-all remaining tests)
#            e.g., skip(1) will skip one test following this command.
# @note
# The skipped tests are not executed, but they **are** checked for syntax
# errors, such as commands and parameters coded in the skipped tests.
#
# @usage
# `test("my first test")`@n
# `~ ~ ~`@n
# `skip(1) # skip one subsequent test`@n
# `test("my second test")`@n
# `~ ~ ~`@n
# `skip() # skip all subsequent tests`@n
# `test("my second test")`@n
# `~ ~ ~`@n
# `~ ~ ~`
#
# @sa
# test()
#
def skip(nTests = 9999):

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

## @brief Send the QS Global Filter to the Target
#
# @description
# This command sends the complete @ref qs_global "QS Global Filter" to the Target.
# Any existing Global Filter setting inside the Target will be overwritten.
#
# @param[in] args  list of Record-Type groups or individual Record-Types to set or clear.
# A given filter-group or an individual filter is set when it is positive, and
# cleared with it is preceded with the minus (`-`) sign.
#
# @n
# The filter list can contain the following:@n
# `GRP_ALL` -- all Record-Types@n
# `GRP_SM` -- State Machine Record-Types@n
# `GRP_AO` -- Active Object Record-Types@n
# `GRP_MP` -- Memory Pool Record-Types@n
# `GRP_EQ` -- Event Queue Record-Types@n
# `GRP_TE` -- Time Events Record-Types@n
# `GRP_QF` -- Framework Record-Types (e.g., post/publish/..)@n
# `GRP_SC` -- Scheduler Record-Types (e.g., scheduler lock/unlock)@n
# `GRP_U0` -- User group 0 (Record-Types 100-104)@n
# `GRP_U1` -- User group 1 (Record-Types 105-109)@n
# `GRP_U2` -- User group 2 (Record-Types 110-114)@n
# `GRP_U3` -- User group 3 (Record-Types 115-119)@n
# `GRP_U4` -- User group 0 (Record-Types 120-124)@n
# `GRP_UA` -- All user records (Record-Types 100-124)@n
# `<num>` -- Specific QS trace Record-Type in the range 0..127
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
# This command sends the complete @ref qs_local "QS Local Filter" to the Target.
# Any existing Local Filter setting inside the Target will be overwritten.
#
# @param[in] args  list of QS-ID groups or individual QS-IDs to set or clear.
# A given filter-group or an individual filter is set when it is positive, and
# cleared with it is preceded with the minus (`-`) sign.
# @n
# This parameter can take one of the following values:@n
# `IDS_ALL` -- all QS-IDs@n
# `IDS_AO` -- Active Object QS-IDs (1..64)@n
# `IDS_EP` -- Event Pool QS-IDs (65-80)@n
# `IDS_EQ` -- Event Queue QS-IDs (81-96)@n
# `IDS_AP` -- Application-Specific QS-IDs (97-127)@n
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
# This command sets or clears the @ref qs_local "QS Local Filter" corresponding to the given AO in the Target.
# Unlike loc_filter(), this facility changes **only** the QS-ID (AO's priority) of the given AO in the Target.
# All other Local Filters will be left unchanged. 
#
# @param[in] obj_id  active object to set/clear the local filter for in the Target
# @n
# This parameter can be either a string (name of the AO) or the AO's priority.
# Also, it can be either positive (to set) or negative (to clear) the QS local filter.
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
# This command sets the "current object" in the Target.
#
# @param[in] obj_kind  Kind of object to set
# @n
# This parameter can take one of the following values:@n
# `OBJ_SM` -- State Machine object@n
# `OBJ_AO` -- Active Object object@n
# `OBJ_MP` -- Memory Pool object@n
# `OBJ_EQ` -- Event Queue object@n
# `OBJ_TE` -- Time Event object@n
# `OBJ_AP` -- Application-Specific object@n
# `OBJ_SM_AO` -- Both, State Machine and Active Object
#
# @param[in] obj_id  Name or addres of the object
#
# @usage
# @include current_obj.py
#
# @sa
# query_curr()@n
# init()@n
# dispatch()
#
def current_obj(obj_kind, obj_id):

## @brief query the @ref current_obj() "current object" in the Target
#
# @description
# This command queries the current object in the Target.
#
# @param[in] obj_kind  Kind of object to query
# @n
# This parameter can take one of the following values:@n
# `OBJ_SM` -- State Machine object@n
# `OBJ_AO` -- Active Object object@n
# `OBJ_MP` -- Memory Pool object@n
# `OBJ_EQ` -- Event Queue object@n
# `OBJ_TE` -- Time Event object
#
# @usage
# The queries for various objects generate the following QS trace records
# from the Target:@n
# `query_curr(OBJ_SM)`:@n
# `"@timestamp Query-SM Obj=<obj-name>,State=<state-name>"`@n
# `query_curr(OBJ_AO)`:@n
# `"@timestamp Query-AO Obj=<obj-name>,Queue<Free=<n>,Min=<m>>"`@n
# `query_curr(OBJ_EQ)`:@n
# `"@timestamp Query-EQ Obj=<obj-name>,Queue<Free=<n>,Min=<m>>"`@n
# `query_curr(OBJ_MP)`:@n
# `"@timestamp Query-MP Obj=<obj-name>,Free=<n>,Min=<m>"`@n
# `query_curr(OBJ_TE)`:@n
# `"@timestamp Query-TE Obj=<obj-name>,Rate=<r>,Sig=<s>,Tim=<n>,Int=<m>,Flags=<f>"`
#
# @sa
# current_obj()
#
def query_curr(obj_kind):

## @brief trigger system clock tick in the Target
#
# @description
# This command triggers the following actions in the Target:@n
# 1. If the @ref current_obj() "current TE object" is defined and
#    the TE is armed, the TE is disarmed (if one-shot) and then
#     posted to the recipient AO.@n
# 2. The linked-list of all armed Time Events is updated.@n
#
# @param[in] tick_rate  the tick rate (0..QF_MAX_TICK_RATE)
#
# @usage
# @include tick.py
#
# @sa
# current_obj()
#
def tick(tick_rate = 0):

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
# This command causes execution of the callback QS_onCommand() inside the
# the Target system.
#
# @param[in] cmdId  the command-id first argument to QS_onCommand()@n
#            NOTE: this could be either the raw number or a name
#            that is delivered by QS_USR_DICTIONARY() from the Target
# @param[in] param1 the "param1" argument to QS_onCommand() (optional)
# @param[in] param2 the "param2" argument to QS_onCommand() (optional)
# @param[in] param3 the "param3" argument to QS_onCommand() (optional)
#
# @usage
# @include command.py
#
def command(cmdId, param1 = 0, param2 = 0, param3 = 0):

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
# `init()`@n
# `init("MY_SIG")`@n
# `init("MY_SIG", pack("<B", 2))`
#
def init(signal = 0, params = None):

## @brief dispatch a given event in the current SM object in the Target
#
# @description
# This command dispatches a given event in the
# @ref current_obj() "current SM object" in the Target.
#
# @param[in] signal  the event signal of the event to be dispatched
# @param[in] params  the parameters of the event to be dispatched
#
# @usage
# `dispatch("MY_SIG")`@n
# `dispatch("MY_SIG", pack("<B", 2))`
#
def dispatch(signal, params = None):

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
# `post("MY_SIG")`@n
# `post("MY_SIG", pack("<B", 2))`
#
def post(signal, params = None):

## @brief publish a given event to subscribers in the Target
#
# @description
# This command publishes a given event in the Target.
#
# @param[in] signal  the event signal of the event to be posted
# @param[in] params  the parameters of the event to be posted
#
# @usage
# `publish("MY_SIG")`@n
# `publish("MY_SIG", pack("<B", 2))`
#
def publish(signal, params = None):

## @brief sends a Test-Probe to the Target
# @description
# This command sends the Test Probe data to the Target. The Target
# collects these Test Probes preserving the order in which they were sent.
# Subsequently, whenever a given API is called inside the Target, it can
# obtain the Test-Probe by means of the QS_TEST_PROBE_DEF() macro.
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
# `probe("myFunction", 123)`
#
def probe(func, data):

## @brief peeks data in the Target
#
# @description
# This command peeks data at the given offset from the start address
# of the current_obj() inside the Target.
#
# @param[in] offset offset [in bytes] from the start of the current_obj()
# @param[in] size   size of the data items (1, 2, or 4)
# @param[in] num    number of data items to peek
#
# @usage
# `peek(0, 1, 10)`@n
# `peek(8, 2, 4)`@n
# `peek(4, 4, 2)`
#
def peek(offset, size, num):

## @brief pokes data into the Target.
# @description
# This command pokes provided data at the given offset from the start
# address of the @ref current_obj() "current AP object" inside the Target.
#
# @param[in] offset offset [in bytes] from the start of the current_obj()
# @param[in] size   size of the data items (1, 2, or 4)
# @param[in] data   binary data to send
#
# @usage
# `poke(4,4,pack("<II",0xB4C4D4E4,0xB5C5D5E5))`@n
# `poke(0, 1, bytearray("dec=%d\0", "ascii"))`@n
# `poke(0, 1, bytes("Hello World!\0","ascii"))`
#
def poke(offset, size, data):

## @brief fills data into the Target.
# @description
# This command fills provided data at the given offset from the
# start address of the current_obj() inside the Target.
#
# @param[in] offset offset [in bytes] from the start of the current_obj()
# @param[in] size   size of the data item (1, 2, or 4)
# @param[in] num    number of data items to fill
# @param[in] item   data item to fill with
#
# @usage
# `fill(0, 1, 100, 0x1A)`@n
# `fill(0, 2, 50, 0x2A2B)`@n
# `fill(0, 4, 25, 0x4A4B4C4D)`
#
def fill(offset, size, num, item = 0):

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
# @n
# `dispatch("MY_SIG", pack("<B", 2))`@n
# `poke(2, 2, pack("<HH", 0xB2C2, 0xD2E2))`
#
def pack(format, v1, v2, ...):

## @brief
# callback function invoked after each Target reset
def on_reset():

## @brief
# callback function invoked at the beginning of each test
def on_setup():

## @brief
# callback function invoked at the end of each test
def on_teardown():

