##
# @file
# @ingroup qpspypy
# @brief The QUtest test context and a number of helper methods for PyTest.

## @cond
#-----------------------------------------------------------------------------
# Last updated for version: 2.0.0
# Last updated on: 2018-08-16
#
# Copyright (c) 2018 Lotus Engineering, LLC
# Copyright (c) 2018 Quantum Leaps, LLC
#
# MIT License:
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Contact information:
# https://www.state-machine.com
# mailto:info@state-machine.com
#-----------------------------------------------------------------------------
# @endcond

import os
import sys
import signal
import pytest
import time
from threading import Event
from queue import Queue
from subprocess import Popen
if sys.platform == 'win32':
    from subprocess import CREATE_NEW_CONSOLE

from qspypy.qspy import qspy, QS_CHANNEL, QS_OBJ_KIND, FILTER, PRIO_COMMAND
import qspypy.config as CONFIG


class qutest_context():
    """ This class provides the main pytest based context."""

    def __init__(self):
        self.qspy_process = None
        self.target_process = None
        self.attached_event = Event()
        self.have_target_event = Event()
        self.text_queue = Queue(maxsize=0)
        self.on_reset_callback = None
        self.on_setup_callback = None
        self.on_teardown_callback = None

    def session_setup(self):
        """ Setup that should run on once per session. """

        # Automatically run qspy backend
        if CONFIG.AUTOSTART_QSPY:
            self.start_qspy()
            # qspy will not listend for the attach immediately
            time.sleep(CONFIG.QSPY_ATTACH_TIMEOUT_SEC)

        self.qspy = qspy()

        self.attached_event.clear()
        self.qspy.attach(self, host = CONFIG.QSPY_HOST, port = CONFIG.QSPY_UDP_PORT, local_port = CONFIG.QSPY_LOCAL_UDP_PORT)
        # Wait for attach
        if not self.attached_event.wait(CONFIG.QSPY_ATTACH_TIMEOUT_SEC):
            __tracebackhide__ = True
            pytest.fail(
                "Timeout waiting for Attach to QSpy (is QSpy running and QSPY_COM_PORT correct?)")

    def session_teardown(self):
        """ Teardown that runs at the end of a session. """
        # Stop target executable
        if CONFIG.USE_LOCAL_TARGET:
            self.stop_local_target()

        self.qspy.detach()

        if CONFIG.AUTOSTART_QSPY:
            self.stop_qspy()


    @staticmethod
    def run_program(argumentList, startInConsole):
        """ Helper method for starting programs like qspy and target

        Args:
          argumentList : Popen list where first item is program name
          startInConsole : bool, if True starts in new console

        Returns:
          A process ID that can be used to terminate the program
        """

        if sys.platform == 'win32':
            if startInConsole:
                process_id = Popen(argumentList, creationflags=CREATE_NEW_CONSOLE)
            else:
                process_id = Popen(argumentList)
        elif sys.platform == 'linux':
            if startInConsole:
                cmd_list = ['gnome-terminal', '--disable-factory', '-e']
                argstring = " ".join(argumentList)
                cmd_list.append(argstring)
                process_id = Popen(cmd_list, preexec_fn=os.setpgrp)
            else:
                process_id = Popen(argumentList, preexec_fn=os.setpgrp)

        elif sys.platform == 'darwin':
            # Don't know if this works, doubt it
            if startInConsole:
                cmd_list = ['open', '-W', '-a', 'Terminal.app', argumentList[0], '--args']
                argstring = " ".join(argumentList[1:])
                cmd_list.append(argstring)
                process_id = Popen(cmd_list)
            else:
                process_id = Popen(argumentList)

        else:
            assert False, "Unknown OS platform:{0}".format(sys.platform)

        return process_id

    @staticmethod
    def halt_program(process_id):
        """ Helper method for stopping programs like qspy and target

        Args:
          process_id : process to terminate

        """
        if process_id is not None:
            if sys.platform == 'win32':
                process_id.terminate()
                process_id.wait()
                process_id = None
            elif sys.platform == 'linux':
                os.killpg(process_id.pid, signal.SIGINT)
            elif sys.platform == 'darwin':
                # Don't know how to do this on Mac
                process_id.terminate()
                process_id.wait()
                os.killpg(process_id.pid, signal.SIGINT)
                process_id = None
            else:
                assert False, "Unknown OS platform:{0}".format(sys.platform)
        else:
            assert False, "Trying to to stop non-existant process"




    def start_qspy(self):
        """ Helper to automatically start qspy. """
        args = ['qspy', '-u' + str(CONFIG.QSPY_UDP_PORT)]

        # Local targets use tcp sockets
        if CONFIG.USE_LOCAL_TARGET:
            args.append('-t')
        else:
            args.append('-c' + CONFIG.QSPY_COM_PORT)
            args.append('-b' + str(CONFIG.QSPY_BAUD_RATE))

        # Start qspy
        self.qspy_process = qutest_context.run_program(args, True)

    def stop_qspy(self):
        """ Helper to stop qspy. """
        if self.qspy_process is not None:
            qutest_context.halt_program(self.qspy_process)

    def start_local_target(self):
        """ Used to start a local target executable for dual targeting. """

        self.target_process = qutest_context.run_program(
            [CONFIG.LOCAL_TARGET_EXECUTABLE, CONFIG.LOCAL_TARGET_QSPY_HOST], CONFIG.LOCAL_TARGET_USES_CONSOLE)

    def stop_local_target(self):
        """ Stops local target. """

        if self.target_process is not None:
            self.qspy.sendReset() # Sending reset halts target
            time.sleep(0.500)
            self.target_process = None

    def reset_target(self):
        """ Resets the target (local or remote). """

        # Clear have target flag
        self.have_target_event.clear()

        # Flush queue in case they miss an expect
        while not self.text_queue.empty():
            print("Flushing Text:", self.text_queue.get())

        # If running with a local target, kill and restart it
        if CONFIG.USE_LOCAL_TARGET:
            if self.target_process is not None:
                self.stop_local_target()
            self.start_local_target()
        else:
            self.qspy.sendReset()

        # Wait for target to be back up
        assert self.have_target_event.wait(
            CONFIG.TARGET_START_TIMEOUT_SEC), "Timeout waiting for target to reset"

        # Call on_reset if defined
        if hasattr(self, "on_reset"):
            on_reset_method = getattr(self, 'on_reset')
            on_reset_method(self)

    def Continue(self):
        """ Sends a continue to a paused target. """

        self.qspy.sendContinue()
        self.expect('           Trg-Ack  QS_RX_TEST_CONTINUE')

    def call_on_setup(self):
        """ Sends a setup command to target."""

        self.qspy.sendSetup()
        self.expect('           Trg-Ack  QS_RX_TEST_SETUP')
        if self.on_setup_callback is not None:
            self.on_setup_callback(self)

    def call_on_teardown(self):
        """ Sends a teardown command to target."""

        self.qspy.sendTeardown()
        self.expect('           Trg-Ack  QS_RX_TEST_TEARDOWN')
        if self.on_teardown_callback is not None:
            self.on_teardown_callback(self)

    def call_on_reset(self):
        """ Resets the target and calls any registered reset handler """

        self.reset_target()
        if self.on_reset_callback is not None:
            self.on_reset_callback(self)


    def expect_pause(self):
        """ Pause expectation. """

        self.expect('           TstPause')

    def glb_filter(self, *args):
        """ Sets the global filter.

        Args:
            args : One or more qspy.FILTER enumerations
        """
        self.qspy.sendGlobalFilters(*args)
        self.expect('           Trg-Ack  QS_RX_GLB_FILTER')

    def loc_filter(self, object_kind, object_id):
        """ Sets a local filter.

        Args:
          object_kind : kind of object from qspy.QS_OBJ_KIND
          object_id : the object which can be an address integer or a dictionary name string
        """

        self.qspy.sendLocalFilter(object_kind, object_id)
        self.expect('           Trg-Ack  QS_RX_LOC_FILTER')

    def current_obj(self, object_kind, object_id):
        """ Sets the current object in qspy.

        Arguments:
        object_kind : kind of object from qspy.QS_OBJ_KIND
        object_id : the object which can be an address integer or a dictionary name string
        """

        self.qspy.sendCurrentObject(object_kind, object_id)
        self.expect('           Trg-Ack  QS_RX_CURR_OBJ')

    def post(self, signal, parameters=None):
        """ Posts an event to the object selected with current_obj().

        Args:
          signal : signal string or number
          parameters : optional event payload defined using struct.pack
        """

        self.qspy.sendEvent(PRIO_COMMAND.POST.value,  signal, parameters)
        self.expect('           Trg-Ack  QS_RX_EVENT')

    def publish(self, signal, parameters=None):
        """ Publishes an event in the system.

        Args:
          signal : signal string or number
          parameters : optional event payload defined using struct.pack
        """

        self.qspy.sendEvent(PRIO_COMMAND.PUBLISH,  signal, parameters)
        self.expect('           Trg-Ack  QS_RX_EVENT')

    def dispatch(self, signal, parameters=None):
        """ Dispatches an event to the object selected with current_obj().

        Args:
          signal : signal string or number
          parameters : optional event payload defined using struct.pack
        """

        self.qspy.sendEvent(PRIO_COMMAND.DISPATCH.value,  signal, parameters)
        self.expect('           Trg-Ack  QS_RX_EVENT')

    def init(self, signal=0, parameter=None):
        """ Take the top-most initial transition in the current object"""

        self.qspy.sendEvent(PRIO_COMMAND.DO_INIT_TRANS, signal, parameter)
        self.expect('           Trg-Ack  QS_RX_EVENT')

    def probe(self, function, data_word):
        """ Sends a test probe to the target.

        The Target collects these Test-Probe preserving the order in which they were sent.
        Subsequently, whenever a given API is called inside the Target, it can
        obtain the Test-Probe by means of the QS_TEST_PROBE_DEF() macro.
        The QS_TEST_PROBE_DEF() macro returns the Test-Probes in the same
        order as they were received to the Target. If there are no more Test-
        Probes for a given API, the Test-Probe is initialized to zero.

        Args:
          function : string function name or integer raw address
          data_word : a single uint 32 for the probe
        """

        self.qspy.sendTestProbe(function, data_word)
        self.expect('           Trg-Ack  QS_RX_TEST_PROBE')

    def tick(self, rate=0):
        """ Triggers a system clock tick.

        Args:
          rate : (optional) which clock rate to tick
        """

        self.qspy.sendTick(rate)
        self.expect('           Trg-Ack  QS_RX_TICK')

    def command(self, command_id, param1=0, param2=0, param3=0):
        """ Sends a qspy command to the target.

        Args:
          command_id : string command name or number
          param1 : (optional) integer argument
          param2 : (optional) integer argument
          param3 : (optional) integer argument
        """

        self.qspy.sendCommand(command_id, param1, param2, param3)
        self.expect('           Trg-Ack  QS_RX_COMMAND')

    def fill(self, offset, size, num, item = 0):
        """ Fills data into the target

        Args:
          offset : offset, in bytes from start of current_obj
          size : size of data item (1, 2, or 4)
          num : number of data items to fill
          item : (optional, default zero) data item to fill with
          """
        self.qspy.sendFill(offset, size, num, item )
        self.expect('           Trg-Ack  QS_RX_FILL')

    def peek(self, offset, size, num):
        """ Peeks data at the given offset from the start address of the
        Application (AP) Current-Object inside the Target.

        Args:
          offset :offset [in bytes] from the start of the current_obj AP
          size : size of the data items (1, 2, or 4)
          num  : number of data items to peek
        """
        assert size == 1 or size == 2 or size == 4, 'Size must be 1, 2, or 4'

        self.qspy.sendPeek(offset, size, num)

    def poke(self, offset, size, data):
        """ Pokes provided data at the given offset from the
        start address of the Application (AP) Current-Object inside the Target.

        Args:
          offset : offset [in bytes] from the start of the current_obj AP
          size :  size of the data items (1, 2, or 4)
          data :  binary data to send
        """

        assert size == 1 or size == 2 or size == 4, 'Size must be 1, 2, or 4'
        length = len(data)
        num = length // size
        self.qspy.sendPoke(offset, size, num, data)
        self.expect('           Trg-Ack  QS_RX_POKE')


    def expect(self, match):
        """ asserts that match string is sent by the cut

        If no string is returned in EXPECT_TIMEOUT_SEC the test will fail

        Args:
          match : is either an entire string or one prepended with
                  %timestamp to ignore timestamp and/or
                  postpended with * to ignore ending
        """

        try:
            next_packet = self.text_queue.get(
                timeout=CONFIG.EXPECT_TIMEOUT_SEC)
        except:
            __tracebackhide__ = True
            pytest.fail('Expect Timeout for match:"{0}"'.format(match))
            #assert False, 'Expect Timeout for match:"{0}"'.format(match)

        _, line = qspy.parse_QS_TEXT(next_packet)

        magic_string = '%timestamp'

        if match.startswith(magic_string):
            line_start = len(magic_string)
        else:
            line_start = 0

        if match.endswith('*'):
            line_end = match.find('*', line_start)
            match = match.rstrip('*')
        else:
            line_end = len(match)

        expected = match[line_start:]
        actual = line[line_start:line_end]
        #assert expected == actual, 'Expect Match Failed! \nExpected:\"{0}\"\nReceived:\"{1}\"'.format(expected, actual)
        if expected != actual:
            __tracebackhide__ = True
            pytest.fail('Expect Match Failed! \nExpected:\"{0}\"\nReceived:\"{1}\"'.format(
                expected, actual))

    ################### qspy backend callbacks #######################

    def OnRecord_QS_TARGET_INFO(self, packet):
        self.have_target_event.set()

    def OnPacket_ATTACH(self, packet):
        self.attached_event.set()

    def OnRecord_QS_TEXT(self, record):
        # put packet in text queue
        self.text_queue.put(record)
        #recordId, line = self.qspy.parse_QS_TEXT(record)
        #print('OnRecord_QS_TEXT record:{0}, line:"{1}"'.format(recordId.name, line) )

def main():
    """ Main entry point for qutest """

    options = ['-v', '--tb=short']

    # Parse command line like Tcl script
    num_tests = 0
    args = sys.argv[1:]
    num_args = len(args)

    if '-h' in args or '--help' in args or '?' in args:
        print("Usage: qutest [test-scripts] [host_exe] [host[:port]] [local_port]")
        return

    if num_args >= 1: # 1 or more test scripts
        test_scripts = []
        for arg in args:
            if '.py' in arg:
                test_scripts.append(arg)
                num_tests += 1

        if '*.py' not in test_scripts:
            options.extend(test_scripts) # pass test scripts to pytest
        else:
            num_tests = 1
        #print("************ test scripts", test_scripts, "num tests", num_tests)
    if num_args > (num_tests) :
        host_exe = args[num_tests]
        if len(host_exe) > 0: # passing "" means no local host
            CONFIG.USE_LOCAL_TARGET = True
            CONFIG.LOCAL_TARGET_EXECUTABLE = host_exe
            #print(f'host_exe:{host_exe}')
    if num_args > (1 + num_tests):
        host_port = args[1 + num_tests]
        host = host_port.split(':')
        CONFIG.QSPY_HOST = host[0]
        if len(host) > 1:
            port = host[1]
            CONFIG.QSPY_UDP_PORT = int(port)
        #print(f'host_port:{host_port}')
    if num_args > (2 + num_tests):
        local_port = args[2 + num_tests]
        CONFIG.QSPY_LOCAL_UDP_PORT = int(local_port)

    # Run pytest with options
    pytest.main(options)

if __name__ == "__main__":
    main()
