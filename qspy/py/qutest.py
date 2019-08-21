#-----------------------------------------------------------------------------
# Product: QUTest Python scripting (compatible with Python 2.7+ and 3.3+)
# Last updated for version 6.6.0
# Last updated on  2019-07-30
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
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Contact information:
# www.state-machine.com
# info@state-machine.com
#-----------------------------------------------------------------------------

# for compatibility from Python 2
from __future__ import print_function
__metaclass__ = type

from fnmatch import fnmatchcase
from glob import glob
from platform import python_version
from subprocess import Popen
from inspect import getframeinfo, stack

import struct
import socket
import time
import sys
import traceback

import os
if os.name == 'nt':
    import msvcrt
else:
    import select

#=============================================================================
# QUTest test runner and state machine
class qutest:
    _VERSION = 660

    # class variables
    _host_exe = ''
    _is_debug = False
    _exit_on_fail = False
    _have_target = False
    _have_info   = False
    _need_reset  = True
    _last_record = ''
    _num_groups  = 0
    _num_tests   = 0
    _num_failed  = 0
    _num_skipped = 0

    # states of the internal state machine
    _INIT = 0
    _TEST = 1
    _FAIL = 2
    _SKIP = 3

    # timeout value [seconds]
    _TOUT = 1.000

    # test command options
    _OPT_NORESET = 0x01

    def __init__(self):
        qutest._have_target = False
        qutest._have_info   = False
        qutest._need_reset  = True

        self._state     = qutest._INIT
        self._timestamp = 0
        self._startTime = 0
        self._to_skip   = 0

        # The following _DSL_dict dictionary defines the QUTest testing
        # DSL (Domain Specific Language), which is documented separately
        # in the file "qutest_dsl.py".
        #
        self._DSL_dict  = {
            'test': self.test,
            'skip': self.skip,
            'expect': self.expect,
            'glb_filter': self.glb_filter,
            'loc_filter': self.loc_filter,
            'current_obj': self.current_obj,
            'query_curr': self.query_curr,
            'tick': self.tick,
            'expect_pause': self.expect_pause,
            'continue_test': self.continue_test,
            'command': self.command,
            'init': self.init,
            'dispatch': self.dispatch,
            'post': self.post,
            'publish': self.publish,
            'probe': self.probe,
            'peek': self.peek,
            'poke': self.poke,
            'fill': self.fill,
            'pack': struct.pack,
            'on_reset': self._dummy_on_reset,
            'on_setup': self._dummy_on_setup,
            'on_teardown': self._dummy_on_teardown,
            'VERSION': qutest._VERSION,
            'NORESET': qutest._OPT_NORESET,
            'OBJ_SM': qspy._OBJ_SM,
            'OBJ_AO': qspy._OBJ_AO,
            'OBJ_MP': qspy._OBJ_MP,
            'OBJ_EQ': qspy._OBJ_EQ,
            'OBJ_TE': qspy._OBJ_TE,
            'OBJ_AP': qspy._OBJ_AP,
            'OBJ_SM_AO': qspy._OBJ_SM_AO,
            'GRP_OFF': 0,
            'GRP_ON': qspy._GRP_ON,
            'GRP_SM': qspy._GRP_SM,
            'GRP_AO': qspy._GRP_AO,
            'GRP_EQ': qspy._GRP_EQ,
            'GRP_MP': qspy._GRP_MP,
            'GRP_TE': qspy._GRP_TE,
            'GRP_QF': qspy._GRP_QF,
            'GRP_SC': qspy._GRP_SC,
            'GRP_U0': qspy._GRP_U0,
            'GRP_U1': qspy._GRP_U1,
            'GRP_U2': qspy._GRP_U2,
            'GRP_U3': qspy._GRP_U3,
            'GRP_U4': qspy._GRP_U4,
            'GRP_UA': qspy._GRP_UA
        }

    def __del__(self):
        #print('~qutest', self)
        pass

    #-------------------------------------------------------------------------
    # QUTest Domain Specific Language (DSL) commands

    # test DSL command .......................................................
    def test(self, name, opt = 0):
        # end the previous test
        self._test_end()

        # start the new test...
        self._startTime = qutest._time()
        qutest._num_tests += 1
        print('%s: ' %name, end = '')

        if self._to_skip > 0:
            self._to_skip -= 1
            qutest._num_skipped += 1
            print('SKIPPED')
            self._tran(qutest._SKIP)
            return

        if opt & qutest._OPT_NORESET != 0:
            if self._state == qutest._FAIL:
                self._fail('NORESET-test follows a failed test')
                return
            if qutest._need_reset:
                self._fail('NORESET-test needs reset')
                return
            self._tran(qutest._TEST)
        else:
            self._tran(qutest._TEST)
            if not self._reset_target():
                return

        if not self._on_setup():
            self._fail('on_setup() failed')
            return

    # expect DSL command .....................................................
    def expect(self, match):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('expect')
        elif self._state == qutest._TEST:

            if not qspy._receive(): # timeout?
                self._fail('expected: "%s"' %match,
                           'received: "" (timeout)')
                return False

            if match.startswith('@timestamp') or match.startswith('%timestamp'):
                self._timestamp += 1
                expected = '%010d' %self._timestamp + match[10:]
            elif match[0:9].isdigit():
                self._timestamp += 1
                expected = match
            else:
                expected = match

            received = qutest._last_record[3:].decode('utf-8')

            if fnmatchcase(received, expected):
                return True
            else:
                self._fail('expected: "%s"' %expected,
                           'received: "%s"' %received)
                return False
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in expect: ' + match

    # glb_filter DSL command .................................................
    def glb_filter(self, *args):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('glb_filter')
        elif self._state == qutest._TEST:
            filter = [0, 0, 0, 0]

            for arg in args:
                if arg == 0:
                    pass
                elif arg < 0x7F:
                    filter[arg // 32] |= 1 << (arg % 32)
                elif arg == qspy._GRP_ON:
                    # all filters on
                    filter[0] = 0xFFFFFFFF
                    filter[1] = 0xFFFFFFFF
                    filter[2] = 0xFFFFFFFF
                    filter[3] = 0x1FFFFFFF
                    break  # no point in continuing
                elif arg == qspy._GRP_SM:  # state machines
                    filter[0] |= 0x000003FE
                    filter[1] |= 0x03800000
                elif arg == qspy._GRP_AO:   # active objects
                    filter[0] |= 0x0007FC00
                    filter[1] |= 0x00002000
                elif arg == qspy._GRP_EQ:  # raw queues
                    filter[0] |= 0x00780000
                    filter[2] |= 0x00004000
                elif arg == qspy._GRP_MP:  # raw memory pools
                    filter[0] |= 0x03000000
                    filter[2] |= 0x00008000
                elif arg == qspy._GRP_QF:  # framework
                    filter[0] |= 0xFC000000
                    filter[1] |= 0x00001FC0
                elif arg == qspy._GRP_TE:  # time events
                    filter[1] |= 0x0000007F
                elif arg == qspy._GRP_SC:  # scheduler
                    filter[1] |= 0x007F0000
                elif arg == qspy._GRP_U0:  # user 70-79
                    filter[2] |= 0x0000FFC0
                elif arg == qspy._GRP_U1:  # user 80-89
                    filter[2] |= 0x03FF0000
                elif arg == qspy._GRP_U2:  # user 90-99
                    filter[2] |= 0xFC000000
                    filter[3] |= 0x0000000F
                elif arg == qspy._GRP_U3:  # user 100-109
                    filter[3] |= 0x00003FF0
                elif arg == qspy._GRP_U4:  # user 110-124
                    filter[3] |= 0x1FFFC000
                elif arg == qspy._GRP_UA:  # user 70-124 (all)
                    filter[2] |= 0xFFFFFFC0
                    filter[3] |= 0x1FFFFFFF
                else:
                    assert 0, 'invalid global filter'

            qspy._sendTo(struct.pack(
                '<BBLLLL', qspy._TRGT_GLB_FILTER, 16,
                filter[0], filter[1], filter[2], filter[3]))

            self.expect('           Trg-Ack  QS_RX_GLB_FILTER')

        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in glb_filter'

    # loc_filter DSL command .................................................
    def loc_filter(self, obj_kind, obj_id):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('loc_filter')
        elif self._state == qutest._TEST:
            fmt = '<BB' + qspy._target_info['objPtr']
            if isinstance(obj_id, int):
                # Send directly to Target
                packet = struct.pack(
                    fmt, qspy._TRGT_LOC_FILTER, obj_kind, obj_id)
            else:
                # Have QSpy interpret obj_id string and send filter
                packet = bytearray(struct.pack(
                    fmt, qspy._QSPY_SEND_LOC_FILTER, obj_kind, 0))
                packet.extend(qspy._str2bin(obj_id))

            qspy._sendTo(packet)
            self.expect('           Trg-Ack  QS_RX_LOC_FILTER')

        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in loc_filter'

    # current_obj DSL command ................................................
    def current_obj(self, obj_kind, obj_id):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('current_obj')
        elif self._state == qutest._TEST:
            fmt = '<BB' + qspy._target_info['objPtr']
            # Build packet according to obj_id type
            if isinstance(obj_id, int):
                packet = struct.pack(
                    fmt, qspy._TRGT_CURR_OBJ, obj_kind, obj_id)
            else:
                packet = bytearray(struct.pack(
                    fmt, qspy._QSPY_SEND_CURR_OBJ, obj_kind, 0))
                # add string object ID to end
                packet.extend(qspy._str2bin(obj_id))

            qspy._sendTo(packet)
            self.expect('           Trg-Ack  QS_RX_CURR_OBJ')

        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in current_obj'

    # query_curr DSL command .................................................
    def query_curr(self, obj_kind):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('query_curr')
        elif self._state == qutest._TEST:
            qspy._sendTo(struct.pack('<BB', qspy._TRGT_QUERY_CURR, obj_kind))
            # test-specific expect
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in query_curr'

    # expect_pause DSL command ...............................................
    def expect_pause(self):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('expect_pause')
        elif self._state == qutest._TEST:
            self.expect('           TstPause')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in expect_pause'

    # continue_test DSL command ..............................................
    def continue_test(self):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('continue_test')
        elif self._state == qutest._TEST:
            qspy._sendTo(struct.pack('<B', qspy._TRGT_CONTINUE))
            self.expect('           Trg-Ack  QS_RX_TEST_CONTINUE')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in continue_test'

    # command DSL command ....................................................
    def command(self, cmdId, param1 = 0, param2 = 0, param3 = 0):
        if self._to_skip > 0:
            pass # ignore
        if self._state == qutest._INIT:
            self._before_test('command')
        elif self._state == qutest._TEST:
            fmt = '<BBIII'
            if isinstance(cmdId, int):
                packet = struct.pack(fmt, qspy._TRGT_COMMAND,
                                     cmdId, param1, param2, param3)
            else:
                packet = bytearray(struct.pack(
                    fmt, qspy._QSPY_SEND_COMMAND, 0, param1, param2, param3))
                # add string command ID to end
                packet.extend(qspy._str2bin(cmdId))
            qspy._sendTo(packet)
            self.expect('           Trg-Ack  QS_RX_COMMAND')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in command'

    # init DSL command .......................................................
    def init(self, signal = 0, params = None):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('init')
        elif self._state == qutest._TEST:
            qspy._sendEvt(qspy._EVT_INIT, signal, params)
            self.expect('           Trg-Ack  QS_RX_EVENT')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in init'

    # dispatch DSL command ...................................................
    def dispatch(self, signal, params = None):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('dispatch')
        elif self._state == qutest._TEST:
            qspy._sendEvt(qspy._EVT_DISPATCH, signal, params)
            self.expect('           Trg-Ack  QS_RX_EVENT')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in dispatch'

    # post DSL command .......................................................
    def post(self, signal, params = None):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('post')
        elif self._state == qutest._TEST:
            qspy._sendEvt(qspy._EVT_POST, signal, params)
            self.expect('           Trg-Ack  QS_RX_EVENT')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in post'

    # publish DSL command ....................................................
    def publish(self, signal, params = None):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('publish')
        elif self._state == qutest._TEST:
            qspy._sendEvt(qspy._EVT_PUBLISH, signal, params)
            self.expect('           Trg-Ack  QS_RX_EVENT')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in publish'

    # probe DSL command ......................................................
    def probe(self, func, data):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('probe')
        elif self._state == qutest._TEST:
            fmt = '<BI' + qspy._target_info['funPtr']
            if isinstance(func, int):
                # Send directly to target
                packet = struct.pack(
                    fmt, qspy._TRGT_TEST_PROBE, data, func)
            else:
                # Send to QSPY to provide 'func' from Fun Dictionary
                packet = bytearray(struct.pack(
                    fmt, qspy._QSPY_SEND_TEST_PROBE, data, 0))
                # add string func name to end
                packet.extend(qspy._str2bin(func))
            qspy._sendTo(packet)
            self.expect('           Trg-Ack  QS_RX_TEST_PROBE')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in probe'

    # tick DSL command .......................................................
    def tick(self, tick_rate = 0):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('tick')
        elif self._state == qutest._TEST:
            qspy._sendTo(struct.pack('<BB', qspy._TRGT_TICK, tick_rate))
            self.expect('           Trg-Ack  QS_RX_TICK')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in tick'

    # peek DSL command .......................................................
    def peek(self, offset, size, num):
        assert size == 1 or size == 2 or size == 4, \
            'Size must be 1, 2, or 4'
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('peek')
        elif self._state == qutest._TEST:
            qspy._sendTo(struct.pack('<BHBB', qspy._TRGT_PEEK,
                offset, size, num))
            # explicit expectation of peek output
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in peek'

    # poke DSL command .......................................................
    def poke(self, offset, size, data):
        assert size == 1 or size == 2 or size == 4, \
            'Size must be 1, 2, or 4'
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('poke')
        elif self._state == qutest._TEST:
            length = len(data)
            num = length // size
            packet = bytearray(struct.pack('<BHBB', qspy._TRGT_POKE,
                offset, size, num))
            packet.extend(data)
            qspy._sendTo(packet)
            self.expect('           Trg-Ack  QS_RX_POKE')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in poke'

    # fill DSL command .......................................................
    def fill(self, offset, size, num, item = 0):
        assert size == 1 or size == 2 or size == 4, \
            'Size must be 1, 2, or 4'
        if self._to_skip > 0:
            pass # ignore
        elif self._state == qutest._INIT:
            self._before_test('fill')
        elif self._state == qutest._TEST:
            if size == 1:
                item_fmt = 'B'
            elif size == 2:
                item_fmt = 'H'
            elif size == 4:
                item_fmt = 'L'
            else:
                assert False, 'size for sendFill must be 1, 2, or 4!'
            fmt = '<BHBB' + item_fmt
            packet = struct.pack(fmt, qspy._TRGT_FILL, offset, size, num, item)
            qspy._sendTo(packet)
            self.expect('           Trg-Ack  QS_RX_FILL')
        elif self._state == qutest._FAIL or self._state == qutest._SKIP:
            pass # ignore
        else:
            assert 0, 'invalid state in fill'

    # skip DSL command .......................................................
    def skip(self, nTests = 9999):
        if self._to_skip == 0: # not skipping already?
            self._to_skip = nTests

    # dummy callbacks --------------------------------------------------------
    def _dummy_on_reset(self):
        #print('_dummy_on_reset')
        pass

    def _dummy_on_setup(self):
        #print('_dummy_on_setup')
        pass

    def _dummy_on_teardown(self):
        #print('_dummy_on_teardown')
        pass

    # helper methods ---------------------------------------------------------
    @staticmethod
    def _run_script(fname):
        print('--------------------------------------------------'
              '\nGroup:', fname)
        qutest._num_groups += 1

        err = 0 # assume no errors
        with open(fname) as f:
            qutest_inst = qutest()
            try:
                code = compile(f.read(), fname, 'exec')
                # execute the script code in a separate instance of qutest
                exec(code, qutest_inst._DSL_dict)
            except (AssertionError,
                    RuntimeError,
                    OSError) as e:
                qutest_inst._fail()
                print(repr(e))
                err = -2
            except: # most likely an error in a test script
                #exc_type, exc_value, exc_traceback = sys.exc_info()
                qutest_inst._fail()
                qutest._quit_host_exe()
                traceback.print_exc(2)
                err = -3

            qutest_inst._test_end()
            qutest._quit_host_exe()
            err = qutest._num_failed

        return err;

    def _tran(self, state):
        #print('tran(%d->%d)' %(self._state, state))
        self._state = state

    def _test_end(self):
        if not self._state == qutest._TEST:
            return

        if not qutest._have_info:
            return

        qspy._sendTo(struct.pack('<B', qspy._TRGT_TEST_TEARDOWN))
        if not qspy._receive(): # timeout?
            self._fail('expected: end-of-test',
                       'received: "" (timeout)')
            return

        expected = '           Trg-Ack  QS_RX_TEST_TEARDOWN'
        received = qutest._last_record[3:].decode('utf-8')
        if received == expected:
            self._DSL_dict['on_teardown']()
            print('PASS (%.3fs)'
                  %(qutest._time() - self._startTime))
            return
        else:
            self._fail('expected: end-of-test',
                       'received: "%s"' %received)
            # ignore all input until timeout
            while qspy._receive():
                pass

    def _reset_target(self):
        if qutest._host_exe != '': # running a host executable?
            qutest._quit_host_exe()

            # lauch a new instance of the host executable
            qutest._have_target = True
            qutest._have_info = False
            Popen([qutest._host_exe,
                   qspy._host_name + ':' + str(qspy._tcp_port)])

        else: # running an embedded target
            qutest._have_target = True
            qutest._have_info = False
            if not qutest._is_debug:
                qspy._sendTo(struct.pack('<B', qspy._TRGT_RESET))

        # ignore all input until a timeout (False)
        while qspy._receive():
            if qutest._have_info:
                break

        if qutest._have_info:
            qutest._need_reset = False;
        else:
            qutest._quit_host_exe()
            raise RuntimeError('Target reset failed')

        self._timestamp = 0
        self._DSL_dict['on_reset']()
        return (self._state == qutest._TEST)

    def _on_setup(self):
        assert self._state == qutest._TEST, \
            'on_setup() outside the TEST state %d' %self._state
        if not qutest._have_info:
            return False

        qspy._sendTo(struct.pack('<B', qspy._TRGT_TEST_SETUP))
        self.expect('           Trg-Ack  QS_RX_TEST_SETUP')
        if self._state == qutest._TEST:
            self._timestamp = 0
            self._DSL_dict['on_setup']()
            return True

    def _before_test(self, command):
        qutest._num_failed += 1
        self._tran(qutest._FAIL)
        msg = '"' + command + '" before any test'
        raise SyntaxError(msg)

    def _fail(self, msg1 = '', msg2 = ''):
        print('FAIL @line:%d (%.3fs):' %(
            getframeinfo(stack()[-4][0]).lineno,
            qutest._time() - self._startTime))
        if msg1 != '':
            print(' ', msg1)
        if msg2 != '':
            print(' ', msg2)
        qutest._num_failed += 1
        qutest._need_reset = True
        self._tran(qutest._FAIL)

    @staticmethod
    def _quit_host_exe():
        if qutest._host_exe != '' and qutest._have_target:
            qutest._have_target = False
            qspy._sendTo(struct.pack('<B', qspy._TRGT_RESET))
            time.sleep(qutest._TOUT) # wait until host-exe quits

    @staticmethod
    def _time():
        if sys.version_info[0] == 2: # Python 2 ?
            return time.time()
        else: # Python 3+
            return time.perf_counter()

#=============================================================================
# Helper class for communication with the QSPY front-end
#
class qspy:
    # class variables...
    _sock = None
    _is_attached = False
    _tx_seq = 0
    _host_name = 'localhost'
    _udp_port = 7701
    _tcp_port = 6601
    _target_info = {
        'objPtr': 'L',
        'funPtr': 'L',
        'tstamp': 'L',
        'sig': 'H',
        'evtSize': 'H',
        'queueCtr': 'B',
        'poolCtr': 'H',
        'poolBlk': 'H',
        'tevtCtr': 'H',
        'target' : 'UNKNOWN'
    }

    # records to the Target...
    _TRGT_INFO       = 0
    _TRGT_COMMAND    = 1
    _TRGT_RESET      = 2
    _TRGT_TICK       = 3
    _TRGT_PEEK       = 4
    _TRGT_POKE       = 5
    _TRGT_FILL       = 6
    _TRGT_TEST_SETUP = 7
    _TRGT_TEST_TEARDOWN = 8
    _TRGT_TEST_PROBE = 9
    _TRGT_GLB_FILTER = 10
    _TRGT_LOC_FILTER = 11
    _TRGT_AO_FILTER  = 12
    _TRGT_CURR_OBJ   = 13
    _TRGT_CONTINUE   = 14
    _TRGT_QUERY_CURR = 15
    _TRGT_EVENT      = 16

    # packets to QSPY only...
    _QSPY_ATTACH     = 128
    _QSPY_DETACH     = 129
    _QSPY_SAVE_DICT  = 130
    _QSPY_SCREEN_OUT = 131
    _QSPY_BIN_OUT    = 132
    _QSPY_MATLAB_OUT = 133
    _QSPY_MSCGEN_OUT = 134
    _QSPY_SEND_EVENT = 135
    _QSPY_SEND_LOC_FILTER = 136
    _QSPY_SEND_CURR_OBJ   = 137
    _QSPY_SEND_COMMAND    = 138
    _QSPY_SEND_TEST_PROBE = 139

    # gloal filter groups...
    _GRP_ON = 0xF0
    _GRP_SM = 0xF1
    _GRP_AO = 0xF2
    _GRP_MP = 0xF3
    _GRP_EQ = 0xF4
    _GRP_TE = 0xF5
    _GRP_QF = 0xF6
    _GRP_SC = 0xF7
    _GRP_U0 = 0xF8
    _GRP_U1 = 0xF9
    _GRP_U2 = 0xFA
    _GRP_U3 = 0xFB
    _GRP_U4 = 0xFC
    _GRP_UA = 0xFD

    # kinds of objects (local-filter and curr-obj)...
    _OBJ_SM = 0
    _OBJ_AO = 1
    _OBJ_MP = 2
    _OBJ_EQ = 3
    _OBJ_TE = 4
    _OBJ_AP = 5
    _OBJ_SM_AO = 6

    # special events for QS-RX
    _EVT_PUBLISH   = 0
    _EVT_POST      = 253
    _EVT_INIT      = 254
    _EVT_DISPATCH  = 255

    # special empty record
    _EMPTY_RECORD  = '    '

    @staticmethod
    def _attach(channels = 0x2):
        # channels: 1-binary, 2-text, 3-both
        # Create socket and connect
        qspy._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        qspy._sock.connect((qspy._host_name, qspy._udp_port))
        qspy._sock.settimeout(qutest._TOUT) # timeout for the socket

        print('Attaching to QSPY (%s:%d) ... '
            %(qspy._host_name, qspy._udp_port), end = '')
        qspy._is_attached = False
        qspy._sendTo(struct.pack('<BB', qspy._QSPY_ATTACH, channels))
        try:
            qspy._receive()
        except:
            pass

        if qspy._is_attached:
            print('OK')
            return True
        else:
            print('FAIL!')
            return False

    @staticmethod
    def _detach():
        if qspy._sock is None:
            return
        qspy._sendTo(struct.pack('<B', qspy._QSPY_DETACH))
        time.sleep(qutest._TOUT)
        qspy._sock.shutdown(socket.SHUT_RDWR)
        qspy._sock.close()
        qspy._sock = None

    @staticmethod
    def _receive():
        '''returns True if packet received, False if timed out'''

        if not qutest._is_debug:
            try:
                data = qspy._sock.recv(4096)
            except socket.timeout:
                qutest._last_record = qspy._EMPTY_RECORD
                return False # timeout
            # don't catch OSError
        else:
            while True:
                try:
                    data = qspy._sock.recv(4096)
                    break
                except socket.timeout:
                    print("\nwaiting for Target (press Enter to skip this test)...", end='')
                    if os.name == 'nt':
                        if msvcrt.kbhit():
                            if msvcrt.getch() == '\r':
                                print()
                                return False; # timeout
                    else:
                        dr,dw,de = select.select([sys.stdin], [], [], 0)
                        if dr != []:
                            sys.stdin.readline() # consue the Return key
                            print()
                            return False; # timeout
                # don't catch OSError

        dlen = len(data)
        if dlen < 2:
            qutest._last_record = qspy._EMPTY_RECORD
            raise RuntimeError('UDP packet from QSPY too short')

        # binary conversion compatible with Python 2
        recID = struct.unpack('B', data[1:2])[0]

        if recID == 0: # text data? (most common)
            qutest._last_record = data
            # QS_ASSERTION?
            if dlen > 3 and struct.unpack('B', data[2:3])[0] == 69:
                qutest._need_reset = True

        elif recID == 64: # target info?
            if dlen != 18:
                qutest._last_record = qspy._EMPTY_RECORD
                raise RuntimeError('Incorrect Target info')

            fmt = 'xBHxLxxxQ'
            bytes = struct.unpack('>BBBBBBBBBBBBB', data[5:18])
            qspy._target_info['objPtr']  = fmt[bytes[3] & 0x0F]
            qspy._target_info['funPtr']  = fmt[bytes[3] >> 4]
            qspy._target_info['tstamp']  = fmt[bytes[4] & 0x0F]
            qspy._target_info['sig']     = fmt[bytes[0] & 0x0F]
            qspy._target_info['evtSize'] = fmt[bytes[0] >> 4]
            qspy._target_info['queueCtr']= fmt[bytes[1] & 0x0F]
            qspy._target_info['poolCtr'] = fmt[bytes[2] >> 4]
            qspy._target_info['poolBlk'] = fmt[bytes[2] & 0x0F]
            qspy._target_info['tevtCtr'] = fmt[bytes[1] >> 4]
            qspy._target_info['target']  = '%02d%02d%02d_%02d%02d%02d' \
                %(bytes[12], bytes[11], bytes[10],
                  bytes[9], bytes[8], bytes[7])
            #print('******* Target:', qspy._target_info['target'])
            qutest._last_record = data
            qutest._have_info = True

        elif recID == 128: # attach
            qutest._last_record = data
            qspy._is_attached = True

        elif recID == 129: # detach
            qutest._quit_host_exe()
            qutest._last_record = data
            qspy._detach()
            qspy._is_attached = False

        else:
            qutest._last_record = qspy._EMPTY_RECORD
            raise RuntimeError('Unrecognized UDP data type from QSPY')

        return True # some input received

    @staticmethod
    def _sendTo(packet):
        tx_packet = bytearray({qspy._tx_seq})
        tx_packet.extend(packet)
        qspy._sock.send(tx_packet)
        qspy._tx_seq = (qspy._tx_seq + 1) & 0xFF
        #print('sendTo', qspy._tx_seq)

    @staticmethod
    def _sendEvt(ao_prio, signal, parameters = None):
        fmt = '<BB' + qspy._target_info['sig'] + 'H'
        if parameters is not None:
            length = len(parameters)
        else:
            length = 0

        if isinstance(signal, int):
            packet = bytearray(struct.pack(
                fmt, qspy._TRGT_EVENT, ao_prio, signal, length))
            if parameters is not None:
                packet.extend(parameters)
        else:
            packet = bytearray(struct.pack(
                fmt, qspy._QSPY_SEND_EVENT, ao_prio, 0, length))
            if parameters is not None:
                packet.extend(parameters)
            packet.extend(qspy._str2bin(signal))

        qspy._sendTo(packet)

    @staticmethod
    def _str2bin(str):
        if sys.version_info[0] == 2: # Python 2 ?
            packed = str
        else: # Python 3+
            packed = bytes(str, 'utf-8')
        fmt = '%dsB' %(len(packed) + 1)
        # Null terminate and return
        return struct.pack(fmt, packed, 0)


#=============================================================================
# main entry point to qutest
def _main(argv):
    startTime = qutest._time()

    print('QUTest unit testing front-end %d.%d.%d running on Python %s' \
        %(qutest._VERSION//100,
            (qutest._VERSION//10) % 10,
            qutest._VERSION % 10, python_version()))
    print('Copyright (c) 2005-2019 Quantum Leaps, www.state-machine.com')

    # list of scripts to exectute...
    scripts = []

    # process command-line arguments...
    args = argv[1:] # remove the 'qutest' name
    #print(args)

    if '-h' in args or '--help' in args or '?' in args:
        print('\nUsage: qutest [-x] [test-scripts] '
              '[host_exe] [qspy_host[:udp_port]] [qspy_tcp_port]\n\n'
              'help at: https://www.state-machine.com/qtools/qutest.html')
        return 0

    argc = len(args)
    if argc > 0 and args[0] == '-x':
        qutest._exit_on_fail = True
        args = args[1:0]
        argc -= 1

    if argc > 0:
        # scan args for test scripts...
        new_args = [] # arguments after removing all test scripts
        for arg in args:
            # if test file input uses wildcard, find matches
            if arg.find('*') >= 0:
                scripts.extend(glob(arg))
            elif arg.endswith('.py'):
                scripts.extend(glob(arg))
            else:
                new_args.append(arg)

        argc = len(new_args)
        if argc > 0:
            qutest._host_exe = new_args[0]
            if qutest._host_exe == 'DEBUG':
                qutest._host_exe = ''
                qutest._is_debug = True
        if argc > 1:
            host_port = new_args[1].split(':')
            if len(host_port) > 0:
                qspy._host_name = host_port[0]
            if len(host_port) > 1:
                qspy._udp_port = int(host_port[1])
        if argc > 2:
            qspy._tcp_port = new_args[2]
    else:
        scripts.extend(glob('*.py'))

    # attach to QSPY
    if not qspy._attach():
        return -1

    # run all the test scripts...
    err = 0
    for scr in scripts:
        # run the script...
        err = qutest._run_script(scr)

        # error encountered and shall we quit on failure?
        if err != 0 and qutest._exit_on_fail:
            break

    if qutest._num_failed == 0:
        status = 'OK'
    else:
        status = 'FAIL!'
    if qutest._have_info:
        print('============= Target:',
               qspy._target_info['target'], '==============')
    else:
        print('================= (no target ) ===================')

    print('%d Groups, %d Tests, %d Failures, %d Skipped (%.3fs)'
          '\n%s'
          %(qutest._num_groups, qutest._num_tests,
            qutest._num_failed, qutest._num_skipped,
            (qutest._time() - startTime),
            status))

    qutest._quit_host_exe()
    qspy._detach()

    return err

#=============================================================================
if __name__ == '__main__':
    sys.exit(_main(sys.argv))
