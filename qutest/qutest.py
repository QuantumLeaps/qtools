#!/usr/bin/env python

#=============================================================================
# QUTest Python scripting support
#
#                    Q u a n t u m  L e a P s
#                    ------------------------
#                    Modern Embedded Software
#
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

# pylint: disable=missing-module-docstring,
# pylint: disable=missing-class-docstring,
# pylint: disable=missing-function-docstring
# pylint: disable=broad-except
# pylint: disable=superfluous-parens

from fnmatch import fnmatchcase
from glob import glob
from platform import python_version
from datetime import datetime
from subprocess import Popen
from inspect import getframeinfo, stack

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

#=============================================================================
# QUTest test-script runner
# https://www.state-machine.com/qtools/qutest_script.html
#
class QUTest:

    # public class constants
    VERSION = 810
    TIMEOUT = 1.000 # timeout value [seconds]

    # private class variables
    _host_exe    = [None, None] # list to be convered to a tuple
    _have_target = False
    _have_info   = False
    _have_assert = False
    _need_reset  = True
    _is_debug    = False
    _num_groups  = 0
    _test_num    = 0
    _num_failed  = 0
    _num_skipped = 0
    _str_failed  = ''
    _str_skipped = ''
    _last_record = ''
    _log_file    = None
    _test_start  = 0

    # private command-line options
    _opt_trace         = False
    _opt_exit_on_fail  = False
    _opt_interactive   = False
    _opt_clear_qspy    = False
    _opt_save_qspy_txt = False
    _opt_save_qspy_bin = False

    # private states of the internal QUTest state machine
    _INIT = 0
    _TEST = 1
    _FAIL = 2
    _SKIP = 3

    # private options for implemented commands
    _OPT_NORESET  = 0x01  # for test() command (NORESET tests)
    _OPT_SCREEN   = 0x01  # for note() command (SCREEN destination)
    _OPT_TRACE    = 0x02  # for note() command (TRACE  destination)

    # private colors/backgrounds for screen output...
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
        self._state      = QUTest._INIT
        self._timestamp  = 0
        self._to_skip    = 0
        self._is_inter   = False
        self._test_fname = ""
        self._test_dname = ""
        self._context    = Context_()

        # The following _dsl_dict dictionary defines the QUTest testing
        # DSL (Domain Specific Language), which is documented separately
        # in the file "qutest_dsl.py".
        #
        self._dsl_dict  = {
            "required": self.required,
            "include": self.include,
            "test": self.test,
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

            "SCENARIO": self.SCENARIO, # for BDD (alternative 1)
            "GIVEN": self.GIVEN,       # for BDD (alternative 1)
            "WHEN": self.WHEN,         # for BDD (alternative 1)
            "THEN": self.THEN,         # for BDD (alternative 1)
            "AND": self.AND,           # for BDD (alternative 1)

            "scenario": scenario,      # for BDD (alternative 2)
            "given": given,            # for BDD (alternative 2)
            "when": when,              # for BDD (alternative 2)
            "then": then,              # for BDD (alternative 2)

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
            "OBJ_EP": QSpy._OBJ_EP,
            "OBJ_SM_AO": QSpy._OBJ_SM_AO,
            "GRP_ALL": QSpy.GRP_ALL,
            "GRP_SM": QSpy.GRP_SM,
            "GRP_AO": QSpy.GRP_AO,
            "GRP_EQ": QSpy.GRP_EQ,
            "GRP_MP": QSpy.GRP_MP,
            "GRP_TE": QSpy.GRP_TE,
            "GRP_QF": QSpy.GRP_QF,
            "GRP_SC": QSpy.GRP_SC,
            "GRP_SEM": QSpy.GRP_SEM,
            "GRP_MTX": QSpy.GRP_MTX,
            "GRP_U0": QSpy.GRP_U0,
            "GRP_U1": QSpy.GRP_U1,
            "GRP_U2": QSpy.GRP_U2,
            "GRP_U3": QSpy.GRP_U3,
            "GRP_U4": QSpy.GRP_U4,
            "GRP_UA": QSpy.GRP_UA,
            "GRP_OFF": -QSpy.GRP_ALL,
            "GRP_ON": QSpy.GRP_ALL,
            "QS_USER": QSpy._QS_USER,
            "IDS_ALL": QSpy.IDS_ALL,
            "IDS_AO": QSpy.IDS_AO,
            "IDS_EP": QSpy.IDS_EP,
            "IDS_EQ": QSpy.IDS_EQ,
            "IDS_AP": QSpy.IDS_AP
        }

    def __del__(self):
        QUTest.trace("~QUTest", self)

    def exec_dsl(self, code):
        # pylint: disable=exec-used
        exec(code, self._dsl_dict)

    @staticmethod
    def trace(*args, **kwargs):
        if QUTest._opt_trace:
            print(*args, **kwargs)

    #-------------------------------------------------------------------------
    # QUTest Domain Specific Language (DSL) commands

    # test DSL command .......................................................
    def test(self, title, opt=0):
        # end the previous test
        self._test_end()

        # start the new test...
        QUTest._test_start = QUTest._time()
        QUTest._test_num += 1

        if self._is_inter:
            QUTest._num_skipped += 1
            QUTest._str_skipped += f" {QUTest._test_num}"

        reset_char = ('-', '^')[opt & QUTest._OPT_NORESET != 0]
        if self._to_skip == 0:
            title_marker = f"[{QUTest._test_num:2d}]{reset_char}"\
                      "--------------------------------------"\
                      "-----------------------------------\n" + title
            QUTest.display(title_marker)
            QSpy.qspy_show(title_marker)
        else:
            self._to_skip -= 1
            QUTest._num_skipped += 1
            QUTest._str_skipped += f" {QUTest._test_num}"
            title_marker = f"[{QUTest._test_num:2d}]{reset_char}"\
                      " - - - - - - - - - - - - - - - - - - -"\
                      "- - - - - - - - - - - - - - - - - -\n" + title
            QUTest.display(title_marker)
            QSpy.qspy_show(title_marker)
            QUTest.display("                                             "
                "                      [ SKIPPED ]")
            QSpy.qspy_show("                                             "
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

    # SCENARIO DSL command .......................................................
    def SCENARIO(self, title="", opt=0):
        self._context = Context_() # reset the context
        self.test(f"SCENARIO: {title}", opt)

    # expect DSL command .....................................................
    def expect(self, exp, ignore=False):
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("expect")
        elif self._state == QUTest._TEST:

            if not QSpy.receive(): # timeout?
                self._fail('got: "" (timeout)',
                          f'exp: "{exp}"')
                return False

            # NOTE: before the call to fnmatchcase(), replace
            # the special characters "[" and "]" with non-printable,
            # but unique counterparts"\2" and "\3", respectively.
            # This is to avoid the unwanted special treatment of "["/"]"
            # in fnmatchcase().
            if exp.startswith("@timestamp"):
                self._timestamp += 1
                exp = f"{self._timestamp:010d}{exp[10:]}"
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

            if not fnmatchcase(names, pattern):
                self._fail(f'got: "{QUTest._last_record}"',
                           f'exp: "{exp}"')
                return False

            # is interactive?
            if self._is_inter and ignore:
                # consume & ignore all output produced
                while QSpy.receive():
                    print(QUTest._last_record)

            return True

        elif self._state in (QUTest._FAIL, QUTest._SKIP):
            pass # ignore
        else:
            assert 0, "invalid state in expect: {exp}"
        return False

    # ensure DSL command .....................................................
    def ensure(self, bool_expr):
        if not bool_expr:
            #code_context = getframeinfo(stack()[1][0]).code_context
            #self._fail(''.join(code_context).strip())
            self._fail('ensure')
            return False
        return True

    # glb_filter DSL command .................................................
    def glb_filter(self, *args):
        # internal helper function
        def _apply(bits, mask, is_neg):
            if is_neg:
                return bits & ~mask
            return bits | mask

        bitmask = 0 # 128-bit integer bitmask
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("glb_bitmask")
        elif self._state == QUTest._TEST:
            for arg in args:
                # NOTE: positive filter argument means 'add' (allow),
                # negative filter argument means 'remove' (disallow)
                is_neg = False
                if isinstance(arg, str):
                    is_neg = (arg[0] == '-') # is  request?
                    if is_neg:
                        arg = arg[1:]
                    try:
                        arg = QSpy.QS_PRE.index(arg)
                    except Exception:
                        assert 0, f'invalid global filter arg="{arg}"'
                else:
                    is_neg = (arg < 0)
                    if is_neg:
                        arg = -arg

                if arg < 0x7F:
                    bitmask = _apply(bitmask, 1 << arg, is_neg)
                elif arg == QSpy.GRP_ALL:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_ALL, is_neg)
                elif arg == QSpy.GRP_SM:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_SM, is_neg)
                elif arg == QSpy.GRP_AO:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_AO, is_neg)
                elif arg == QSpy.GRP_QF:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_QF, is_neg)
                elif arg == QSpy.GRP_TE:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_TE, is_neg)
                elif arg == QSpy.GRP_EQ:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_EQ, is_neg)
                elif arg == QSpy.GRP_MP:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_MP, is_neg)
                elif arg == QSpy.GRP_SC:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_SC, is_neg)
                elif arg == QSpy.GRP_SEM:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_SEM, is_neg)
                elif arg == QSpy.GRP_MTX:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_MTX, is_neg)
                elif arg == QSpy.GRP_U0:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_U0, is_neg)
                elif arg == QSpy.GRP_U1:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_U1, is_neg)
                elif arg == QSpy.GRP_U2:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_U2, is_neg)
                elif arg == QSpy.GRP_U3:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_U3, is_neg)
                elif arg == QSpy.GRP_U4:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_U4, is_neg)
                elif arg == QSpy.GRP_UA:
                    bitmask = _apply(bitmask, QSpy.GLB_FLT_MASK_UA, is_neg)
                else:
                    assert 0, f"invalid global filter arg=0x{arg:X}"

            QSpy.send_to(struct.pack("<BBQQ", QSpy.TO_TRG_GLB_FILTER, 16,
                                     bitmask & 0xFFFFFFFFFFFFFFFF,
                                     bitmask >> 64))

            self.expect("           Trg-Ack  QS_RX_GLB_FILTER")

        elif self._state in (QUTest._FAIL, QUTest._SKIP):
            pass # ignore
        else:
            assert 0, "invalid state in glb_filter"
        return bitmask

    # loc_filter DSL command .................................................
    def loc_filter(self, *args):
        # internal helper function
        def _apply(bits, mask, is_neg):
            if is_neg:
                return bits & ~mask
            return bits | mask

        bitmask = 0 # 128-bit integer bitmask
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("loc_filter")
        elif self._state == QUTest._TEST:
            for arg in args:
                # NOTE: positive filter argument means 'add' (allow),
                # negative filter argument means 'remove' (disallow)
                is_neg = (arg < 0)
                if is_neg:
                    arg = -arg

                if arg < 0x7F:
                    bitmask = _apply(bitmask, 1 << arg, is_neg)
                elif arg == QSpy.IDS_ALL:
                    bitmask = _apply(bitmask, QSpy.LOC_FLT_MASK_ALL, is_neg)
                elif arg == QSpy.IDS_AO:
                    bitmask = _apply(bitmask, QSpy.LOC_FLT_MASK_AO, is_neg)
                elif arg == QSpy.IDS_EP:
                    bitmask = _apply(bitmask, QSpy.LOC_FLT_MASK_EP, is_neg)
                elif arg == QSpy.IDS_EQ:
                    bitmask = _apply(bitmask, QSpy.LOC_FLT_MASK_EQ, is_neg)
                elif arg == QSpy.IDS_AP:
                    bitmask = _apply(bitmask, QSpy.LOC_FLT_MASK_AP, is_neg)
                else:
                    assert 0, f"invalid local filter arg=0x{arg:X}"

            QSpy.send_to(struct.pack("<BBQQ", QSpy.TO_TRG_LOC_FILTER, 16,
                                     bitmask & 0xFFFFFFFFFFFFFFFF,
                                     bitmask >> 64))

            self.expect("           Trg-Ack  QS_RX_LOC_FILTER")

        elif self._state in (QUTest._FAIL, QUTest._SKIP):
            pass # ignore
        else:
            assert 0, "invalid state in loc_filter"
        return bitmask

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
                QSpy.send_to(struct.pack(
                    fmt, QSpy.TO_SPY_TRG_AO_FILTER, remove, 0),
                    obj_id) # add string object-ID to end
            else:
                if obj_id < 0:
                    obj_id = -obj_id
                    remove = 1
                QSpy.send_to(struct.pack(
                    fmt, QSpy.TO_TRG_AO_FILTER, remove, obj_id))

            self.expect("           Trg-Ack  QS_RX_AO_FILTER")

        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
                QSpy.send_to(struct.pack(
                    fmt, QSpy.TO_TRG_CURR_OBJ, obj_kind, obj_id))
            else:
                QSpy.send_to(struct.pack(
                    fmt, QSpy.TO_SPY_TRG_CURR_OBJ, obj_kind, 0),
                    obj_id) # add string object ID to end
            self.expect("           Trg-Ack  QS_RX_CURR_OBJ")

        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
            QSpy.send_to(struct.pack("<BB", QSpy.TO_TRG_QUERY_CURR, obj_kind))
            # test-specific expect
            if self._is_inter:
                self.expect("@timestamp *")
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
            QSpy.send_to(struct.pack("<B", QSpy.TO_TRG_CONTINUE))
            self.expect("           Trg-Ack  QS_RX_TEST_CONTINUE")
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
            pass # ignore
        else:
            assert 0, "invalid state in expect_run"

    # command DSL command ....................................................
    def command(self, cmd_id, param1 = 0, param2 = 0, param3 = 0):
        if self._to_skip > 0:
            pass # ignore
        if self._state == QUTest._INIT:
            self._before_test("command")
        elif self._state == QUTest._TEST:
            fmt = "<BBIII"
            if isinstance(cmd_id, int):
                QSpy.send_to(struct.pack(fmt, QSpy.TO_TRG_COMMAND,
                                     cmd_id, param1, param2, param3))
            else:
                QSpy.send_to(struct.pack(
                    fmt, QSpy.TO_SPY_TRG_COMMAND, 0, param1, param2, param3),
                    cmd_id) # add string command ID to end
            self.expect("           Trg-Ack  QS_RX_COMMAND", True)
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
            QSpy.send_evt(QSpy.EVT_INIT, signal, params)
            self.expect("           Trg-Ack  QS_RX_EVENT", True)
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
            QSpy.send_evt(QSpy.EVT_DISPATCH, signal, params)
            self.expect("           Trg-Ack  QS_RX_EVENT", True)
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
            QSpy.send_evt(QSpy.EVT_POST, signal, params)
            self.expect("           Trg-Ack  QS_RX_EVENT", True)
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
            QSpy.send_evt(QSpy.EVT_PUBLISH, signal, params)
            self.expect("           Trg-Ack  QS_RX_EVENT", True)
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
                QSpy.send_to(struct.pack(
                    fmt, QSpy.TO_TRG_TEST_PROBE, data, func))
            else:
                # Send to QSpy to provide "func" from Fun Dictionary
                QSpy.send_to(struct.pack(
                    fmt, QSpy.TO_SPY_TRG_TEST_PROBE, data, 0),
                    func) # add string func name to end
            self.expect("           Trg-Ack  QS_RX_TEST_PROBE")
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
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
            QSpy.send_to(struct.pack("<BB", QSpy.TO_TRG_TICK, tick_rate))
            self.expect("           Trg-Ack  QS_RX_TICK", True)
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
            pass # ignore
        else:
            assert 0, "invalid state in tick"

    # peek DSL command .......................................................
    def peek(self, offset, size, num):
        assert size in (1, 2, 4), "Size must be 1, 2, or 4"
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("peek")
        elif self._state == QUTest._TEST:
            QSpy.send_to(struct.pack("<BHBB", QSpy.TO_TRG_PEEK,
                offset, size, num))
            # explicit expectation of peek output
            if self._is_inter:
                self.expect("@timestamp *")
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
            pass # ignore
        else:
            assert 0, "invalid state in peek"

    # poke DSL command .......................................................
    def poke(self, offset, size, data):
        assert size in (1, 2, 4), "Size must be 1, 2, or 4"
        if self._to_skip > 0:
            pass # ignore
        elif self._state == QUTest._INIT:
            self._before_test("poke")
        elif self._state == QUTest._TEST:
            length = len(data)
            num = length // size
            QSpy.send_to(struct.pack("<BHBB", QSpy.TO_TRG_POKE,
                         offset, size, num) + data)
            self.expect("           Trg-Ack  QS_RX_POKE")
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
            pass # ignore
        else:
            assert 0, "invalid state in poke"

    # fill DSL command .......................................................
    def fill(self, offset, size, num, item = 0):
        assert size in (1, 2, 4), "Size must be 1, 2, or 4"
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
            packet = struct.pack(fmt, QSpy.TO_TRG_FILL,
                                 offset, size, num, item)
            QSpy.send_to(packet)
            self.expect("           Trg-Ack  QS_RX_FILL")
        elif self._state in (QUTest._FAIL, QUTest._SKIP):
            pass # ignore
        else:
            assert 0, "invalid state in fill"

    # skip DSL command .......................................................
    def skip(self, n_tests = 9999):
        if self._to_skip == 0: # not skipping already?
            self._to_skip = n_tests

    # required DSL command ....................................................
    def required(self, cond, msg = ""):
        if not cond:
            self.note(msg)
            QUTest_inst._fail()
            raise ExitOnFailException

    # include DSL command ....................................................
    def include(self, fname):
        path = os.path.normcase(os.path.join(self._test_dname, fname))
        if not os.path.exists(path):
            raise FileNotFoundError(path)
        with open(path, encoding="utf-8") as inc_file:
            code = compile(inc_file.read(), path, "exec")
            # execute the include script code in this instance of QUTest
            self.exec_dsl(code)

    # test file/dir read-only accessors ......................................
    def test_file(self):
        return self._test_fname

    def test_dir(self):
        return self._test_dname

    # last_rec DSL command ...................................................
    def last_rec(self):
        return QUTest._last_record

    # note DSL command .....................................................
    def note(self, msg, dest=0x3):
        if self._to_skip > 0:
            return
        if (dest & QUTest._OPT_SCREEN) != 0:
            QUTest.display(msg)
        if (dest & QUTest._OPT_TRACE) != 0:
            QSpy.qspy_show(msg, kind=0x0)

    # GIVEN DSL command .....................................................
    def GIVEN(self, msg="", dest=0x3):
        self.note(f"\n   GIVEN: {msg}")

    # WHEN DSL command .....................................................
    def WHEN(self, msg="", dest=0x3):
        self.note(f"    WHEN: {msg}")

    # THEN DSL command .....................................................
    def THEN(self, msg="", dest=0x3):
        self.note(f"    THEN: {msg}")

    # AND DSL command .....................................................
    def AND(self, msg="", dest=0x3):
        self.note(f"     AND: {msg}")

    # dummy callbacks --------------------------------------------------------
    def _dummy_on_reset(self):
        QUTest.trace("_dummy_on_reset")
        self.expect("           QF_RUN")

    def _dummy_on_setup(self):
        QUTest.trace("_dummy_on_setup")
        pass

    def _dummy_on_teardown(self):
        QUTest.trace("_dummy_on_teardown")
        pass

    # helper methods ---------------------------------------------------------
    @staticmethod
    def _run_script(fname):
        # pylint: disable=invalid-name
        QUTest._num_groups += 1

        with open(fname, encoding="utf-8") as script_file:
            # pylint: disable=protected-access
            global QUTest_inst
            QUTest_inst = QUTest()
            QUTest_inst._test_fname = fname
            QUTest_inst._test_dname = os.path.dirname(fname)

            QUTest.display("\n=================================="\
                            f"[Group {QUTest._num_groups:2d}]"\
                            "==================================")
            QUTest.display(fname, QUTest._COL_GRP1, QUTest._COL_GRP2)
            QSpy.qspy_show("\n=================================="\
                           f"[Group {QUTest._num_groups:2d}]"\
                           "==================================\n")

            if not QUTest_inst._test_dname: # empty dir?
                QUTest_inst._test_dname = "."
            try:
                QUTest._test_start = QUTest._time()
                code = compile(script_file.read(), fname, "exec")

                # the last test ended with assertion?
                if QUTest._have_assert:
                    QUTest._have_assert = False
                    if not QUTest._host_exe[0]:
                        # ignore all input until timeout
                        while QSpy.receive():
                            pass

                # execute the script code in a *separate instance* of QUTest
                QUTest_inst.exec_dsl(code)

            except ExitOnFailException:
                # QUTest_inst._fail() already done
                pass

            except RuntimeError as err:
                QUTest.display(str(err), QUTest._COL_ERR1, QUTest._COL_ERR2)
                QSpy.qspy_show(str(err))
                QUTest_inst._fail()

            except SyntaxError:
                QUTest.display(traceback.format_exc(limit=0),
                    QUTest._COL_ERR1, QUTest._COL_ERR2)
                QSpy.qspy_show(traceback.format_exc(limit=0))
                QUTest._num_failed += 1
                QUTest._str_failed += f" {QUTest._num_groups}"

            except ExitOnDetachException:
                QUTest.display("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ "\
                     "QSPY detached ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~",
                     QUTest._COL_ERR1, QUTest._COL_ERR2)
                sys.exit(-1)

            except Exception:
                QUTest.display(traceback.format_exc(),
                    QUTest._COL_ERR1, QUTest._COL_ERR2)
                QUTest_inst._fail()

            if QUTest._opt_interactive:
                QUTest_inst._interact()

            # properly end the last test in the group
            QUTest_inst._test_end()
            QUTest._quithost_exe(9)

    def _interact(self):
        self._is_inter   = True
        self._test_fname = ''
        self._test_dname = os.path.dirname('.')

        if self._state == QUTest._TEST:
            QUTest._num_skipped += 1
            QUTest._str_skipped += f" {QUTest._test_num}"

        # consume & ignore all output produced so far
        while QSpy.receive():
            print(QUTest._last_record)

        # enter "interactive mode": commands entered as user input
        while True:
            code = input('>>> ')
            if code:
                try:
                    self.exec_dsl(code)
                except Exception:
                    traceback.print_exc(2)
            else:
                break

    def _tran(self, state):
        QUTest.trace(f"tran({self._state}->{state})")
        self._state = state

    def _test_end(self):
        if self._state != QUTest._TEST:
            return

        if not QUTest._have_info:
            return

        if self._is_inter:
            QUTest.display("                                             "
                "                      [ SKIPPED ]")
            QSpy.qspy_show("                                             "
                "                      [ SKIPPED ]")
            return

        elapsed = QUTest._time() - QUTest._test_start
        if QUTest._have_assert:
            QUTest.display("                                             "\
                f"                [ PASS ({elapsed:5.1f}s) ]",
                QUTest._COL_PASS1, QUTest._COL_PASS2)
            QSpy.qspy_show(f"[{QUTest._test_num:2d}]------------------------"\
                f"---------------------------------[ PASS ({elapsed:5.1f}s) ]")
            return

        QSpy.send_to(struct.pack("<B", QSpy.TO_TRG_TEST_TEARDOWN))
        if not QSpy.receive(): # timeout?
            self._fail('got: "" (timeout)',
                       'exp: end-of-test')
            return

        if QUTest._last_record != "           Trg-Ack  QS_RX_TEST_TEARDOWN":
            self._fail(f'got: "{QUTest._last_record}"',
                        'exp: end-of-test')
            # ignore all input until timeout
            while QSpy.receive():
                pass
            return

        self._dsl_dict["on_teardown"]() # on_teardown() callback

        QUTest.display("                                             "\
                        f"                [ PASS ({elapsed:5.1f}s) ]",
                        QUTest._COL_PASS1, QUTest._COL_PASS2)
        QSpy.qspy_show(f"[{QUTest._test_num:2d}]------------------------"\
           f"---------------------------------[ PASS ({elapsed:5.1f}s) ]")


    def _reset_target(self):
        if QUTest._host_exe[0]:
            if not QUTest._have_assert:
                QUTest._quithost_exe(1)

            # lauch a new instance of the host executable
            QUTest._have_target = True
            QUTest._have_info = False
            # pylint: disable=consider-using-with
            Popen(QUTest._host_exe)

        else: # running remote target
            QUTest._have_target = True
            QUTest._have_info = False
            if not QUTest._is_debug and not QUTest._have_assert:
                QSpy.send_to(struct.pack("<B", QSpy.TO_TRG_RESET))

        # ignore all input until have-target-info or timeout
        while QSpy.receive():
            if QUTest._have_info:
                break

        if QUTest._have_info:
            QUTest._have_assert = False
            QUTest._need_reset  = False
        else:
            QUTest._quithost_exe(2)
            raise RuntimeError("Target reset failed")

        self._timestamp = 0
        self._dsl_dict["on_reset"]()
        return self._state == QUTest._TEST

    def _on_setup(self):
        assert self._state == QUTest._TEST, \
            f"on_setup() outside the TEST state {self._state}"
        if not QUTest._have_info:
            return False

        QSpy.send_to(struct.pack("<B", QSpy.TO_TRG_TEST_SETUP))
        self.expect("           Trg-Ack  QS_RX_TEST_SETUP")
        if self._state == QUTest._TEST:
            self._timestamp = 0
            self._dsl_dict["on_setup"]()
        return True

    def _before_test(self, command):
        QUTest._num_failed += 1
        self._tran(QUTest._FAIL)
        msg = f'"{command}" before any test'
        raise SyntaxError(msg)

    def _fail(self, err = "", exp = ""):
        stk = stack()
        max_lvl = len(stk) - 3
        lvl = 2
        while lvl < max_lvl:
            fun = getframeinfo(stk[lvl][0]).function
            if fun == "<module>":
                QUTest.display(f"  @{getframeinfo(stk[lvl][0]).filename}"\
                                f":{getframeinfo(stk[lvl][0]).lineno}")
            else:
                QUTest.display(f"  @{getframeinfo(stk[lvl][0]).filename}/"\
                                f"{getframeinfo(stk[lvl][0]).function}:"\
                                f"{getframeinfo(stk[lvl][0]).lineno}")
            lvl += 1
        if exp != "":
            QUTest.display(exp, QUTest._COL_EXP1, QUTest._COL_EXP2)
        if err == "ensure":
            if 2 < max_lvl:
                err = ''.join(getframeinfo(stk[2][0]).code_context).strip()
            QUTest.display(err, QUTest._COL_ERR1, QUTest._COL_ERR2)
        elif err != "":
            QUTest.display(err, QUTest._COL_ERR1, QUTest._COL_ERR2)

        if self._is_inter:
            return

        elapsed = QUTest._time() - QUTest._test_start
        QUTest.display("! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! "\
            f"! ! ! ! ! ! ! ! ![ FAIL ({elapsed:5.1f}s) ]",
            QUTest._COL_FAIL1, QUTest._COL_FAIL2)
        QSpy.qspy_show(f"[{QUTest._test_num:2d}]! ! ! ! ! ! ! ! ! ! ! "\
            f"! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ![ FAIL ({elapsed:5.1f}s) ]")
        QUTest._num_failed += 1
        QUTest._str_failed += f" {QUTest._test_num}"
        QUTest._need_reset = True
        self._tran(QUTest._FAIL)
        if QUTest._opt_exit_on_fail:
            raise ExitOnFailException
        # ignore all input until timeout
        while QSpy.receive():
            pass

    @staticmethod
    def _quithost_exe(source):
        QUTest.trace(f"attempt to quit host exe from={source}")
        if QUTest._host_exe[0] and QUTest._have_target:
            QUTest.trace("quitting host exe...")
            QUTest._have_target = False
            QSpy.send_to(struct.pack("<B", QSpy.TO_TRG_RESET))
            time.sleep(0.2 * QUTest.TIMEOUT) # wait until host-exe quits

    @staticmethod
    def _time():
        return time.perf_counter()

    @staticmethod
    def display(msg, color1='', color2='', eol=True):
        if QUTest._log_file:
            QUTest._log_file.write(msg)
            if eol:
                QUTest._log_file.write('\n')
        if color1 != '':
            msg = color1 + msg
        if color2 != '':
            msg = msg + color2
        if eol:
            print(msg)
        else:
            print(msg, end='')

#=============================================================================
# BDD support...

class Context_:
    pass

class scenario:
    def __init__(self, title, opt=0):
        self.title = title
        self.opt = opt

    def __call__(self, action):
        def wrapper():
            pass

        QUTest_inst.test(f"SCENARIO: {self.title}", self.opt)
        QUTest_inst._context = action(Context_()) # clear the context!
        return wrapper

class given:
    def __init__(self, *msgs):
        self.msgs = msgs

    def __call__(self, action):
        def wrapper():
            pass

        global QUTest_inst
        if QUTest_inst._state == QUTest._SKIP:
            return wrapper

        if len(self.msgs) > 0:
            QUTest_inst.note(f"\n   GIVEN: {self.msgs[0]}")
            for msg in self.msgs[1:]:
                QUTest_inst.note(f"     AND: {msg}")
        else:
            QUTest_inst.note("\n   GIVEN:")
        QUTest_inst._context = action(QUTest_inst._context)
        return wrapper

class when:
    def __init__(self, *msgs):
        self.msgs = msgs

    def __call__(self, action):
        def wrapper():
            pass

        global QUTest_inst
        if QUTest_inst._state == QUTest._SKIP:
            return wrapper

        if len(self.msgs) > 0:
            QUTest_inst.note(f"    WHEN: {self.msgs[0]}")
            for msg in self.msgs[1:]:
                QUTest_inst.note(f"     AND: {msg}")
        else:
            QUTest_inst.note("    WHEN:")
        action(QUTest_inst._context)
        return wrapper

class then:
    def __init__(self, *msgs):
        self.msgs = msgs

    def __call__(self, action):
        def wrapper():
            pass

        global QUTest_inst
        if QUTest_inst._state == QUTest._SKIP:
            return wrapper

        if len(self.msgs) > 0:
            QUTest_inst.note(f"    THEN: {self.msgs[0]}")
            for msg in self.msgs[1:]:
                QUTest_inst.note(f"     AND: {msg}")
        else:
            QUTest_inst.note("    THEN:")
        action(QUTest_inst._context)
        return wrapper

#=============================================================================
class ExitOnFailException(Exception):
    # test failed and exit-on-fail (-x) is set
    pass

class ExitOnDetachException(Exception):
    # QSPY detached in the middle of the run
    pass

#=============================================================================
# Helper class for communication with the QSpy front-end
#
class QSpy:
    # private class variables...
    _sock = None
    _is_attached = False
    _tx_seq = 0
    host_udp = ["localhost", 7701] # list to be converted to a tuple
    _local_port = 0 # let the OS decide the best local port

    # formats of various packet elements from the Target
    trg_QPver    = 0
    trg_QPdate   = 0
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

    # records directly to the Target...
    TO_TRG_INFO       = 0
    TO_TRG_COMMAND    = 1
    TO_TRG_RESET      = 2
    TO_TRG_TICK       = 3
    TO_TRG_PEEK       = 4
    TO_TRG_POKE       = 5
    TO_TRG_FILL       = 6
    TO_TRG_TEST_SETUP = 7
    TO_TRG_TEST_TEARDOWN = 8
    TO_TRG_TEST_PROBE = 9
    TO_TRG_GLB_FILTER = 10
    TO_TRG_LOC_FILTER = 11
    TO_TRG_AO_FILTER  = 12
    TO_TRG_CURR_OBJ   = 13
    TO_TRG_CONTINUE   = 14
    TO_TRG_QUERY_CURR = 15
    TO_TRG_EVENT      = 16

    # packets to QSpy to be "massaged" and forwarded to the Target...
    TO_SPY_TRG_EVENT      = 135
    TO_SPY_TRG_AO_FILTER  = 136
    TO_SPY_TRG_CURR_OBJ   = 137
    TO_SPY_TRG_COMMAND    = 138
    TO_SPY_TRG_TEST_PROBE = 139

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

    # gloal filter groups by topic...
    GRP_ALL= 0xF0
    GRP_SM = 0xF1
    GRP_AO = 0xF2
    GRP_MP = 0xF3
    GRP_EQ = 0xF4
    GRP_TE = 0xF5
    GRP_QF = 0xF6
    GRP_SC = 0xF7
    GRP_SEM= 0xF8
    GRP_MTX= 0xF9
    GRP_U0 = 0xFA
    GRP_U1 = 0xFB
    GRP_U2 = 0xFC
    GRP_U3 = 0xFD
    GRP_U4 = 0xFE
    GRP_UA = 0xFF

    _QS_USER = 100

    # local filter groups...
    IDS_ALL= 0xF0
    IDS_AO = (0x80 + 0)
    IDS_EP = (0x80 + 64)
    IDS_EQ = (0x80 + 80)
    IDS_AP = (0x80 + 96)

    # kinds of objects (local-filter and curr-obj)...
    _OBJ_SM = 0    # State Machine
    _OBJ_AO = 1    # Active Object
    _OBJ_MP = 2    # Memory Pool
    _OBJ_EQ = 3    # Event Queue
    _OBJ_TE = 4    # Time Event
    _OBJ_AP = 5    # Application
    _OBJ_SM_AO = 6 # State Machine & Active Object
    _OBJ_EP = 7    # Event Pool

    # event processing commands for QS-RX
    EVT_PUBLISH   = 0
    EVT_POST      = 253
    EVT_INIT      = 254
    EVT_DISPATCH  = 255

    # tuple of QS predefined record names from the Target.
    # !!! NOTE: Must match qpc/include/qs.h !!!
    QS_PRE = ("QS_EMPTY",
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
                                  "QS_QF_ACTIVE_DEFER_ATTEMPT",
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
    GLB_FLT_MASK_ALL= 0x1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
    GLB_FLT_MASK_SM = 0x000000000000000003800000000003FE
    GLB_FLT_MASK_AO = 0x0000000000020000000020000007FC00
    GLB_FLT_MASK_QF = 0x000000000000000000001FC0FC800000
    GLB_FLT_MASK_TE = 0x00000000000000000000003F00000000
    GLB_FLT_MASK_EQ = 0x00000000000000000000400000780000
    GLB_FLT_MASK_MP = 0x00000000000000000000800003000000
    GLB_FLT_MASK_SC = 0x0000000000000000003F000000000000
    GLB_FLT_MASK_SEM= 0x00000000000007800000000000000000
    GLB_FLT_MASK_MTX= 0x000000000001F8000000000000000000
    GLB_FLT_MASK_U0 = 0x000001F0000000000000000000000000
    GLB_FLT_MASK_U1 = 0x00003E00000000000000000000000000
    GLB_FLT_MASK_U2 = 0x0007C000000000000000000000000000
    GLB_FLT_MASK_U3 = 0x00F80000000000000000000000000000
    GLB_FLT_MASK_U4 = 0x1F000000000000000000000000000000
    GLB_FLT_MASK_UA = 0x1FFFFFF0000000000000000000000000

    # local filter masks
    LOC_FLT_MASK_ALL= 0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
    LOC_FLT_MASK_AO = 0x0000000000000001FFFFFFFFFFFFFFFE
    LOC_FLT_MASK_EP = 0x000000000000FFFE0000000000000000
    LOC_FLT_MASK_EQ = 0x00000000FFFF00000000000000000000
    LOC_FLT_MASK_AP = 0xFFFFFFFF000000000000000000000000

    @staticmethod
    def _init():
        # Create socket
        QSpy._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        QSpy._sock.settimeout(QUTest.TIMEOUT) # timeout for blocking socket
        try:
            QSpy._sock.bind(("0.0.0.0", QSpy._local_port))
            QUTest.trace(f'bind: ("0.0.0.0", {QSpy._local_port})')
        except Exception:
            print("UDP Socket Error"\
                  "Can't bind the UDP socket\nto the specified local_host")
            sys.exit(-1)
        return 0

    @staticmethod
    def _attach(channels = 0x2):
        # channels: 1-binary, 2-text, 3-both
        print(f"Attaching to QSpy "\
              f"({QSpy.host_udp[0]}:{QSpy.host_udp[1]})... ", end='')
        QSpy._is_attached = False
        # cannot use QSpy.send_to() because the socket is not attached yet
        tx_packet = bytearray([QSpy._tx_seq])
        tx_packet.extend(struct.pack("<BB", QSpy._QSPY_ATTACH, channels))
        QSpy._sock.sendto(tx_packet, QSpy.host_udp)
        QSpy._tx_seq = (QSpy._tx_seq + 1) & 0xFF
        try:
            QSpy.receive()
        except Exception:
            pass

        if QSpy._is_attached:
            print("OK\n")
            return True
        print("FAILED\n")
        return False

    @staticmethod
    def _detach():
        if QSpy._sock is None:
            return
        QSpy.send_to(struct.pack("<B", QSpy._QSPY_DETACH))
        time.sleep(QUTest.TIMEOUT)
        #QSpy._sock.shutdown(socket.SHUT_RDWR)
        QSpy._sock.close()
        QSpy._sock = None
        QSpy._is_attached = False

    # returns True if packet received, False if timed out
    @staticmethod
    def receive():
        # pylint: disable=protected-access
        if not QUTest._is_debug:
            try:
                packet = QSpy._sock.recv(4096)
            except socket.timeout:
                QUTest._last_record = ""
                return False # timeout
            # don"t catch OSError
        else: # debug mode
            while True:
                try:
                    packet = QSpy._sock.recv(4096)
                    break
                except socket.timeout:
                    print("waiting for Target output "\
                        "(press Enter to quit this test)...")
                    if os.name == "nt":
                        if msvcrt.kbhit():
                            if msvcrt.getch() == b'\r':
                                print("quit")
                                return False # timeout
                    else:
                        ready,_,_ = select.select([sys.stdin], [], [], 0)
                        if ready != []:
                            sys.stdin.readline() # consume the Return key
                            print("quit")
                            return False # timeout
                # don"t catch OSError

        dlen = len(packet)
        if dlen < 2:
            QUTest._last_record = ""
            raise RuntimeError("UDP packet from QSpy too short")

        rec_id = packet[1]
        if rec_id == QSpy._PKT_TEXT_ECHO: # text packet (most common)
            QUTest._last_record = packet[3:].decode("utf-8")
            # QS_ASSERTION?
            if dlen > 3 and packet[2] == QSpy._PKT_ASSERTION:
                QUTest._have_assert = True
                QUTest._have_target = False
                QUTest._need_reset  = True
                if QSpy.trg_QPver < 720: # before QP 7.2.0?
                    QSpy.send_to(struct.pack("<B", QSpy.TO_TRG_RESET))

        elif rec_id == QSpy._PKT_TARGET_INFO: # target info?
            QUTest._last_record = ""
            if dlen == 18:
                raise RuntimeError(
                    "QP Version Error, QP 8.0.0 or newer required")
            if dlen != 20:
                raise RuntimeError("Corrupted Target info")

            qp_release = ~(packet[3] | (packet[4] << 8) \
                           | (packet[5] << 16) | (packet[6] << 24)) \
                           & 0xFFFFFFFF
            QSpy.trg_QPver = qp_release % 10000
            QSpy.trg_QPdate = qp_release // 10000
            fmt = "xBHxLxxxQ"
            trg_info = packet[7:20]
            QSpy.trg_objPtr  = fmt[trg_info[3] & 0x0F]
            QSpy.trg_funPtr  = fmt[trg_info[3] >> 4]
            QSpy.trg_tsize   = fmt[trg_info[4] & 0x0F]
            QSpy.trg_sig     = fmt[trg_info[0] & 0x0F]
            QSpy.trg_evtSize = fmt[trg_info[0] >> 4]
            QSpy.trg_queueCtr= fmt[trg_info[1] & 0x0F]
            QSpy.trg_poolCtr = fmt[trg_info[2] >> 4]
            QSpy.trg_poolBlk = fmt[trg_info[2] & 0x0F]
            QSpy.trg_tevtCtr = fmt[trg_info[1] >> 4]
            QSpy.trg_tstamp  = \
                 f"{trg_info[12]:02d}{trg_info[11]:02d}{trg_info[10]:02d}_"\
                 f"{trg_info[9]:02d}{trg_info[8]:02d}{trg_info[7]:02d}"
            QUTest.trace("Target:", QSpy.trg_tstamp)
            QUTest._have_info = True

        elif rec_id == QSpy._PKT_ATTACH_CONF:
            QUTest._last_record = ""
            QSpy._is_attached = True

        elif rec_id == QSpy._PKT_DETACH:
            QUTest._quithost_exe(0)
            QUTest._last_record = ""
            QSpy._detach()
            raise ExitOnDetachException()

        else:
            QUTest._last_record = ""
            raise RuntimeError("Unrecognized UDP packet type from QSpy")

        return True # some input received

    @staticmethod
    def send_to(packet, payload=None):
        if not QSpy._is_attached:
            return
        tx_packet = bytearray([QSpy._tx_seq])
        tx_packet.extend(packet)
        if payload is not None:
            tx_packet.extend(bytes(payload, "utf-8"))
            tx_packet.extend(b"\0") # zero-terminate
        QSpy._sock.sendto(tx_packet, QSpy.host_udp)
        QSpy._tx_seq = (QSpy._tx_seq + 1) & 0xFF
        QUTest.trace("sendTo", QSpy._tx_seq)

    @staticmethod
    def send_evt(ao_prio, signal, parameters = None):
        fmt = "<BB" + QSpy.trg_sig + "H"
        if parameters is not None:
            length = len(parameters)
        else:
            length = 0

        if isinstance(signal, int):
            packet = bytearray(struct.pack(
                fmt, QSpy.TO_TRG_EVENT, ao_prio, signal, length))
            if parameters is not None:
                packet.extend(parameters)
            QSpy.send_to(packet)
        else:
            packet = bytearray(struct.pack(
                fmt, QSpy.TO_SPY_TRG_EVENT, ao_prio, 0, length))
            if parameters is not None:
                packet.extend(parameters)
            QSpy.send_to(packet, signal)

    @staticmethod
    def qspy_show(note, kind=0xFF):
        QSpy.send_to(struct.pack("<BB", QSpy._QSPY_SHOW_NOTE, kind), note)

#=============================================================================
# main entry point to QUTest
def main():
    # pylint: disable=protected-access

    run_start = QUTest._time()
    run_id = datetime.now().strftime("%y%m%d_%H%M%S")

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
        version=f"QUTest script runner {QUTest.VERSION//100}."\
                f"{(QUTest.VERSION//10) % 10}.{QUTest.VERSION % 10} "\
                f"on Python {python_version()}",
        help='Display QUTest version')

    parser.add_argument('-e', '--exe', nargs='?', default='', const='',
        help="Optional host executable or debug/DEBUG")
    parser.add_argument('-q', '--qspy', nargs='?', default='', const='',
        help="optional qspy host, [:ud_port][:tcp_port]")
    parser.add_argument('-l', '--log', nargs='?', default='', const='',
        help="Optional log directory (might not exist yet)")
    parser.add_argument('-o', '--opt', nargs='?', default='', const='',
        help="txciob: t:trace,x:exit-on-fail,i:inter,\n"
             "c:qspy-clear,o:qspy-save-txt,b:qspy-save-bin")
    parser.add_argument('scripts', nargs='*',
                        help="List (comma-separated) of test scripts to run")
    args = parser.parse_args()
    QUTest.trace(args)

    # process command-line argumens...
    if args.exe != '':
        if args.exe in ('debug', 'DEBUG'):
            QUTest._is_debug = True
        else:
            host_exe = glob(args.exe)
            if host_exe:
                QUTest._host_exe = [host_exe[0], "localhost:6601"]
            else:
                print(f"\nProvided test executable '{args.exe}' not found\n")
                return sys.exit(-1)

    if args.qspy != '':
        qspy_conf = args.qspy.split(":")
        if len(qspy_conf) > 0 and not qspy_conf[0] == '':
            QSpy.host_udp[0] = qspy_conf[0]
        if len(qspy_conf) > 1 and not qspy_conf[1] == '':
            QSpy.host_udp[1] = int(qspy_conf[1])
        if len(qspy_conf) > 2 and not qspy_conf[2] == '':
            if QUTest._host_exe[0]:
                QUTest._host_exe[1] = f"{QSpy.host_udp[0]}:{qspy_conf[2]}"
            else:
                print("\nTCP port specified without host executable\n")
                return sys.exit(-1)

    log = args.log
    if log != '':
        if not (log.endswith('/') or log.endswith('\\')):
            log += '/'
        try:
            os.makedirs(os.path.dirname(log), exist_ok=True)
        except Exception:
            pass
        if os.path.isdir(log): # is arg.log a directory
            log += f"qutest{run_id}.log"
        else: # not a directory
            log = ''
            print("\nWrong directory for log output\n")
            return sys.exit(-1)

    # convert to immutable tuples
    QSpy.host_udp = tuple(QSpy.host_udp)
    if QUTest._host_exe[0]:
        QUTest._host_exe = tuple(QUTest._host_exe)

    QUTest._opt_trace         = 't' in args.opt
    QUTest._opt_exit_on_fail  = 'x' in args.opt
    QUTest._opt_interactive   = 'i' in args.opt
    QUTest._opt_clear_qspy    = 'c' in args.opt
    QUTest._opt_save_qspy_txt = 'o' in args.opt
    QUTest._opt_save_qspy_bin = 'b' in args.opt

    if not args.scripts: # scripts not provided?
        QUTest.trace("applying default *.py")
        args.scripts = ['*.py'] # apply the default "*.py"
    scripts = []
    for script in args.scripts:
        if script.endswith('.exe'):
            print("\nIncorrect test script(s). Old QUTEST command-line?\n")
            parser.print_help()
            return sys.exit(-1)
        scripts.extend(glob(script))
    # still no scripts?
    if (not scripts) and (not QUTest._opt_interactive):
        print("\nFound no test scripts to run")
        return sys.exit(0)

    QUTest.trace("scripts:", scripts)
    QUTest.trace("host_exe:", QUTest._host_exe)
    QUTest.trace("debug:", QUTest._is_debug)
    QUTest.trace("_log_file:", log)
    QUTest.trace("host_udp:", QSpy.host_udp)
    QUTest.trace("opt: t", QUTest._opt_trace)
    QUTest.trace("opt: x", QUTest._opt_exit_on_fail)
    QUTest.trace("opt: i", QUTest._opt_interactive)
    QUTest.trace("opt: c", QUTest._opt_clear_qspy)
    QUTest.trace("opt: o", QUTest._opt_save_qspy_txt)
    QUTest.trace("opt: b", QUTest._opt_save_qspy_bin)
    #return 0

    # init QSpy socket
    err = QSpy._init()
    if err:
        return sys.exit(err)

    # attach the the QSPY Back-End...
    if not QSpy._attach():
        return sys.exit(-1)

    if QUTest._opt_clear_qspy:
        QSpy.send_to(struct.pack("<B", QSpy._QSPY_CLEAR_SCREEN))

    # open the log file only after potential initialization errors
    if log != '': # log file provided?
        try:
            # pylint: disable=consider-using-with
            QUTest._log_file = open(log, 'w', encoding="utf-8")
        except Exception:
            print("Requested log file cannot be opened")
            return sys.exit(-1)

    if QUTest._opt_save_qspy_txt:
        QSpy.send_to(struct.pack("<BB", QSpy._QSPY_TEXT_OUT, 1))
    if QUTest._opt_save_qspy_bin:
        QSpy.send_to(struct.pack("<BB", QSpy._QSPY_BIN_OUT, 1))

    msg = f"Run ID    : {run_id}"
    QUTest.display(msg)
    QSpy.qspy_show(msg)
    if QUTest._host_exe[0]:
        msg = f"Target    : {QUTest._host_exe[0]},{QUTest._host_exe[1]}"
    else:
        msg = "Target    : remote"
    QUTest.display(msg)
    QSpy.qspy_show(msg)

    # run all the test scripts...
    if scripts:
        for scr in scripts:
            QUTest._run_script(scr)
            # errors encountered? and shall we exit on failure?
            if (QUTest._num_failed != 0) and QUTest._opt_exit_on_fail:
                break
    elif QUTest._opt_interactive:
        # pylint: disable=invalid-name
        QUTest_inst = QUTest()
        QUTest_inst._interact()

    # print the SUMMARY of the run...
    elapsed = QUTest._time() - run_start
    msg = "\n==================================[ SUMMARY ]"\
          "=================================\n"
    QUTest.display(msg)
    QSpy.qspy_show(msg)

    if QUTest._have_info:
        msg = f"Target ID : {QSpy.trg_tstamp} (QP-Ver={QSpy.trg_QPver:3d})"
    else:
        msg = "Target ID : N/A"
    QUTest.display(msg)
    QSpy.qspy_show(msg)

    if QUTest._log_file:
        msg = f"Log file  : {log}"
    else:
        msg = "Log file  :"
    QUTest.display(msg)
    QSpy.qspy_show(msg)

    msg = f"Groups    : {QUTest._num_groups}"
    QUTest.display(msg)
    QSpy.qspy_show(msg)

    msg = f"Tests     : {QUTest._test_num}"
    QUTest.display(msg)
    QSpy.qspy_show(msg)

    if QUTest._num_skipped == 0:
        msg = "Skipped   : 0"
    else:
        msg = f"Skipped   : {QUTest._num_skipped} [{QUTest._str_skipped} ]"
    QUTest.display(msg)
    QSpy.qspy_show(msg)

    if QUTest._num_failed == 0:
        msg = "Failed    : 0"
    else:
        msg = f"FAILED    : {QUTest._num_failed} [{QUTest._str_failed} ]"
    QUTest.display(msg)
    QSpy.qspy_show(msg)

    if QUTest._num_failed == 0:
        msg = f"\n==============================[  OK  ({elapsed:5.1f}s) ]"\
                "==============================="
        QUTest.display(msg, QUTest._COL_OK1, QUTest._COL_OK2)
        QSpy.qspy_show(msg)
    else:
        msg = f"\n==============================[ FAIL ({elapsed:5.1f}s) ]"\
                "==============================="
        QUTest.display(msg, QUTest._COL_NOK1, QUTest._COL_NOK2)
        QSpy.qspy_show(msg)

        if QUTest._opt_exit_on_fail:
            msg = "Exiting after first failure (-x)"
            QUTest.display(msg)
            QSpy.qspy_show(msg)

    # cleanup...
    if QUTest._log_file:
        QUTest._log_file.close()

    if QUTest._opt_save_qspy_txt:
        QSpy.send_to(struct.pack("<BB", QSpy._QSPY_TEXT_OUT, 0))
    if QUTest._opt_save_qspy_bin:
        QSpy.send_to(struct.pack("<BB", QSpy._QSPY_BIN_OUT, 0))

    QUTest._quithost_exe(99)
    QSpy._detach()

    return sys.exit(QUTest._num_failed) # report to the caller (e.g., make)

#=============================================================================
if __name__ == "__main__":
    print(f"\nQUTest unit testing front-end "\
        f"{QUTest.VERSION//100}.{(QUTest.VERSION//10) % 10}."\
        f"{QUTest.VERSION % 10} running on Python {python_version()}")
    print("Copyright (c) 2005-2025 Quantum Leaps, www.state-machine.com")
    if sys.version_info >= (3,6):
        main()
    else:
        print("\nERROR: QUTest requires Python 3.6 or newer")
        sys.exit(-1)
