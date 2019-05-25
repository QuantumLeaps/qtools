About this Directory
====================
This directory contains the Python module "qutest.py" that supports
writing Python test scripts for the QUTest unit testing harness.

The module "qutest_dsl.py" contains the separate Doxygen documentation
of the small unit-testing "Domain Specific Language" (DSL) for writing
test scripts in Python.


General Requirements
====================
The "qutest" package requires Python 2.7+ or Python 3.4+, which are
freely available from the Internet.

To launch any of the scripts in this directory, you need to first run
the QSPY console application (version 6.x or higher) with the -u option
(the -u option opens up the UDP socket for attaching "front-ends").
Once QSPY is running, you can "attach" to the UDP socket and start
communicating with the QSPY back-end or to the Target (through QSPY).


Using "qutest"
===============
The usage of the "qutest" is as follows:

python <qutest-dir>qutest.py [-x] [test-scripts] [host_exe] [qspy_host[:udp_port]] [qspy_tcp_port]

where:
<qutest-dir>   - directory with the qutest.py script
[test_scripts] - optional specification of the Python test scripts to run.
                 If not specified, qutest will try to run all *.py files
                 in the current directory as test scripts

[host_exe]     - optional specification of the host executable to
                 launch for testing embedded code on the host computer.
                 The special value DEBUG means that qutest.py will start
                 in the 'debug' mode, in which it will NOT launch the
                 host executables and it will wait for the Target reset
                 and other responses from the Target.
                 If host_exe is not specified, an embedded target is assumed.

[qspy_host[:udp_port]] - optional host-name/IP-address:port for the host
                 running the QSpy utility. If not specified, the default
                 is localhost:7701.

[tcp_port]     - optional the QSpy TCP port number for connecting
                 host executables.

Examples (for Windows):
python %QTOOLS%\qspy\py\qutest.py
python %QTOOLS%\qspy\py\qutest.py *.py
python %QTOOLS%\qspy\py\qutest.py *.py build\dpp.exe
python %QTOOLS%\qspy\py\qutest.py *.py build\dpp.exe 192.168.1.100:7705
python %QTOOLS%\qspy\py\qutest.py *.py build\dpp.exe localhost:7701 6605
python %QTOOLS%\qspy\py\qutest.py *.py DEBUG
python %QTOOLS%\qspy\py\qutest.py *.py DEBUG localhost:7701 6605


Examples (for Linux/MacOS):
python $(QTOOLS)/qspy/py/qutest.py
python $(QTOOLS)/qspy/py/qutest.py *.py
python $(QTOOLS)/qspy/py/qutest.py *.py build/dpp
python $(QTOOLS)/qspy/py/qutest.py *.py build/dpp 192.168.1.100:7705
python $(QTOOLS)/qspy/py/qutest.py *.py build/dpp localhost:7701 6605
python %QTOOLS%\qspy\py\qutest.py *.py DEBUG
python %QTOOLS%\qspy\py\qutest.py *.py DEBUG localhost:7701 6605


More Information
================
More information about the QUTest unit testing harness is available
online at:

https://www.state-machine.com/qtools/qutest.html

More information about the QP/QSPY software tracing system is available
online at:

https://www.state-machine.com/qtools/qpspy.html


