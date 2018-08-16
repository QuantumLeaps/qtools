##
# @file
# @ingroup qpspypy
# @brief Configuraton parameters for "qutest"; can be modifed here
# or via another test start script

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

#### QSpy settings  ####
# Set to true have QSpy automatically start/stop at the beginning/end of a test session
# Note: qspy must be on system path
AUTOSTART_QSPY = False

# Serial port that target is connected to for qspy, used when AUTO_START_QSPY is true
# MODIFY FOR YOUR REMOTE TARGET
QSPY_COM_PORT = 'COM1'

# BAUD rate for serially connected qspy, used when AUTO_START_QSPY is true
# MODIFY FOR YOUR REMOTE TARGET
QSPY_BAUD_RATE = 115200

# The UDP port to connect to qpsy on
QSPY_UDP_PORT = 7701

# The local UDP port used to connect to qspy with, default None means
# that the system will choose one
QSPY_LOCAL_UDP_PORT = None

# The host machine that qspy is on
QSPY_HOST = 'localhost'


###### Local host settings ######
# Set to true to launch and connect to a local target
USE_LOCAL_TARGET = False

# Set to the IP address of where the QSpy resides (normally the local host)
LOCAL_TARGET_QSPY_HOST = 'localhost'

# Set this to the target executible name (e.g. test_dpp), target must be on system path
LOCAL_TARGET_EXECUTABLE = 'test_dpp'

# Set this to true to have the target start in a console
LOCAL_TARGET_USES_CONSOLE = False


###### Misc. settings ##########
# How long to wait for expect calls to return (was TIMEOUT_MS in qutest.tcl)
EXPECT_TIMEOUT_SEC = 0.500

# Reset the target on every test setUp call that uses the qutest fixture
RESET_TARGET_ON_SETUP = True

# How long we wait for the target to come up and send the target info record
TARGET_START_TIMEOUT_SEC = 1.000

# How long we wait for the QSPY to come up
QSPY_ATTACH_TIMEOUT_SEC = 1.0
