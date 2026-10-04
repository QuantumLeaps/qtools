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

# Usage without Installation (recommended)
The recommended way is to run the scripts of the `qtools/qutest/` directory
of the [QTools collection](https://github.com/QuantumLeaps/qtools) directly
with Python 3, **without installation** in your Python system
(see [Examples below](#examples-for-windows)):

- `qutest.py` - the QUTest test-script runner
  (`python3 <path-to-qtools>/qutest/qutest.py <command-line-options>`)
- `qimpact.py` - the instrumentation-impact check of the QP host unit tests,
  called by the QP test Makefiles (see [The "qimpact" Utility](#the-qimpact-utility))
- `qutestify.py` - the test-script converter (see
  [The "qutestify" Utility](#the-qutestify-utility))

This is how the QP test Makefiles and the test runner use them, and these
copies are the ones qualified in the Certification Kits (the self-test cases of
`qimpact.py` in `qimpact_selftest/` are available only in this directory).

> **REMARK:**
The QTools collection for Windows already includes Python 3, so you don't
need to install anything extra.

# Installation with `pip` (optional)
Alternatively, you can install the `qutest` package from the
[PyPi package index](https://pypi.org/project/qutest/) with the standard
`pip` package manager:

`pip install qutest`

The package installs the commands `qutest`, `qimpact`, and `qutestify`
(the same scripts as in `qtools/qutest/`, without the self-test cases of
`qimpact`).


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
directory as test scripts. The scripts are run in the order of the
arguments; the files matched by a wildcard argument are run in sorted
order (the same on all hosts), and a script matched by more than one
argument is run only once. An explicitly specified argument that matches
no file is an error. The resolved list of scripts is reported in the log
(`Scripts   :` line).

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

# The "qimpact" Utility
`qimpact.py` checks, as part of every host unit test of the QP Framework, that
the QP/Spy instrumentation (`Q_SPY`, `Q_UTEST`) is purely **additive** for the
module under test: the code compiled for production must be the code compiled
for the test, minus complete, instrumentation-only statements. When the check
passes, the test results and the structural coverage measured on the
instrumented test build apply to the production build. The same script works
for C (`gcc`) and C++ (`g++`). It was first released with QTools 8.1.6.

| File | Location |
|------|----------|
| `qimpact.py` | `qtools/qutest/` (next to `qutest.py`) |
| `qimpact_selftest/*.c, *.cpp` | `qtools/qutest/qimpact_selftest/` (needed only for the self-test) |
| `gnu-host_c.mak`, `gnu-host_cpp.mak` | `tests/shared/` of the QP/C and QP/C++ test trees (call the check) |

No per-test Makefile changes are needed.

## How it works
The shared host Makefile knows the module under test (`$(PROJECT).c` or
`$(PROJECT).cpp`, found through `VPATH`; the same module that gcov reports).
After the QUTest run and gcov, the `run` target:

1. compiles the module twice with exactly the same compiler (`$(CC)` or
   `$(CPP)`), base flags, include paths, and port:
   - test build: `$(CFLAGS)` / `$(CPPFLAGS)` (with `-DQ_SPY -DQ_UTEST`);
   - production build: `$(CFLAGS_PROD)` / `$(CPPFLAGS_PROD)`, i.e. the same
     without `$(IMP_DEFS)` (`-DQ_SPY -DQ_UTEST`);

   each time adding `-fdump-tree-original=<file>`, which writes GCC's own
   parse of every function (before optimization);
2. runs `qimpact.py`, which compares the two dumps and writes its report to
   `$(PROJECT).instr` (displayed after the coverage summary). The test
   runner `tests/auto_run/run_tests.py` archives the report in the test log.

Both steps are the canned recipe `IMP_CHECK` in the shared Makefile. `make`
ignores the exit status of the check (exit codes: 0 PASS or N/A, 1 FAIL,
2 REVIEW, 3 tool error); the test runner `run_tests.py` enforces the verdict.

`qimpact.py` itself writes no files except the report. Everything the
compiler produces stays in the Makefile's build directory (`BIN_DIR`,
`build/`) for manual inspection:

| File | Content |
|------|---------|
| `build/$(PROJECT)_instr_test.tree` | compiler dump of the test build |
| `build/$(PROJECT)_instr_prod.tree` | compiler dump of the production build |
| `build/$(PROJECT)_instr_test.o`, `..._prod.o` | object files (also `.gcno` with coverage flags) |
| `build/qimpact_selftest/<case>_test.tree`, `<case>_prod.tree`, `*.o` | outputs of `make instr_selftest` |

The check's files are deleted only right before the next check. The object
files also serve as the evidence that both compilations succeeded: GCC
writes the dump even when a compilation fails, and the C++ front end does
not always mark errors in it, so a missing object file is a tool error.

## Command line

```
qimpact.py [-v] SOURCE DUMPS REPORT COMPILER [DEFS]
qimpact.py --selftest COMPILER [WORKDIR]
```

| Argument | Meaning |
|----------|---------|
| `SOURCE` | source file of the module under test |
| `DUMPS` | common prefix of the dumps and objects: `DUMPS_test.tree`, `DUMPS_prod.tree`, `DUMPS_test.o`, `DUMPS_prod.o` |
| `REPORT` | report file, appended to (`-`: full report to standard output) |
| `COMPILER` | compiler and base flags as one quoted argument (recorded in the report with the compiler version) |
| `DEFS` | removed instrumentation defines as one quoted argument (report only) |
| `WORKDIR` | self-test only: directory for the compiler outputs of the cases, kept (default: a temporary directory, removed) |

The C Makefile calls it as
`$(QIMPACT) "$(IMP_SRC)" $(IMP_DUMPS) $(1) "$(CC) $(strip $(CFLAGS_BASE))" "$(IMP_DEFS)"`
(C++: `"$(CPP) $(strip $(CPPFLAGS_BASE))"`), where `IMP_DUMPS` is
`$(BIN_DIR)/$(PROJECT)_instr` and `$(1)` the report file. The short argument
list matters where `python3` is a batch-file launcher (such as
`%QTOOLS%\bin\python3.bat`), which forwards only a limited number of
arguments.

Makefile targets:

| Target | Action |
|--------|--------|
| `run` (default) | QUTest run, gcov, then the check (report in `$(PROJECT).instr`) |
| `instr` | the check alone (report in `$(PROJECT).instr`, displayed at the end) |
| `instr_selftest` | the self-test (tool qualification, see below) |
| `show` | also lists `QIMPACT` and `IMP_SRC` |

The variable `QIMPACT` defaults to `python3 $(QTOOLS)/qutest/qimpact.py`
and can be overridden on the `make` command line.

The report lists every function with test-only code or a finding; functions
identical in both builds are only counted (`-v` lists all functions and
the test-only statements).

## Rules
After normalization (compiler label/temporary numbers, no-op statements,
debug markers, dead `if (0) {}` blocks, bare-scope braces), each function's
statements are aligned (test vs production) and every test-only region is
checked:

| Rule | Checks | Violation |
|------|--------|-----------|
| R1 | No statement or function exists only in the production build | FAIL |
| R2 | Every test-only region is a complete statement or block (never wraps or guards production code); no test-only declaration shadows a name used by production code in its scope | FAIL |
| R3 | No test-only `return`/`break`/`continue`; a test-only `goto` stays inside its own test-only block | FAIL |
| R4 | `goto`/label references of matching statements correspond one-to-one | FAIL |
| R5 | Test-only code calls only QS functions (C: `QS_*`; C++: `QP::QS::*`, `QP::QSpyId::*`) and the port's critical-section/memory-isolation functions, and assigns only QS objects, its own locals, or the port's critical-section status from the critical-section entry | REVIEW |

A dump of a failed compilation (`<<< error >>>`, or a missing object file) is
rejected as a tool error.

Configuration-specific names: the QP link-time configuration check
(`qp_cfgchk.h`/`qp_cfgchk.hpp`) names the kernel's initialization function
`QF_<QP_VERSION>_<settings>` (e.g. `QF_816U_a32t01p03e4q1m2s42x`), and the
encoded settings include `Q_SPY` and `Q_UTEST`. The name therefore differs
between the test and the production build by design, both where the kernel
defines the function and where the inline `QF_init()` (C++: `QF::init()`)
of the Framework header calls it. Before the comparison, every name matching
`QF_<digits>U_<lowercase letters and digits>` is replaced in both dumps by
one placeholder (`QF_CFGCHK_NAME`), so these names are compared as one name.
All other differences, including calls of such a name that exist in one
build only, are checked as usual (self-test cases `st32`, `st33`).

## C++ specifics
- The C++ front end prints some statements as `<<< Unknown tree: expr_stmt
  ... >>>` (over several lines) and wraps full expressions in
  `<<cleanup_point ... >>`. `qimpact.py` joins and removes these wrappers
  first (`void_cst` becomes a no-op, `(void) (x = y)` becomes `x = y`), then
  applies the same rules as for C. C dumps never contain the wrappers.
- A C++ dump also contains the **inline functions of the included headers**
  (`qp.hpp`, `qs.hpp`, the C++ standard library). They are checked like the
  module's own functions, because they are compiled into the module.
  Functions are identified by their full signature, so overloads and
  `const` variants are distinct. A standard-library template instantiated
  only by test-only code (e.g. `std::array<unsigned char, 9>::operator[]`)
  is a function that exists only in the test build (R5, REVIEW).
- The dump prints static data members **without their class** (`QP::QS::priv_`
  appears as `priv_`), so a test-only write to such a member cannot be
  attributed to QS and is reported as REVIEW (R5).

## Self-test (tool qualification)
`qimpact_selftest/` holds 33 seeded cases, each with its expected verdict
(`EXPECT: PASS|FAIL|REVIEW`): 24 C cases (`st01`..`st18`, `st28`..`st33`,
valid C and C++) and 9 C++ cases (`st19`..`st27`). With a C compiler the C
cases run; with a C++ compiler (`g++`) all 33 run (the C cases then
compiled as C++). The self-test is a separate **tool qualification** step,
not part of every test run:

```
qimpact.py --selftest COMPILER [WORKDIR]
make instr_selftest          (from any test's host directory)
```

`make instr_selftest` passes exactly the compiler and base flags of the unit
tests (`$(CC) $(CFLAGS_BASE)` or `$(CPP) $(CPPFLAGS_BASE)`) and keeps the
compiler outputs in `build/qimpact_selftest/`. It lists every case and exits
with 0 when all verdicts match, 3 otherwise (the tool must then not be used
with that compiler and those flags). Archive its output with your tool
qualification records.

Repeat the self-test whenever the tool, the compiler (version), or the base
compiler flags change: the dump format is internal to GCC, and a flag change
alone can change it (`-g -O` adds debug markers). To tie each check to a
qualified configuration, every report and the self-test record the version
and the SHA-256 checksum of `qimpact.py`, the compiler version line, the
compiler's target triple (`-dumpmachine`, e.g. `i686-w64-mingw32` or
`x86_64-linux-gnu`), and the base flags; those of each report must match
those of the archived self-test run.

The self-test cases are available only in `qtools/qutest/`, not in the
`pip` package.

| Case | Expected | Seeded property |
|------|----------|-----------------|
| st01 | PASS | additive trace records, QS-private data, test-only local |
| st02 | FAIL | decision condition depends on `Q_SPY` |
| st03 | FAIL | statement only in the production build |
| st04 | FAIL | test build wraps a production statement in a condition |
| st05 | FAIL | test-only early `return` |
| st06 | FAIL | test-only `break` out of a production loop |
| st07 | FAIL | function only in the production build |
| st08 | REVIEW | test-only assignment to production data |
| st09 | REVIEW | test-only call to a non-instrumentation function |
| st10 | FAIL | loop structure differs (`while` vs `do-while`) |
| st11 | PASS | identical builds |
| st12 | PASS | loop entirely inside instrumentation code |
| st13 | PASS | production dead `if (0)` blocks and casted no-ops |
| st14 | FAIL | production dead block that still contains a statement |
| st15 | PASS | trace blocks right after closing braces (alignment) |
| st16 | PASS | test-only declaration opens a bare scope (alignment) |
| st17 | FAIL | test-only declaration shadows a production variable |
| st18 | PASS | test-only declaration in a sibling scope (no shadowing) |
| st19 | PASS | C++ member function with trace records, critical sections, `QS::force_cast`, `QSpyId` |
| st20 | FAIL | C++: test build wraps the production call in a condition |
| st21 | FAIL | C++: test-only early `return` (Q_UTEST dummy-object pattern) |
| st22 | FAIL | C++: inline member function with a different signature (`const` only in production) |
| st23 | REVIEW | C++: test-only write to production data through `this` |
| st24 | REVIEW | C++: test-only write to a static data member (class not visible in the dump) |
| st25 | FAIL | C++: shift amount differs inside a full expression (wrapper scanner vs `>>`) |
| st26 | PASS | C++: class temporaries, templates, `operator[]` around a trace record |
| st27 | FAIL | C++: production passes a different argument (`sender` vs `nullptr`) |
| st28 | FAIL | jump target differs: test build `continue`s where production `break`s (R4) |
| st29 | FAIL | test-only `goto` leaves its test-only block to a production label (R3) |
| st30 | PASS | test-only `goto` and label inside the same test-only block |
| st31 | PASS | production code with its own label (C dump lists it as `void L = <<< error >>>;`) |
| st32 | PASS | configuration-specific name of the link-time configuration check (compared as one name) |
| st33 | FAIL | call of a configuration-specific name only in the production build |

Add a case whenever a new kind of difference is found.


# More Information
More information about the QUTest unit testing harness is available
online at:

- https://www.state-machine.com/qtools/qutest.html

More information about the QP/QSPY software tracing system is available
online at:

- https://www.state-machine.com/qtools/qpspy.html
