![](https://www.state-machine.com/img/qutest_banner.jpg)

The "qutest" Python package supports developing
[Python test scripts](https://www.state-machine.com/qtools/qutest_script.html)
for the
[QUTest unit testing harness](https://www.state-machine.com/qtools/qutest.html).
Thus the "qutest" Python package is part of the larger unit-testing
infrastructure for embedded C or C++ code on embedded targets as well as
host computers.


General Requirements
====================
The "qutest" package requires standard Python 3, which is included in
the [QTools distribution](https://www.state-machine.com/qtools) for Windows
and is typically included with other operating systems, such as Linux and macOS.

To execute test scripts in Python, you need to first launch the
[QSPY console application](https://www.state-machine.com/qtools/qspy.html)
to communicate with the chosen embedded target or the host executable.
Once QSPY is running, from a separate terminal you can launch `qutest.py`
and "attach" to the [QSPY UDP socket](https://www.state-machine.com/qtools/qspy_udp.html).
After this communication has been established, "qutest" can execute the test
scripts in Python to perform testing of the
[test fixture](https://www.state-machine.com/qtools/qutest_fixture.html)
inside the target (through QSPY).

![](https://www.state-machine.com/img/qutest_targ.gif)


Installation
============
The `qutest.py` script can be used standalone, **without** any
installation (see Using "qutest" below).

Alternatively, you can **install** `qutest.py` with `pip` from PyPi by
executing the following command:


`pip install qutest`


Or directly from the sources directory (e.g., `/qp/qtools/qutest`):


`python setup.py install --install-dir=/qp/qtools/qutest`


Using "qutest"
==============
If you are using `qutest` as a standalone Python script, you invoke
it as follows:

`python /path-to-qutest-script/qutest.py [-x] [test-scripts] [host_exe] [qspy_host[:udp_port]] [qspy_tcp_port]`

Alternatively, if you've installed `qutest` with `pip`, you invoke
it as follows:

`qutest [-x] [test-scripts] [host_exe] [qspy_host[:udp_port]] [qspy_tcp_port]`


Command-line Options
--------------------
- `-x` - optional flag that causes `qutest` to exit on first test failure.

- `test_scripts` - optional specification of the Python test scripts to run.
If not specified, qutest will try to run all *.py files in the current
directory as test scripts

- `host_exe | DEBUG` - optional specification of the test-fixture compiled
for the host (host executable) for testing on the *host computer*.
The special value **DEBUG** means that `qutest` will run in the "debug mode",
in which it will NOT launch the host executables and it will wait for the
Target reset and other responses from the Target. If `host_exe` is not
specified, an **embedded target** is assumed (which is loaded with the test
fixture alredy).

- `qspy_host[:udp_port]` - optional host-name/IP-address:port for the host
running the QSPY host utility. If not specified, the default
is 'localhost:7701'.

- `tcp_port` - optional the QSpy TCP port number for connecting
host executables. If not specified, the default is '6601'.


Examples (for Windows):
-----------------------
`python %QTOOLS%\qutest\qutest.py`

`python %QTOOLS%\qutest\qutest.py *.py`

`python %QTOOLS%\qutest\qutest.py *.py build\dpp.exe`

`qutest *.py build\dpp.exe 192.168.1.100:7705`

`qutest *.py build\dpp.exe localhost:7701 6605`

`qutest *.py DEBUG`

`qutest *.py DEBUG localhost:7701 6605`


Examples (for Linux/macOS):
---------------------------
`python $(QTOOLS)/qutest/qutest.py`

`python $(QTOOLS)/qutest/qutest.py *.py`

`qutest *.py build/dpp`

`qutest *.py build/dpp 192.168.1.100:7705`

`qutest *.py build/dpp localhost:7701 6605`

`qutest *.py DEBUG`

`qutest *.py DEBUG localhost:7701 6605`


More Information
================
More information about the QUTest unit testing harness is available
online at:

- https://www.state-machine.com/qtools/qutest.html

More information about the QP/QSPY software tracing system is available
online at:

- https://www.state-machine.com/qtools/qpspy.html


