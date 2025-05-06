![](https://www.state-machine.com/img/qutest_banner.jpg)

The `qutest.py` package is a [test-script](https://www.state-machine.com/qtools/qutest_script.html) runner for the [QUTest testing system](https://www.state-machine.com/qtools/qutest.html).


# General Requirements
In order to run tests in the [QUTest environment]((https://www.state-machine.com/qtools/qutest.html)), you need the following three components:

1. The [test fixture in C or C++](https://www.state-machine.com/qtools/qutest_fixture.html) running on a remote target (or the host computer)
2. The [QSPY host application](https://www.state-machine.com/qtools/qspy.html) running and connected to the target
3. The `qutest.py` script runner and some test scripts.

> **NOTE:**
The `qutest.py` script runner requires standard Python 3, which is included in
the [QTools distribution](https://www.state-machine.com/qtools) for Windows
and is typically included with other host operating systems, such as Linux and macOS.

# Usage without Installation
The `qutest.py` script runner can be used standalone, **without installation** in your Python system (see [Examples below](#examples-for-windows)).

> **REMARK:**
The latest `qutest.py` script is included in the [QTools collection](https://github.com/QuantumLeaps/qtools). Also, the QTools collection for Windows already includes Python 3, so you don't need to install anything extra.

# Installation with `pip`
You can install the `qutest` package with the standard `pip` package manager.

> **NOTE:**
The `qutest` package is no longer available in the [PyPi package index](https://pypi.org)
due to the overcomplicated "two-factor authentication".

Instead, you can direct `pip` to install directly from the `qutest` directory
(e.g., `/qp/qtools/qutest`):

`pip install /qp/qtools/qutest/qutest.tar.gz`

Alternatively, you can direct `pip` to install from Quantum Leaps GitHub:

`pip install https://github.com/QuantumLeaps/qtools/releases/latest/download/qutest.tar.gz`


# Using `qutest.py`

If you are using `qutest.py` as a standalone Python script, you invoke it as follows:

`python3 <path-to-qutest-script>/qutest.py <command-line-options>`

Alternatively, if you've installed `qutest.py` with `pip`, you invoke it as follows:

`qutest <command-line-options>`


## Command-line Options
The Python test scripts are executed by the QUTest test script runner `qutest.py` (typically located in `qtools/qutest/` folder), with the following usage:

> **ATTENTION**
The `qutest.py` script runner command-line options have been expanded and changed at version 7.2.0. Unfortunately, it was not possible to preserve the backwards compatibility with the earlier versions.

```
usage: python qutest.py [-h] [-v] [-e [EXE]] [-q [QSPY]] [-l [LOG]] [-o [OPT]] [scripts ...]

QUTest test script runner

positional arguments:
  scripts               List (comma-separated) of test scripts to run

options:
  -h, --help            show this help message and exit
  -v, --version         Display QUTest version
  -e [EXE], --exe [EXE]
                        Optional host executable or debug/DEBUG
  -q [QSPY], --qspy [QSPY]
                        optional qspy host, [:ud_port][:tcp_port]
  -l [LOG], --log [LOG]
                        Optional log directory (might not exist yet)
  -o [OPT], --opt [OPT]
                        xcob: x:exit-on-fail, c:qspy-clear, o:qspy-save-txt, b:qspy-save-bin

More info: https://www.state-machine.com/qtools/qutest.html
```

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
fixture already).

- `qspy_host[:udp_port]` - optional host-name/IP-address:port for the host
running the QSPY host utility. If not specified, the default
is 'localhost:7701'.

- `tcp_port` - optional the QSpy TCP port number for connecting
host executables. If not specified, the default is '6601'.

> **NOTE:**
For reliable operation it is recommended to apply the short options without
a space between the option and the parameter (e.g., 1-q192.168.1.100, -ocx1).


## Examples (for Windows):

```
[1] python3 %QTOOLS%\qutest\qutest.py
[2] python3 %QTOOLS%\qutest\qutest.py -- test_mpu.py
[3] python3 %QTOOLS%\qutest\qutest.py -ebuild/test_dpp.exe
[4] python3 %QTOOLS%\qutest\qutest.py -ebuild/test_dpp.exe -q192.168.1.100 -l../log -oco
[5] qutest -qlocalhost:7702 -oxc --  test_qk.py,test_mpu.py
[6] python3 %QTOOLS%\qutest\qutest.py -eDEBUG -- test_mpu.py
```

`[1]` runs all test scripts (*.py) in the current directory on a remote
target connected to QSPU host utility.

`[2]` runs the test script test_mpu.py in the current directory on a remote
target connected to QSPU host utility.

`[3]` runs all test scripts (*.py) in the current directory and uses the
host executable build/test_dpp.exe (test fixture).

`[4]` runs all test scripts (*.py) in the current directory, uses the
host executable build/test_dpp.exe (test fixture), and connects to QSPY
running on a machine with IP address 192.168.1.100. Also produces QUTest
log (-l) in the directory ../log. Also clears the QUTest screen before the
run (-oc) and causes QSPY to save the text output to a file (-oo)

`[5]` runs "qutest" (installed with pip) to execute the test scripts
`test_qk.py`,`test_mpu.py`` in the current directory, and connects to
QSPY at UDP-host:port localhost:7701.

`[6]` runs "qutest" in the DEBUG mode to execute the test script `test_mpu.py`
in the current directory.


## Examples (for Linux/macOS):
```
[1] python3 $(QTOOLS)/qutest/qutest.py
[2] python3 $(QTOOLS)/qutest/qutest.py -- test_mpu.py
[3] python3 $(QTOOLS)/qutest/qutest.py -ebuild/test_dpp
[4] python3 $(QTOOLS)/qutest/qutest.py -ebuild/test_dpp -q192.168.1.100 -l../log -oco
[5] qutest -qlocalhost:7702 -oxc --  test_qk.py,test_mpu.py
[6] python3 $(QTOOLS)/qutest/qutest.py -eDEBUG -- test_mpu.py
```

`[1]` runs all test scripts (*.py) in the current directory on a remote
target connected to QSPU host utility.

`[2]` runs the test script test_mpu.py in the current directory on a remote
target connected to QSPU host utility.

`[3]` runs all test scripts (*.py) in the current directory and uses the
host executable build/test_dpp (test fixture).

`[4]` runs all test scripts (*.py) in the current directory, uses the
host executable build/test_dpp (test fixture), and connects to QSPY running
on a machine with IP address 192.168.1.100. Also produces QUTest log (-l)
in the directory ../log. Also clears the QUTest screen before the run (-oc)
and causes QSPY to save the text output to a file (-oo)

`[5]` runs "qutest" (installed with pip) to execute the test scripts
`test_qk.py`,`test_mpu.py` in the current directory, and connects to
QSPY at UDP-host:port localhost:7701.

`[6]` runs "qutest" in the DEBUG mode to execute the test script
`test_mpu.py` in the current directory.


# Generating Test Logs
As required for safety certification, the `qutest.py` test runner can
generate permanent records of the runs by producing log files. This feature
is enabled by the `-l<log-dir>` command-line option.

The various make-files supplied in QP/C and QP/C++ allow you to supply
the command-line options for saving QUTest logs (by defining the `LOG=`
symbol while invoking `make`), for example:

```
[1] make LOG=.
[2] make LOG=../log
[3] make LOG=c:/cert/logs
```

`[1]` generates QUTest log file in the current directory (`.`)

`[2]` generates QUTest log file in the `../log` directory
(relative to the current directory)

`[3]` generates QUTest log file in the absolute directory `c:/cert/logs`


The following following listing shows the generated log file:

```
Run ID    : 221221_161550
Target    : build/test_qutest.exe

===================================[group]====================================
test_assert.py

This test group contains tests that intenionally FAIL,
to exercise failure modes of the QUTest system.

[ 1]--------------------------------------------------------------------------
Expected assertion
                                                             [ PASS (  0.1s) ]
[ 2]--------------------------------------------------------------------------
Unexpected assertion (should FAIL!)
  @test_assert.py:22
exp: "0000000002 COMMAND CMD_A 0"
got: "0000000002 =ASSERT= Mod=test_qutest,Loc=100"
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ![ FAIL (  0.2s) ]
[ 3]--------------------------------------------------------------------------
Simple passing test
                                                             [ PASS (  0.1s) ]
[ 4]--------------------------------------------------------------------------
Wrong assertion expectation (should FAIL!)
  @test_assert.py:32
exp: "0000000002 =ASSERT= Mod=test_qutest,Loc=200"
got: "0000000002 =ASSERT= Mod=test_qutest,Loc=100"
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ![ FAIL (  1.1s) ]
[ 5]--------------------------------------------------------------------------
Simple passing test
                                                             [ PASS (  0.1s) ]

=================================[ SUMMARY ]==================================

Target ID : 221221_161031 (QP-Ver=720)
Log file  : ./qutest221221_161550.txt
Groups    : 1
Tests     : 5
Skipped   : 0
FAILED    : 2 [ 2 4 ]

==============================[ FAIL (  2.7s) ]===============================
```

# The "qutestify" Utility
While working with QUTest, it is often convenient to copy the parts of
the QSPY raw output and convert them into QUTest expectations. For example,
the following raw output:

```
0000000007 Disp===> Obj=TstSM_inst,Sig=B_SIG,State=TstSM_s211
===RTC===> St-Exit  Obj=TstSM_inst,State=TstSM_s211
===RTC===> St-Exit  Obj=TstSM_inst,State=TstSM_s21
===RTC===> St-Entry Obj=TstSM_inst,State=TstSM_s22
0000000008 ===>Tran Obj=TstSM_inst,Sig=B_SIG,State=TstSM_s2->TstSM_s22
0000000009 Trg-Done QS_RX_EVENT
```
shall be transfored into the following expectations in a test script:

```
expect("@timestamp Disp===> Obj=TstSM_inst,Sig=B_SIG,State=TstSM_s211")
expect("===RTC===> St-Exit  Obj=TstSM_inst,State=TstSM_s211")
expect("===RTC===> St-Exit  Obj=TstSM_inst,State=TstSM_s21")
expect("===RTC===> St-Entry Obj=TstSM_inst,State=TstSM_s22")
expect("@timestamp ===>Tran Obj=TstSM_inst,Sig=B_SIG,State=TstSM_s2->TstSM_s22")
expect("@timestamp Trg-Done QS_RX_EVENT")
```

The small Python utility `qutestify.py` is provided to automate the process.
The utility takes a test script file name as parameter. It works by scanning
the provided test script file for the signatures of raw QSpy output. Every
recognized line is transformed into `expect("...")`.

Here is an example of the `qutestify` run:

```
> python3 %QTOOLS%\qutest\qutestify.py test.py

QUTestestify utility 8.0.4
Copyright (c) 2005-2025 Quantum Leaps, www.state-machine.com
qutestifying: test.py
changed lines: 6.
```

# More Information
More information about the QUTest unit testing harness is available
online at:

- https://www.state-machine.com/qtools/qutest.html

More information about the QP/QSPY software tracing system is available
online at:

- https://www.state-machine.com/qtools/qpspy.html
