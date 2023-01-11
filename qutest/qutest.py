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
# @date Last updated on: 2023-01-11
# @version Last updated for version: 7.2.1
#
# @file
# @brief QUTest Python scripting support (implementation)
# @ingroup qutest

import argparse
import socket
import struct
import time
import sys
import traceback
import os
if os.name == "nt":
    import msvcrt
else:
    import select

from fnmatch import fnmatchcase
from glob import glob
from platform import python_version
from datetime import datetime
from subprocess import Popen
from inspect import getframeinfo, stack

#=============================================================================
# QUTest test-script runner
# https://www.state-machine.com/qtools/qutest_script.html
#
class QUTest:
    VERSION = 721

    # class variables
    _have_target = False
    _have_info   = False
    _have_assert = False
    _need_reset  = True
    _is_debug    = False
    _last_record = ''
    _num_groups  = 0
    _num_tests   = 0
    _num_failed  = 0
    _num_skipped = 0
    _str_failed  = ''
    _str_skipped = ''
    _host_exe    = None
    _log_file    = None
    # command-line options
    _opt_exit_on_fail  = False
    _opt_interactive   = False
    _opt_clear_qspy    = False
    _opt_save_qspy_txt = False
    _opt_save_qspy_bin = False

    # states of the internal QUTest state machine
    _INIT = 0
    _TEST = 1
    _FAIL = 2
    _SKIP = 3

    # timeout value [seconds]
    _TOUT = 1.000

    # options for implemented commands
    _OPT_NORESET  = 0x01  # for test() command (NORESET tests)
    _OPT_SCREEN   = 0x01  # for note() command (SCREEN destination)
    _OPT_TRACE    = 0x02  # for note() command (TRACE  destination)

    # colors/backgrounds for screen output...
    _COL_PASS1 = "\x1b[32m"           # GREEN on DEFAULT
    _COL_PASS2 = "\x1b[0m"
    _COL_FAIL1 = "\x1b[31;1m"         # B-RED on DEFAULT
    _COL_FAIL2 = "\x1b[0m"
    _COL_ERR1  = "\x1b[41m\x1b[37m"   # WHITE on RED
    _COL_ERR2  = "\x1b[0m"            # expectation end DEFAULT
    _COL_EXP1  = "\x1b[44m\x1b[37m"   # WHITE on BLUE
    _COL_EXP2  = "\x1b[0m"            # expectation end DEFAULT
    _COL_EXC1  = "\x1b[31m"           # exception text begin RED
    _COL_EXC2  = "\x1b[0m"            # exception text end   DEFAULT
    _COL_GRP1  = "\x1b[32;1m"         # test-group text begin B-YELLOW
    _COL_GRP2  = "\x1b[0m"            # test-group test end   DEFAULT
    _COL_OK1   = "\x1b[42m\x1b[30m"   # BLACK on GREEN
    _COL_OK2   = "\x1b[0m"
    _COL_NOK1  = "\x1b[41m\x1b[32;1m" # B-YELLOW on RED
    _COL_NOK2  = "\x1b[0m"

    def __init__(self):
        QUTest._have_target = False
        QUTest._have_info   = False
        QUTest._need_reset  = True
        # don't clear 'QUTest._have_assert'

        # instance variables...
        self._state     = QUTest._INIT
        self._timestamp = 0
        self._startTime = 0
        self._to_skip   = 0
        self._test_file = ""
        self._test_dir  = ""
        self._is_inter  = False

        # The following _DSL_dict dictionary defines the QUTest testing
        # DSL (Domain Specific Language), which is documented separately
        # in the file "qutest_dsl.py".
        #
        self._DSL_dict  = {
            "include": self.include,
            "test": self.test,
            "scenario": self.test, # alias for 'test' for BDD
            "skip": self.skip,
            "expect": self.expect,
            "glb_filter": self.glb_filter,
            "loc_filter": self.loc_filter,
            "ao_filter": self.ao_filter,
            "current_obj": self.current_obj,
            "query_curr": self.query_curr,
            "tick": self.tick,
            "expect_pause": self.expect_pause,
            "expect_run": self.expect_run,
            "continue_test": self.continue_test,
            "ensure": self.ensure,
            "command": self.command,
            "init": self.init,
            "dispatch": self.dispatch,
            "post": self.post,
            "publish": self.publish,
            "probe": self.probe,
            "peek": self.peek,
            "poke": self.poke,
            "fill": self.fill,
            "note": self.note,
            "tag": self.note,  # alias for 'note' for BDD
            "pack": struct.pack,
            "test_file": self.test_file,
            "test_dir": self.test_dir,
            "on_reset": self._dummy_on_reset,
            "on_setup": self._dummy_on_setup,
            "on_teardown": self._dummy_on_teardown,
            "last_rec": self.last_rec,
            "VERSION": QUTest.VERSION,
            "NORESET": QUTest._OPT_NORESET,
            "SCREEN": QUTest._OPT_SCREEN,
            "TRACE":  QUTest._OPT_TRACE,
            "OBJ_SM": QSpy._OBJ_SM,
            "OBJ_AO": QSpy._OBJ_AO,
            "OBJ_MP": QSpy._OBJ_MP,
            "OBJ_EQ": QSpy._OBJ_EQ,
            "OBJ_TE": QSpy._OBJ_TE,
            "OBJ_AP": QSpy._OBJ_AP,
            "OBJ_SM_AO": QSpy._OBJ_SM_AO,
            "GRP_ALL": QSpy._GRP_ALL,
            "GRP_SM": QSpy._GRP_SM,
            "GRP_AO": QSpy._GRP_AO,
            "GRP_EQ": QSpy._GRP_EQ,
            "GRP_MP": QSpy._GRP_MP,
            "GRP_TE": QSpy._GRP_TE,
            "GRP_QF": QSpy._GRP_QF,
            "GRP_SC": QSpy._GRP_SC,
            "GRP_SEM": QSpy._GRP_SEM,
            "GRP_MTX": QSpy._GRP_MTX,
            "GRP_U0": QSpy._GRP_U0,
            "GRP_U1": QSpy._GRP_U1,
            "GRP_U2": QSpy._GRP_U2,
            "GRP_U3": QSpy._GRP_U3,
            "GRP_U4": QSpy._GRP_U4,
            "GRP_UA": QSpy._GRP_UA,
            "GRP_OFF": -QSpy._GRP_ALL,
            "GRP_ON": QSpy._GRP_ALL,
            "QS_USER": QSpy._QS_USER,
            "IDS_ALL": QSpy._IDS_ALL,
            "IDS_AO": QSpy._IDS_AO,
            "IDS_EP": QSpy._IDS_EP,
            "IDS_EQ": QSpy._IDS_EQ,
            "IDS_AP": QSpy._IDS_AP
        }

    def __del__(self):
        #print("~QUTest", self)
        pass

    #-------------------------------------------------------------------------
    # QUTest Domain Specific Language (DSL) commands

    # test DSL command .......................................................
    def test(self, title, opt=0):
        # end the previous test
        self._test_end()

        # start the new test...
        self._startTime = QUTest._time()
        QUTest._num_tests += 1

        if self._is_inter:
            QUTest._num_skipped += 1
            QUTest._str_skipped += " %d"%(QUTest._num_tests)

        if self._to_skip == 0:
            marker = "[%2d]%s--------------------------------------"\
                "-----------------------------------\n%s"%(
                    QUTest._num_tests,
                    ('-', "^")[opt & QUTest._OPT_NORESET != 0], # '^' NORESET
                    title)
            QUTest._display(marker)
            QSpy._qspy_show(marker)
        else:
            self._to_skip -= 1
            QUTest._num_skipped += 1
            QUTest._str_skipped += " %d"%(QUTest._num_tests)

            marker = "[%2d]%s - - - - - - - - - - - - - - - - - - -"\
                "- - - - - - - - - - - - - - - - - -\n%s"%(
                    QUTest._num_tests,
                    ('-', "^")[opt & QUTest._OPT_NORESET != 0], # '^' NORESET
                    title)
            QUTest._display(marker)
            QSpy._qspy_show(marker)
            QUTest._display("                                             "
                "                      [ SKIPPED ]")
            QSpy._qspy_show("                                             "
                "                      [ SKIPPED ]")
            self._tran(QUTest._SKIP)
            return

        if opt & QUTest._OPT_NORESET != 0:
            if self._state == QUTest._FAIL:
                self._fail("NORESET-test follows a failed test")
                return
            if QUTest._need_reset:
                self._fail("NORESET-test needs reset")
                return
            self._tran(QUTest._TEST)
        else:
            self._tran(QUTest._TEST)
            if not self._reset_target():
                return

        if not self._on_setup():
            self._fail("on_setup() failed")
            return

    # expect DSL command .....................................................
    def expect(self, exp, ignore=False):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("expect")
        elif self._state == QUTest._TEST:

            if not QSpy._receive(): # timeout?
                self._fail('got: "" (timeout)',
                           'exp: "%s"'%(exp))
                return False

            # NOTE: ahead of the call to fnmatchcase(), replace
            # the special characters "[" and "]" with non-printable,
            # but unique counterparts"\2" and "\3", respectively.
            # This is to avoid the unwanted special treatment of "["/"]"
            # in fnmatchcase().
            if exp.startswith("@timestamp"):
                self._timestamp += 1
                exp = "%010d"%(self._timestamp) + exp[10:]
                pattern = exp.replace("[", "\2")
                pattern = pattern.replace("]", "\3")
            elif exp[0:9].isdigit():
                self._timestamp += 1
                pattern = exp.replace("[", "\2")
                pattern = pattern.replace("]", "\3")
            else:
                pattern = exp.replace("[", "\2")
                pattern = pattern.replace("]", "\3")

            names = QUTest._last_record.replace("[", "\2")
            names = names.replace("]", "\3")

            if fnmatchcase(names, pattern):
                pass
            else:
                self._fail('got: "%s"'%(QUTest._last_record),
                           'exp: "%s"'%(exp))
                return False

            # is interactive?
            if self._is_inter and ignore:
                # consume & ignore all output produced
                while QSpy._receive():
                    print(QUTest._last_record)
                    pass

            return True

        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in expect: " + exp

    # ensure DSL command .....................................................
    def ensure(self, bool_expr):
        if bool_expr:
            return True
        else:
            #code_context = getframeinfo(stack()[1][0]).code_context
            #self._fail(''.join(code_context).strip())
            self._fail('ensure')
            return False

    # glb_filter DSL command .................................................
    def glb_filter(self, *args):
        # internal helper function
        def _apply(filter, mask, is_neg):
            if is_neg:
                return filter & ~mask
            else:
                return filter | mask

        filter = 0
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("glb_filter")
        elif self._state == QUTest._TEST:
            filter = 0 # 128-bit integer bitmask
            for arg in args:
                # NOTE: positive filter argument means 'add' (allow),
                # negative filter argument means 'remove' (disallow)
                is_neg = False
                if isinstance(arg, str):
                    is_neg = (arg[0] == '-') # is  request?
                    if is_neg:
                        arg = arg[1:]
                    try:
                        arg = QSpy._QS.index(arg)
                    except:
                        assert 0, 'invalid global filter arg="' + arg + '"'
                else:
                    is_neg = (arg < 0)
                    if is_neg:
                        arg = -arg

                if arg < 0x7F:
                    filter = _apply(filter, 1 << arg, is_neg)
                elif arg == QSpy._GRP_ALL:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_ALL, is_neg)
                elif arg == QSpy._GRP_SM:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_SM, is_neg)
                elif arg == QSpy._GRP_AO:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_AO, is_neg)
                elif arg == QSpy._GRP_QF:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_QF, is_neg)
                elif arg == QSpy._GRP_TE:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_TE, is_neg)
                elif arg == QSpy._GRP_EQ:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_EQ, is_neg)
                elif arg == QSpy._GRP_MP:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_MP, is_neg)
                elif arg == QSpy._GRP_SC:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_SC, is_neg)
                elif arg == QSpy._GRP_SEM:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_SEM, is_neg)
                elif arg == QSpy._GRP_MTX:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_MTX, is_neg)
                elif arg == QSpy._GRP_U0:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_U0, is_neg)
                elif arg == QSpy._GRP_U1:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_U1, is_neg)
                elif arg == QSpy._GRP_U2:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_U2, is_neg)
                elif arg == QSpy._GRP_U3:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_U3, is_neg)
                elif arg == QSpy._GRP_U4:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_U4, is_neg)
                elif arg == QSpy._GRP_UA:
                    filter = _apply(filter, QSpy._GLB_FLT_MASK_UA, is_neg)
                else:
                    assert 0, "invalid global filter arg=0x%X"%(arg)

            QSpy._sendTo(struct.pack("<BBQQ", QSpy._TRGT_GLB_FILTER, 16,
                                     filter & 0xFFFFFFFFFFFFFFFF,
                                     filter >> 64))

            self.expect("           Trg-Ack  QS_RX_GLB_FILTER")

        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in glb_filter"
        return filter

    # loc_filter DSL command .................................................
    def loc_filter(self, *args):
        # internal helper function
        def _apply(filter, mask, is_neg):
            if is_neg:
                return filter & ~mask
            else:
                return filter | mask

        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("loc_filter")
        elif self._state == QUTest._TEST:
            filter = 0 # 128-bit integer bitmask
            for arg in args:
                # NOTE: positive filter argument means 'add' (allow),
                # negative filter argument means 'remove' (disallow)
                is_neg = (arg < 0)
                if is_neg:
                    arg = -arg

                if arg < 0x7F:
                    filter = _apply(filter, 1 << arg, is_neg)
                elif arg == QSpy._IDS_ALL:
                    filter = _apply(filter, QSpy._LOC_FLT_MASK_ALL, is_neg)
                elif arg == QSpy._IDS_AO:
                    filter = _apply(filter, QSpy._LOC_FLT_MASK_AO, is_neg)
                elif arg == QSpy._IDS_EP:
                    filter = _apply(filter, QSpy._LOC_FLT_MASK_EP, is_neg)
                elif arg == QSpy._IDS_EQ:
                    filter = _apply(filter, QSpy._LOC_FLT_MASK_EQ, is_neg)
                elif arg == QSpy._IDS_AP:
                    filter = _apply(filter, QSpy._LOC_FLT_MASK_AP, is_neg)
                else:
                    assert 0, "invalid local filter arg=0x%X"%(arg)

            QSpy._sendTo(struct.pack("<BBQQ", QSpy._TRGT_LOC_FILTER, 16,
                                     filter & 0xFFFFFFFFFFFFFFFF,
                                     filter >> 64))

            self.expect("           Trg-Ack  QS_RX_LOC_FILTER")

        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in loc_filter"
        return filter

    # ao_filter DSL command ................................................
    def ao_filter(self, obj_id):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("ao_filter")
        elif self._state == QUTest._TEST:
            # NOTE: positive obj_id argument means 'add' (allow),
            # negative obj_id argument means 'remove' (disallow)
            remove = 0
            fmt = "<BB" + QSpy.trg_objPtr
            if isinstance(obj_id, str):
                if obj_id == '-': # is it remvoe request?
                    obj_id = obj_id[1:]
                    remove = 1
                QSpy._sendTo(struct.pack(
                    fmt, QSpy._QSPY_SEND_AO_FILTER, remove, 0),
                    obj_id) # add string object-ID to end
            else:
                if obj_id < 0:
                    obj_id = -obj_id
                    remove = 1
                QSpy._sendTo(struct.pack(
                    fmt, QSpy._TRGT_AO_FILTER, remove, obj_id))

            self.expect("           Trg-Ack  QS_RX_AO_FILTER")

        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in ao_filter"

    # current_obj DSL command ................................................
    def current_obj(self, obj_kind, obj_id):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("current_obj")
        elif self._state == QUTest._TEST:
            fmt = "<BB" + QSpy.trg_objPtr
            # Build packet according to obj_id type
            if isinstance(obj_id, int):
                QSpy._sendTo(struct.pack(
                    fmt, QSpy._TRGT_CURR_OBJ, obj_kind, obj_id))
            else:
                QSpy._sendTo(struct.pack(
                    fmt, QSpy._QSPY_SEND_CURR_OBJ, obj_kind, 0),
                    obj_id) # add string object ID to end
            self.expect("           Trg-Ack  QS_RX_CURR_OBJ")

        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in current_obj"

    # query_curr DSL command .................................................
    def query_curr(self, obj_kind):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("query_curr")
        elif self._state == QUTest._TEST:
            QSpy._sendTo(struct.pack("<BB", QSpy._TRGT_QUERY_CURR, obj_kind))
            # test-specific expect
            if self._is_inter:
                self.expect("@timestamp *")
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in query_curr"

    # expect_pause DSL command ...............................................
    def expect_pause(self):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("expect_pause")
        elif self._state == QUTest._TEST:
            self.expect("           TstPause")
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in expect_pause"

    # continue_test DSL command ..............................................
    def continue_test(self):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("continue_test")
        elif self._state == QUTest._TEST:
            QSpy._sendTo(struct.pack("<B", QSpy._TRGT_CONTINUE))
            self.expect("           Trg-Ack  QS_RX_TEST_CONTINUE")
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in continue_test"

    # expect_run DSL command ...............................................
    def expect_run(self):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("expect_run")
        elif self._state == QUTest._TEST:
            self.expect("           QF_RUN")
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in expect_run"

    # command DSL command ....................................................
    def command(self, cmdId, param1 = 0, param2 = 0, param3 = 0):
        if self._to_skip > 0:
            pass # ignore
        if self._state == QUTest._INIT:
            self._before_test("command")
        elif self._state == QUTest._TEST:
            fmt = "<BBIII"
            if isinstance(cmdId, int):
                QSpy._sendTo(struct.pack(fmt, QSpy._TRGT_COMMAND,
                                     cmdId, param1, param2, param3))
            else:
                QSpy._sendTo(struct.pack(
                    fmt, QSpy._QSPY_SEND_COMMAND, 0, param1, param2, param3),
                    cmdId) # add string command ID to end
            self.expect("           Trg-Ack  QS_RX_COMMAND", True)
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in command"

    # init DSL command .......................................................
    def init(self, signal = 0, params = None):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("init")
        elif self._state == QUTest._TEST:
            QSpy._sendEvt(QSpy._EVT_INIT, signal, params)
            self.expect("           Trg-Ack  QS_RX_EVENT", True)
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in init"

    # dispatch DSL command ...................................................
    def dispatch(self, signal, params = None):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("dispatch")
        elif self._state == QUTest._TEST:
            QSpy._sendEvt(QSpy._EVT_DISPATCH, signal, params)
            self.expect("           Trg-Ack  QS_RX_EVENT", True)
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in dispatch"

    # post DSL command .......................................................
    def post(self, signal, params = None):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("post")
        elif self._state == QUTest._TEST:
            QSpy._sendEvt(QSpy._EVT_POST, signal, params)
            self.expect("           Trg-Ack  QS_RX_EVENT", True)
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in post"

    # publish DSL command ....................................................
    def publish(self, signal, params = None):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("publish")
        elif self._state == QUTest._TEST:
            QSpy._sendEvt(QSpy._EVT_PUBLISH, signal, params)
            self.expect("           Trg-Ack  QS_RX_EVENT", True)
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in publish"

    # probe DSL command ......................................................
    def probe(self, func, data):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("probe")
        elif self._state == QUTest._TEST:
            fmt = "<BI" + QSpy.trg_funPtr
            if isinstance(func, int):
                # Send directly to target
                QSpy._sendTo(struct.pack(
                    fmt, QSpy._TRGT_TEST_PROBE, data, func))
            else:
                # Send to QSpy to provide "func" from Fun Dictionary
                QSpy._sendTo(struct.pack(
                    fmt, QSpy._QSPY_SEND_TEST_PROBE, data, 0),
                    func) # add string func name to end
            self.expect("           Trg-Ack  QS_RX_TEST_PROBE")
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in probe"

    # tick DSL command .......................................................
    def tick(self, tick_rate = 0):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("tick")
        elif self._state == QUTest._TEST:
            QSpy._sendTo(struct.pack("<BB", QSpy._TRGT_TICK, tick_rate))
            self.expect("           Trg-Ack  QS_RX_TICK", True)
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in tick"

    # peek DSL command .......................................................
    def peek(self, offset, size, num):
        assert size == 1 or size == 2 or size == 4, \
            "Size must be 1, 2, or 4"
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("peek")
        elif self._state == QUTest._TEST:
            QSpy._sendTo(struct.pack("<BHBB", QSpy._TRGT_PEEK,
                offset, size, num))
            # explicit expectation of peek output
            if self._is_inter:
                self.expect("@timestamp *")
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in peek"

    # poke DSL command .......................................................
    def poke(self, offset, size, data):
        assert size == 1 or size == 2 or size == 4, \
            "Size must be 1, 2, or 4"
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("poke")
        elif self._state == QUTest._TEST:
            length = len(data)
            num = length // size
            QSpy._sendTo(struct.pack("<BHBB", QSpy._TRGT_POKE,
                         offset, size, num) + data)
            self.expect("           Trg-Ack  QS_RX_POKE")
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in poke"

    # fill DSL command .......................................................
    def fill(self, offset, size, num, item = 0):
        assert size == 1 or size == 2 or size == 4, \
            "Size must be 1, 2, or 4"
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("fill")
        elif self._state == QUTest._TEST:
            if size == 1:
                item_fmt = "B"
            elif size == 2:
                item_fmt = "H"
            elif size == 4:
                item_fmt = "L"
            else:
                assert False, "size for sendFill must be 1, 2, or 4!"
            fmt = "<BHBB" + item_fmt
            packet = struct.pack(fmt, QSpy._TRGT_FILL, offset, size, num, item)
            QSpy._sendTo(packet)
            self.expect("           Trg-Ack  QS_RX_FILL")
        elif self._state == QUTest._FAIL or self._state == QUTest._SKIP:
            pass # ignore
        else:
            assert 0, "invalid state in fill"

    # skip DSL command .......................................................
    def skip(self, nTests = 9999):
        if self._to_skip == 0: # not skipping already?
            self._to_skip = nTests

    # include DSL command ....................................................
    def include(self, fname):
        path = os.path.normcase(os.path.join(self._test_dir, fname))
        if not os.path.exists(path):
            raise FileNotFoundError(path)
        with open(path) as f:
            code = compile(f.read(), path, "exec")
            # execute the include script code in this instance of QUTest
            exec(code, self._DSL_dict)

    # test file/dir read-only accessors ......................................
    def test_file(self):
        return self._test_file

    def test_dir(self):
        return self._test_dir

    # last_rec DSL command ...................................................
    def last_rec(self):
        return QUTest._last_record

    # note DSL command .....................................................
    def note(self, msg, dest=0x3):
        if self._to_skip > 0:
            return
        if not (dest & QUTest._OPT_SCREEN) == 0:
            QUTest._display(msg)
        if not (dest & QUTest._OPT_TRACE) == 0:
            QSpy._qspy_show(msg, kind=0x0)

    # dummy callbacks --------------------------------------------------------
    def _dummy_on_reset(self):
        #print("_dummy_on_reset")
        self.expect("           QF_RUN")
        pass

    def _dummy_on_setup(self):
        #print("_dummy_on_setup")
        pass

    def _dummy_on_teardown(self):
        #print("_dummy_on_teardown")
        pass

    # helper methods ---------------------------------------------------------
    @staticmethod
    def _run_script(fname):
        QUTest._num_groups += 1

        with open(fname) as f:
            QUTest_inst = QUTest()
            QUTest_inst._test_file = fname
            QUTest_inst._test_dir = os.path.dirname(fname)

            QUTest._display("\n==================================[Group %2d]"
                "=================================="%(QUTest._num_groups))
            QUTest._display(fname, QUTest._COL_GRP1, QUTest._COL_GRP2)
            QSpy._qspy_show("\n==================================[Group %2d]"
                "==================================\n%s"%(
                QUTest._num_groups, fname))

            if not QUTest_inst._test_dir: # empty dir?
                QUTest_inst._test_dir = "."
            try:
                QUTest_inst._startTime = QUTest._time()
                code = compile(f.read(), fname, "exec")

                # the last test ended with assertion?
                if QUTest._have_assert:
                    QUTest._have_assert = False
                    if not QUTest._host_exe:
                        # ignore all input until timeout
                        while QSpy._receive():
                            pass

                # execute the script code in a separate instance of QUTest
                exec(code, QUTest_inst._DSL_dict)

            except ExitOnFailException:
                # QUTest_inst._fail() already done
                pass

            except RuntimeError as e:
                QUTest._display(str(e), QUTest._COL_ERR1, QUTest._COL_ERR2)
                QSpy._qspy_show(str(e))
                QUTest_inst._fail()

            except SyntaxError:
                QUTest._display(traceback.format_exc(limit=0),
                    QUTest._COL_ERR1, QUTest._COL_ERR2)
                QSpy._qspy_show(traceback.format_exc(limit=0))
                QUTest._num_failed += 1
                QUTest._str_failed += " G%d"%(QUTest._num_groups)

            except Exception:
                QUTest._display(traceback.format_exc(),
                    QUTest._COL_ERR1, QUTest._COL_ERR2)
                QUTest_inst._fail()

            if QUTest._opt_interactive:
                QUTest_inst._interact()

            # properly end the last test in the group
            QUTest_inst._test_end()
            QUTest._quit_host_exe()

    def _interact(self):
        self._is_inter  = True
        self._test_file = ''
        self._test_dir  = os.path.dirname('.')

        if self._state == QUTest._TEST:
            QUTest._num_skipped += 1
            QUTest._str_skipped += " %d"%(QUTest._num_tests)

        # consume & ignore all output produced so far
        while QSpy._receive():
            print(QUTest._last_record)
            pass

        # enter "interactive mode": commands entered as user input
        while True:
            code = input('>>> ')
            if code:
                try:
                    exec(code, self._DSL_dict)
                except:
                    traceback.print_exc(2)
            else:
                break

    def _tran(self, state):
        #print("tran(%d->%d)"%(self._state, state))
        self._state = state

    def _test_end(self):
        if not self._state == QUTest._TEST:
            return

        if not QUTest._have_info:
            return

        if self._is_inter:
            QUTest._display("                                             "
                "                      [ SKIPPED ]")
            QSpy._qspy_show("                                             "
                "                      [ SKIPPED ]")
            return

        elapsed = QUTest._time() - self._startTime
        if QUTest._have_assert:
            QUTest._display("                                                "
                "             [ PASS (%5.1fs) ]"%(elapsed),
                            QUTest._COL_PASS1, QUTest._COL_PASS2)
            QSpy._qspy_show("[%2d]-------------------------------------------"
               "--------------[ PASS (%5.1fs) ]"%(QUTest._num_tests, elapsed))
            return

        QSpy._sendTo(struct.pack("<B", QSpy._TRGT_TEST_TEARDOWN))
        if not QSpy._receive(): # timeout?
            self._fail('got: "" (timeout)',
                       'exp: end-of-test')
            return

        exp = "           Trg-Ack  QS_RX_TEST_TEARDOWN"
        if QUTest._last_record == exp:

            self._DSL_dict["on_teardown"]() # on_teardown() callback

            QUTest._display("                                                "
                "             [ PASS (%5.1fs) ]"%(elapsed),
                            QUTest._COL_PASS1, QUTest._COL_PASS2)
            QSpy._qspy_show("[%2d]-------------------------------------------"
               "--------------[ PASS (%5.1fs) ]"%(QUTest._num_tests, elapsed))
            return
        else:
            self._fail('got: "%s"'%(QUTest._last_record),
                       'exp: end-of-test')
            # ignore all input until timeout
            while QSpy._receive():
                pass

    def _reset_target(self):
        if QUTest._host_exe:
            if not QUTest._have_assert:
                #print("quitting exe")
                QUTest._quit_host_exe()

            # lauch a new instance of the host executable
            QUTest._have_target = True
            QUTest._have_info = False
            Popen(QUTest._host_exe)

        else: # running remote target
            QUTest._have_target = True
            QUTest._have_info = False
            if not (QUTest._is_debug or QUTest._have_assert):
                QSpy._sendTo(struct.pack("<B", QSpy._TRGT_RESET))

        # ignore all input until have-target-info or timeout
        while QSpy._receive():
            if QUTest._have_info:
                break

        if QUTest._have_info:
            QUTest._have_assert = False;
            QUTest._need_reset  = False;
        else:
            QUTest._quit_host_exe()
            raise RuntimeError("Target reset failed")

        self._timestamp = 0
        self._DSL_dict["on_reset"]()
        return (self._state == QUTest._TEST)

    def _on_setup(self):
        assert self._state == QUTest._TEST, \
            "on_setup() outside the TEST state %d"%(self._state)
        if not QUTest._have_info:
            return False

        QSpy._sendTo(struct.pack("<B", QSpy._TRGT_TEST_SETUP))
        self.expect("           Trg-Ack  QS_RX_TEST_SETUP")
        if self._state == QUTest._TEST:
            self._timestamp = 0
            self._DSL_dict["on_setup"]()
            return True

    def _before_test(self, command):
        QUTest._num_failed += 1
        self._tran(QUTest._FAIL)
        msg = '"' + command + '" before any test'
        raise SyntaxError(msg)

    def _fail(self, err = "", exp = ""):
        stk = stack()
        max = len(stk) - 3
        lvl = 2
        while lvl < max:
            fun = getframeinfo(stk[lvl][0]).function
            if fun == "<module>":
                QUTest._display("  @%s:%d"%(
                      getframeinfo(stk[lvl][0]).filename,
                      getframeinfo(stk[lvl][0]).lineno))
            else:
                QUTest._display("  @%s/%s:%d"%(
                      getframeinfo(stk[lvl][0]).filename,
                      getframeinfo(stk[lvl][0]).function,
                      getframeinfo(stk[lvl][0]).lineno))
            lvl += 1
        if exp != "":
            QUTest._display(exp, QUTest._COL_EXP1, QUTest._COL_EXP2)
        if err == "ensure":
            if 2 < max:
                err = ''.join(getframeinfo(stk[2][0]).code_context).strip()
            QUTest._display(err, QUTest._COL_ERR1, QUTest._COL_ERR2)
        elif err != "":
            QUTest._display(err, QUTest._COL_ERR1, QUTest._COL_ERR2)

        if self._is_inter:
            return

        elapsed = QUTest._time() - self._startTime
        QUTest._display("! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! "
            "! ! ! ! ! ! ! ! ![ FAIL (%5.1fs) ]"%(elapsed),
            QUTest._COL_FAIL1, QUTest._COL_FAIL2)
        QSpy._qspy_show("[%2d]! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! "
            "! ! ! ! ! ! ! ! ![ FAIL (%5.1fs) ]"%(
                QUTest._num_tests, elapsed))
        QUTest._num_failed += 1
        QUTest._str_failed += " %d"%(QUTest._num_tests)
        QUTest._need_reset = True
        self._tran(QUTest._FAIL)
        if QUTest._opt_exit_on_fail:
            raise ExitOnFailException

    @staticmethod
    def _quit_host_exe():
        if QUTest._host_exe and QUTest._have_target:
            QUTest._have_target = False
            QSpy._sendTo(struct.pack("<B", QSpy._TRGT_RESET))
            time.sleep(QUTest._TOUT) # wait until host-exe quits

    @staticmethod
    def _time():
        if sys.version_info[0] == 2: # Python 2 ?
            return time.time()
        else: # Python 3+
            return time.perf_counter()

    @staticmethod
    def _display(msg, c1='', c2='', eol=True):
        if QUTest._log_file:
            QUTest._log_file.write(msg)
            if eol:
                QUTest._log_file.write('\n')
        if not c1 == '':
            msg = c1 + msg
        if not c2 == '':
            msg = msg + c2
        if eol:
            print(msg)
        else:
            print(msg, end='')

class ExitOnFailException(Exception):
    # test failed and exit-on-fail (-x) is set
    pass

#=============================================================================
# Helper class for communication with the QSpy front-end
#
class QSpy:
    # private class variables...
    _sock = None
    _is_attached = False
    _tx_seq = 0
    _host_udp = ["localhost", 7701] # list, to be converted to a tuple
    _local_port = 0 # let the OS decide the best local port

    # formats of various packet elements from the Target
    trg_QPver    = 0
    trg_objPtr   = "L"
    trg_funPtr   = "L"
    trg_tsize    = "L"
    trg_sig      = "H"
    trg_evtSize  = "H"
    trg_queueCtr = "B"
    trg_poolCtr  = "H"
    trg_poolBlk  = "H"
    trg_tevtCtr  = "H"
    trg_tstamp   = "UNKNOWN"

    # packets from QSPY...
    _PKT_TEXT_ECHO   = 0
    _PKT_TARGET_INFO = 64
    _PKT_ASSERTION   = 69
    _PKT_ATTACH_CONF = 128
    _PKT_DETACH      = 129

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

    # packets to QSpy only...
    _QSPY_ATTACH          = 128
    _QSPY_DETACH          = 129
    _QSPY_SAVE_DICT       = 130
    _QSPY_TEXT_OUT        = 131
    _QSPY_BIN_OUT         = 132
    _QSPY_MATLAB_OUT      = 133
    _QSPY_SEQUENCE_OUT    = 134
    _QSPY_CLEAR_SCREEN    = 140
    _QSPY_SHOW_NOTE       = 141

    # packets to QSpy to be "massaged" and forwarded to the Target...
    _QSPY_SEND_EVENT      = 135
    _QSPY_SEND_AO_FILTER  = 136
    _QSPY_SEND_CURR_OBJ   = 137
    _QSPY_SEND_COMMAND    = 138
    _QSPY_SEND_TEST_PROBE = 139

    # gloal filter groups...
    _GRP_ALL= 0xF0
    _GRP_SM = 0xF1
    _GRP_AO = 0xF2
    _GRP_MP = 0xF3
    _GRP_EQ = 0xF4
    _GRP_TE = 0xF5
    _GRP_QF = 0xF6
    _GRP_SC = 0xF7
    _GRP_SEM= 0xF8
    _GRP_MTX= 0xF9
    _GRP_U0 = 0xFA
    _GRP_U1 = 0xFB
    _GRP_U2 = 0xFC
    _GRP_U3 = 0xFD
    _GRP_U4 = 0xFE
    _GRP_UA = 0xFF

    _QS_USER = 100

    # local filter groups...
    _IDS_ALL= 0xF0
    _IDS_AO = (0x80 + 0)
    _IDS_EP = (0x80 + 64)
    _IDS_EQ = (0x80 + 80)
    _IDS_AP = (0x80 + 96)

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

    # tuple of QS records from the Target.
    # !!! NOTE: Must match qpc/include/qs.h !!!
    _QS = ("QS_EMPTY",
        # [1] SM records
        "QS_QEP_STATE_ENTRY",     "QS_QEP_STATE_EXIT",
        "QS_QEP_STATE_INIT",      "QS_QEP_INIT_TRAN",
        "QS_QEP_INTERN_TRAN",     "QS_QEP_TRAN",
        "QS_QEP_IGNORED",         "QS_QEP_DISPATCH",
        "QS_QEP_UNHANDLED",

        # [10] Active Object (AO) records
        "QS_QF_ACTIVE_DEFER",     "QS_QF_ACTIVE_RECALL",
        "QS_QF_ACTIVE_SUBSCRIBE", "QS_QF_ACTIVE_UNSUBSCRIBE",
        "QS_QF_ACTIVE_POST",      "QS_QF_ACTIVE_POST_LIFO",
        "QS_QF_ACTIVE_GET",       "QS_QF_ACTIVE_GET_LAST",
        "QS_QF_ACTIVE_RECALL_ATTEMPT",

        # [19] Event Queue (EQ) records
        "QS_QF_EQUEUE_POST",      "QS_QF_EQUEUE_POST_LIFO",
        "QS_QF_EQUEUE_GET",       "QS_QF_EQUEUE_GET_LAST",

        # [23] Framework (QF) records
        "QS_QF_NEW_ATTEMPT",

        # [24] Memory Pool (MP) records
        "QS_QF_MPOOL_GET",        "QS_QF_MPOOL_PUT",

        # [26] Additional Framework (QF) records
        "QS_QF_PUBLISH",          "QS_QF_NEW_REF",
        "QS_QF_NEW",              "QS_QF_GC_ATTEMPT",
        "QS_QF_GC",               "QS_QF_TICK",

        # [32] Time Event (TE) records
        "QS_QF_TIMEEVT_ARM",      "QS_QF_TIMEEVT_AUTO_DISARM",
        "QS_QF_TIMEEVT_DISARM_ATTEMPT", "QS_QF_TIMEEVT_DISARM",
        "QS_QF_TIMEEVT_REARM",    "QS_QF_TIMEEVT_POST",

        # [38] Additional (QF) records
        "QS_QF_DELETE_REF",       "QS_QF_CRIT_ENTRY",
        "QS_QF_CRIT_EXIT",        "QS_QF_ISR_ENTRY",
        "QS_QF_ISR_EXIT",         "QS_QF_INT_DISABLE",
        "QS_QF_INT_ENABLE",

        # [45] Additional Active Object (AO) records
        "QS_QF_ACTIVE_POST_ATTEMPT",

        # [46] Additional Event Queue (EQ) records
        "QS_QF_EQUEUE_POST_ATTEMPT",

        # [47] Additional Memory Pool (MP) records
        "QS_QF_MPOOL_GET_ATTEMPT",

        # [48] Scheduler (SC) records
        "QS_SCHED_PREEMPT",       "QS_SCHED_RESTORE",
        "QS_SCHED_LOCK",          "QS_SCHED_UNLOCK",
        "QS_SCHED_NEXT",          "QS_SCHED_IDLE",

        # [54] Miscellaneous QS records (not maskable)
        "QS_ENUM_DICT",

        # [55] Additional QEP records
        "QS_QEP_TRAN_HIST",       "QS_QEP_TRAN_EP",
        "QS_QEP_TRAN_XP",

        # [58] Miscellaneous QS records (not maskable)
        "QS_TEST_PAUSED",         "QS_TEST_PROBE_GET",
        "QS_SIG_DICT",            "QS_OBJ_DICT",
        "QS_FUN_DICT",            "QS_USR_DICT",
        "QS_TARGET_INFO",         "QS_TARGET_DONE",
        "QS_RX_STATUS",           "QS_QUERY_DATA",
        "QS_PEEK_DATA",           "QS_ASSERT_FAIL",
        "QS_QF_RUN",

        # [71] Semaphore (SEM) records
        "QS_SEM_TAKE",            "QS_SEM_BLOCK",
        "QS_SEM_SIGNAL",          "QS_SEM_BLOCK_ATTEMPT",

        # [75] Mutex (MTX) records
        "QS_MTX_LOCK",            "QS_MTX_BLOCK",
        "QS_MTX_UNLOCK",          "QS_MTX_LOCK_ATTEMPT",
        "QS_MTX_BLOCK_ATTEMPT",   "QS_MTX_UNLOCK_ATTEMPT",

        # [81] Reserved QS records
                                  "QS_RESERVED_81",
        "QS_RESERVED_82",         "QS_RESERVED_83",
        "QS_RESERVED_84",         "QS_RESERVED_85",
        "QS_RESERVED_86",         "QS_RESERVED_87",
        "QS_RESERVED_88",         "QS_RESERVED_89",
        "QS_RESERVED_90",         "QS_RESERVED_91",
        "QS_RESERVED_92",         "QS_RESERVED_93",
        "QS_RESERVED_94",         "QS_RESERVED_95",
        "QS_RESERVED_96",         "QS_RESERVED_97",
        "QS_RESERVED_98",         "QS_RESERVED_99",

        # [100] Application-specific (User) QS records
        "QS_USER_00",             "QS_USER_01",
        "QS_USER_02",             "QS_USER_03",
        "QS_USER_04",             "QS_USER_05",
        "QS_USER_06",             "QS_USER_07",
        "QS_USER_08",             "QS_USER_09",
        "QS_USER_10",             "QS_USER_11",
        "QS_USER_12",             "QS_USER_13",
        "QS_USER_14",             "QS_USER_15",
        "QS_USER_16",             "QS_USER_17",
        "QS_USER_18",             "QS_USER_19",
        "QS_USER_20",             "QS_USER_21",
        "QS_USER_22",             "QS_USER_23",
        "QS_USER_24")

    # global filter masks
    _GLB_FLT_MASK_ALL= 0x1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
    _GLB_FLT_MASK_SM = 0x000000000000000003800000000003FE
    _GLB_FLT_MASK_AO = 0x0000000000000000000020000007FC00
    _GLB_FLT_MASK_QF = 0x000000000000000000001FC0FC800000
    _GLB_FLT_MASK_TE = 0x00000000000000000000003F00000000
    _GLB_FLT_MASK_EQ = 0x00000000000000000000400000780000
    _GLB_FLT_MASK_MP = 0x00000000000000000000800003000000
    _GLB_FLT_MASK_SC = 0x0000000000000000003F000000000000
    _GLB_FLT_MASK_SEM= 0x00000000000007800000000000000000
    _GLB_FLT_MASK_MTX= 0x000000000001F8000000000000000000
    _GLB_FLT_MASK_U0 = 0x000001F0000000000000000000000000
    _GLB_FLT_MASK_U1 = 0x00003E00000000000000000000000000
    _GLB_FLT_MASK_U2 = 0x0007C000000000000000000000000000
    _GLB_FLT_MASK_U3 = 0x00F80000000000000000000000000000
    _GLB_FLT_MASK_U4 = 0x1F000000000000000000000000000000
    _GLB_FLT_MASK_UA = 0x1FFFFFF0000000000000000000000000

    # local filter masks
    _LOC_FLT_MASK_ALL= 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
    _LOC_FLT_MASK_AO = 0x0000000000000001FFFFFFFFFFFFFFFE
    _LOC_FLT_MASK_EP = 0x000000000000FFFE0000000000000000
    _LOC_FLT_MASK_EQ = 0x00000000FFFF00000000000000000000
    _LOC_FLT_MASK_AP = 0xFFFFFFFF000000000000000000000000

    @staticmethod
    def _init():
        # Create socket
        QSpy._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        QSpy._sock.settimeout(QUTest._TOUT) # timeout for blocking socket
        #bufsize = QSpy._sock.getsockopt(socket.SOL_UDP, socket.SO_RCVBUF)
        #print("SO_RCVBUF ", bufsize)
        try:
            QSpy._sock.bind(("0.0.0.0", QSpy._local_port))
            #print("bind: ", ("0.0.0.0", QSpy._local_port))
        except:
            messagebox.showerror("UDP Socket Error",
               "Can't bind the UDP socket\nto the specified local_host")
            QSpyView._gui.destroy()
            return -1
        return 0

    @staticmethod
    def _attach(channels = 0x2):
        # channels: 1-binary, 2-text, 3-both
        print("Attaching to QSpy (%s:%d) ... "%(
              QSpy._host_udp[0], QSpy._host_udp[1]), end='')
        QSpy._is_attached = False
        QSpy._sendTo(struct.pack("<BB", QSpy._QSPY_ATTACH, channels))
        try:
            QSpy._receive()
        except:
            pass

        if QSpy._is_attached:
            print("OK\n")
            return True
        else:
            print("FAILED\n")
            return False

    @staticmethod
    def _detach():
        if QSpy._sock is None:
            return
        QSpy._sendTo(struct.pack("<B", QSpy._QSPY_DETACH))
        time.sleep(QUTest._TOUT)
        #QSpy._sock.shutdown(socket.SHUT_RDWR)
        QSpy._sock.close()
        QSpy._sock = None

    ## returns True if packet received, False if timed out
    @staticmethod
    def _receive():
        if not QUTest._is_debug:
            try:
                packet = QSpy._sock.recv(4096)
            except socket.timeout:
                QUTest._last_record = ""
                return False # timeout
            # don"t catch OSError
        else:
            while True:
                try:
                    packet = QSpy._sock.recv(4096)
                    break
                except socket.timeout:
                    print("\nwaiting for Target "\
                        "(press Enter to quit this test)...", end="")
                    if os.name == "nt":
                        if msvcrt.kbhit():
                            if msvcrt.getch() == b'\r':
                                print("quit")
                                return False; # timeout
                    else:
                        dr,dw,de = select.select([sys.stdin], [], [], 0)
                        if dr != []:
                            sys.stdin.readline() # consume the Return key
                            print("quit")
                            return False; # timeout
                # don"t catch OSError

        dlen = len(packet)
        if dlen < 2:
            QUTest._last_record = ""
            raise RuntimeError("UDP packet from QSpy too short")

        recID = packet[1]
        if recID == QSpy._PKT_TEXT_ECHO: # text packet (most common)
            QUTest._last_record = packet[3:].decode("utf-8")
            # QS_ASSERTION?
            if dlen > 3 and packet[2] == QSpy._PKT_ASSERTION:
                QUTest._have_assert = True
                QUTest._have_target = False
                QUTest._need_reset  = True
                if QSpy.trg_QPver < 720: # before QP 7.2.0?
                    QSpy._sendTo(struct.pack("<B", QSpy._TRGT_RESET))

        elif recID == QSpy._PKT_TARGET_INFO: # target info?
            QUTest._last_record = ""
            if dlen != 18:
                raise RuntimeError("Incorrect Target info")

            QSpy.trg_QPver = packet[3] + (packet[4] << 8)

            fmt = "xBHxLxxxQ"
            trg_info = packet[5:18]
            QSpy.trg_objPtr  = fmt[trg_info[3] & 0x0F]
            QSpy.trg_funPtr  = fmt[trg_info[3] >> 4]
            QSpy.trg_tsize   = fmt[trg_info[4] & 0x0F]
            QSpy.trg_sig     = fmt[trg_info[0] & 0x0F]
            QSpy.trg_evtSize = fmt[trg_info[0] >> 4]
            QSpy.trg_queueCtr= fmt[trg_info[1] & 0x0F]
            QSpy.trg_poolCtr = fmt[trg_info[2] >> 4]
            QSpy.trg_poolBlk = fmt[trg_info[2] & 0x0F]
            QSpy.trg_tevtCtr = fmt[trg_info[1] >> 4]
            QSpy.trg_tstamp = "%02d%02d%02d_%02d%02d%02d"%(
                   trg_info[12], trg_info[11], trg_info[10],
                   trg_info[9], trg_info[8], trg_info[7])
            #print("Target:", QSpy.trg_tstamp)
            QUTest._have_info = True

        elif recID == QSpy._PKT_ATTACH_CONF:
            QUTest._last_record = ""
            QSpy._is_attached = True

        elif recID == QSpy._PKT_DETACH:
            QUTest._quit_host_exe()
            QUTest._last_record = ""
            QSpy._detach()
            QSpy._is_attached = False

        else:
            QUTest._last_record = ""
            raise RuntimeError("Unrecognized UDP packet type from QSpy")

        return True # some input received

    @staticmethod
    def _sendTo(packet, str=None):
        tx_packet = bytearray([QSpy._tx_seq])
        tx_packet.extend(packet)
        if str is not None:
            tx_packet.extend(bytes(str, "utf-8"))
            tx_packet.extend(b"\0") # zero-terminate
        QSpy._sock.sendto(tx_packet, QSpy._host_udp)
        QSpy._tx_seq = (QSpy._tx_seq + 1) & 0xFF
        #print("sendTo", QSpy._tx_seq)

    @staticmethod
    def _sendEvt(ao_prio, signal, parameters = None):
        fmt = "<BB" + QSpy.trg_sig + "H"
        if parameters is not None:
            length = len(parameters)
        else:
            length = 0

        if isinstance(signal, int):
            packet = bytearray(struct.pack(
                fmt, QSpy._TRGT_EVENT, ao_prio, signal, length))
            if parameters is not None:
                packet.extend(parameters)
            QSpy._sendTo(packet)
        else:
            packet = bytearray(struct.pack(
                fmt, QSpy._QSPY_SEND_EVENT, ao_prio, 0, length))
            if parameters is not None:
                packet.extend(parameters)
            QSpy._sendTo(packet, signal)

    @staticmethod
    def _qspy_show(note, kind=0xFF):
        QSpy._sendTo(struct.pack("<BB", QSpy._QSPY_SHOW_NOTE, kind), note)

#=============================================================================
# main entry point to QUTest
def main():
    startTime = QUTest._time()
    tstamp = datetime.now().strftime("%y%m%d_%H%M%S")

    # on Windows enable color escape characters in the console...
    if os.name == "nt":
        os.system("color")

    # parse command-line arguments...
    parser = argparse.ArgumentParser(
        prog="python qutest.py",
        description="QUTest test script runner",
        epilog="More info: https://www.state-machine.com/qtools/qutest.html")
    parser.add_argument('-v', '--version',
        action='version',
        version="QUTest script runner %d.%d.%d on Python %s"%(
                    QUTest.VERSION//100, (QUTest.VERSION//10) % 10,
                    QUTest.VERSION % 10,
            python_version()),
        help='Display QUTest version')

    parser.add_argument('-e', '--exe', nargs='?', default='', const='',
        help="Optional host executable or debug/DEBUG")
    parser.add_argument('-q', '--qspy', nargs='?', default='', const='',
        help="optional qspy host, [:ud_port][:tcp_port]")
    parser.add_argument('-l', '--log', nargs='?', default='', const='',
        help="Optional log directory (might not exist yet)")
    parser.add_argument('-o', '--opt', nargs='?', default='', const='',
        help="xciob: x:exit-on-fail,i:inter,"
             "c:qspy-clear,o:qspy-save-txt,b:qspy-save-bin")
    parser.add_argument('scripts', nargs='*',
                        help="List (comma-separated) of test scripts to run")
    args = parser.parse_args()
    #print(args)

    print("\nQUTest unit testing front-end %d.%d.%d running on Python %s"%(
            QUTest.VERSION//100,
            (QUTest.VERSION//10) % 10,
             QUTest.VERSION % 10, python_version()))
    print("Copyright (c) 2005-2022 Quantum Leaps, www.state-machine.com")

    # process command-line argumens...
    if args.exe != '':
        if args.exe == 'debug' or args.exe == 'DEBUG':
            QUTest._is_debug = True
        else:
            host_exe = glob(args.exe)
            if host_exe:
                QUTest._host_exe = [host_exe[0], "localhost:6601"]
            else:
                print("\nProvided test executable '%s' not found\n"%(
                      args.exe))
                return sys.exit(-1)

    if args.qspy != '':
        qspy_conf = args.qspy.split(":")
        if len(qspy_conf) > 0 and not qspy_conf[0] == '':
            QSpy._host_udp[0] = qspy_conf[0]
        if len(qspy_conf) > 1 and not qspy_conf[1] == '':
            QSpy._host_udp[1] = int(qspy_conf[1])
        if len(qspy_conf) > 2 and not qspy_conf[2] == '':
            if QUTest._host_exe:
                QUTest._host_exe[1] = (QSpy._host_udp[0] + ":" + qspy_conf[2])
            else:
                print("\nTCP port specified without host executable\n")
                return sys.exit(-1)

    log = args.log
    if log != '':
        if not (log.endswith('/') or log.endswith('\\')):
            log += '/'
        try:
            os.makedirs(os.path.dirname(log), exist_ok=True)
        except:
            pass
        if os.path.isdir(log): # is arg.log a directory
            log += "qutest" + tstamp + ".txt"
        else: # not a directory
            log = ''
            print("\nWrong directory for log output\n")
            return sys.exit(-1)

    # convert to immutable tuples
    QSpy._host_udp = tuple(QSpy._host_udp)
    if QUTest._host_exe:
        QUTest._host_exe = tuple(QUTest._host_exe)

    QUTest._opt_exit_on_fail  = 'x' in args.opt
    QUTest._opt_interactive   = 'i' in args.opt
    QUTest._opt_clear_qspy    = 'c' in args.opt
    QUTest._opt_save_qspy_txt = 'o' in args.opt
    QUTest._opt_save_qspy_bin = 'b' in args.opt

    if not args.scripts: # scripts not provided?
        #print("applying default *.py")
        args.scripts = ['*.py'] # apply the default "*.py"
    scripts = []
    for s in args.scripts:
        if s.endswith('.exe'):
            print("\nIncorrect test script(s). Old QUTEST command-line?\n")
            parser.print_help()
            return sys.exit(-1)
        scripts.extend(glob(s))
    # still no scripts?
    if (not scripts) and (not QUTest._opt_interactive):
        print("\nFound no test scripts to run")
        return sys.exit(0)

    #print("scripts:", scripts)
    #print("host_exe:", QUTest._host_exe)
    #print("debug:", QUTest._is_debug)
    #print("_log_file:", log)
    #print("host_udp:", QSpy._host_udp)
    #print("opt: x", QUTest._opt_exit_on_fail)
    #print("opt: i", QUTest._opt_interactive)
    #print("opt: c", QUTest._opt_clear_qspy)
    #print("opt: o", QUTest._opt_save_qspy_txt)
    #print("opt: o", QUTest._opt_save_qspy_bin)
    #return 0

    # init QSpy socket
    err = QSpy._init()
    if err:
        return sys.exit(err)

    # attach the the QSPY Back-End...
    if not QSpy._attach():
        return sys.exit(-1)

    if QUTest._opt_clear_qspy:
        QSpy._sendTo(struct.pack("<B", QSpy._QSPY_CLEAR_SCREEN))

    # open the log file only after potential initialization errors
    if log != '': # log file provided?
        try:
            QUTest._log_file = open(log, 'w')
        except:
            print("Requested log file cannot be opened")
            return sys.exit(-1)

    if QUTest._opt_save_qspy_txt:
         QSpy._sendTo(struct.pack("<BB", QSpy._QSPY_TEXT_OUT, 1))
    if QUTest._opt_save_qspy_bin:
         QSpy._sendTo(struct.pack("<BB", QSpy._QSPY_BIN_OUT, 1))

    msg = "Run ID    : %s"%(tstamp)
    QUTest._display(msg)
    QSpy._qspy_show(msg)
    if QUTest._host_exe:
        msg = "Target    : %s"%(QUTest._host_exe[0])
    else:
       msg = "Target    : remote"
    QUTest._display(msg)
    QSpy._qspy_show(msg)

    # run all the test scripts...
    if scripts:
        for scr in scripts:
            QUTest._run_script(scr)
            # errors encountered? and shall we exit on failure?
            if (QUTest._num_failed != 0) and QUTest._opt_exit_on_fail:
                break
    elif QUTest._opt_interactive:
        QUTest_inst = QUTest()
        QUTest_inst._interact()

    # print the SUMMARY of the run...
    elapsed = QUTest._time() - startTime
    msg = "\n==================================[ SUMMARY ]"\
          "=================================\n"
    QUTest._display(msg)
    QSpy._qspy_show(msg)

    if QUTest._have_info:
        msg = "Target ID : %s (QP-Ver=%3d)"%(QSpy.trg_tstamp, QSpy.trg_QPver)
    else:
        msg = "Target ID : N/A"
    QUTest._display(msg)
    QSpy._qspy_show(msg)

    if QUTest._log_file:
        msg = "Log file  : %s"%(log)
    else:
        msg = "Log file  :"
    QUTest._display(msg)
    QSpy._qspy_show(msg)

    msg = "Groups    : %d"%(QUTest._num_groups)
    QUTest._display(msg)
    QSpy._qspy_show(msg)

    msg = "Tests     : %d"%(QUTest._num_tests)
    QUTest._display(msg)
    QSpy._qspy_show(msg)

    if QUTest._num_skipped == 0:
        msg = "Skipped   : 0"
    else:
        msg = "Skipped   : %d [%s ]"%(QUTest._num_skipped,QUTest._str_skipped)
    QUTest._display(msg)
    QSpy._qspy_show(msg)

    if QUTest._num_failed == 0:
        msg = "Failed    : 0"
    else:
        msg = "FAILED    : %d [%s ]"%(QUTest._num_failed, QUTest._str_failed)
    QUTest._display(msg)
    QSpy._qspy_show(msg)

    if QUTest._num_failed == 0:
        msg = "\n==============================[  OK  (%5.1fs) ]"\
                "==============================="%(elapsed)
        QUTest._display(msg, QUTest._COL_OK1, QUTest._COL_OK2)
        QSpy._qspy_show(msg)
    else:
        msg = "\n==============================[ FAIL (%5.1fs) ]"\
                "==============================="%(elapsed)
        QUTest._display(msg, QUTest._COL_NOK1, QUTest._COL_NOK2)
        QSpy._qspy_show(msg)

        if QUTest._opt_exit_on_fail:
            msg = "Exiting after first failure (-x)"
            QUTest._display(msg);
            QSpy._qspy_show(msg)

    # cleanup...
    if QUTest._log_file:
        QUTest._log_file.close()

    if QUTest._opt_save_qspy_txt:
         QSpy._sendTo(struct.pack("<BB", QSpy._QSPY_TEXT_OUT, 0))
    if QUTest._opt_save_qspy_bin:
         QSpy._sendTo(struct.pack("<BB", QSpy._QSPY_BIN_OUT, 0))

    QUTest._quit_host_exe()
    QSpy._detach()

    return sys.exit(QUTest._num_failed) # report to the caller (e.g., make)

#=============================================================================
if __name__ == "__main__":
    main()
