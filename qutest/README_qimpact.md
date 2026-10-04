# qimpact - QUTest Instrumentation-Impact Check

`qimpact.py` checks, as part of every host unit test, that the QP/Spy
instrumentation (`Q_SPY`, `Q_UTEST`) is purely **additive** for the module
under test: the code compiled for production must be the code compiled for
the test, minus complete, instrumentation-only statements. When the check
passes, the test results and the structural coverage measured on the
instrumented test build apply to the production build (`STP_QP`, section
"Instrumented Test Build vs. Production Build").

The same script works for SafeQP/C (`gcc`) and SafeQP/C++ (`g++`).

(The tool was called `qinstr` / `qp_instr_check` during development.)

## Files

| File | Location |
|------|----------|
| `qimpact.py` | `%QTOOLS%\qutest\` (next to `qutest.py`) |
| `qimpact_selftest\*.c, *.cpp` | `%QTOOLS%\qutest\qimpact_selftest\` (needed only for the self-test) |
| `gnu-host_c.mak` | `safe-qpc\tests\shared\` (calls the check for SafeQP/C) |
| `gnu-host_cpp.mak` | `safe-qpcpp\tests\shared\` (calls the check for SafeQP/C++) |

No per-test Makefile changes are needed. Requirements: Python 3.7+ (already
used by QUTest), the host GCC that builds the tests.

## How it works

The shared Makefile knows the module under test (`$(PROJECT).c` or
`$(PROJECT).cpp`, found through `VPATH`; the same module that gcov reports).
After the QUTest run and gcov, the `run` target:

1. compiles the module twice with exactly the same compiler (`$(CC)` or
   `$(CPP)`), base flags, include paths, and port:
   - test build: `$(CFLAGS)` / `$(CPPFLAGS)` (with `-DQ_SPY -DQ_UTEST`);
   - production build: `$(CFLAGS_PROD)` / `$(CPPFLAGS_PROD)`, i.e. the same
     without `$(IMP_DEFS)` (`-DQ_SPY -DQ_UTEST`);

   each time adding `-fdump-tree-original=<file>`, which writes GCC's own
   parse of every function (before optimization);
2. runs `qimpact.py`, which compares the two dumps and appends its report to
   `$(PROJECT).cov` (the console gets a one-line summary; the report itself
   is displayed once, by the existing `$(CAT) $(PROJECT).cov`).
   `run_host.bat` already copies `*.cov` into
   `tests/auto_run/TUN_QP_<module>-host.log`, so the report becomes part of
   the archived test log without any change to the batch file.

Both steps are the canned recipe `IMP_CHECK` in the shared Makefile.

### Files in the build directory

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
(`make clean` deletes `build/*.*`, i.e. not the `qimpact_selftest`
subdirectory.)

### Command line

The script takes only five positional arguments (no temporary files):

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
arguments (7 after the script name were observed).

Makefile targets:

| Target | Action |
|--------|--------|
| `run` (default) | QUTest run, gcov, then the check (report appended to `$(PROJECT).cov`) |
| `instr` | the check alone (report in `$(PROJECT).instr`, displayed at the end) |
| `instr_selftest` | the self-test (tool qualification, see below) |
| `show` | also lists `QIMPACT` and `IMP_SRC` |

The variable `QIMPACT` defaults to `python3 $(QTOOLS)/qutest/qimpact.py`
and can be overridden on the `make` command line.

The report lists every function with test-only code or a finding; functions
identical in both builds are only counted (`-v` lists all functions and
the test-only statements).

**For now the check only reports:** every step of `IMP_CHECK` is prefixed
with `-`, so neither a finding nor a tool error stops `make` or
`run_host.bat`. To make it blocking later, remove the `-` before
`$(QIMPACT)` in the `IMP_CHECK` recipe (exit codes: 0 PASS or N/A, 1 FAIL,
2 REVIEW, 3 tool error).

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

### C++ specifics

- The C++ front end prints some statements as `<<< Unknown tree: expr_stmt
  ... >>>` (over several lines) and wraps full expressions in
  `<<cleanup_point ... >>`. `qimpact.py` joins and removes these wrappers
  first (`void_cst` becomes a no-op, `(void) (x = y)` becomes `x = y`), then
  applies the same rules as for C. C dumps never contain the wrappers and
  are processed exactly as before (verified: identical results on all
  SafeQP/C modules and C cases).
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

`qimpact_selftest/` holds 31 seeded cases, each with its expected verdict
(`EXPECT: PASS|FAIL|REVIEW`): 22 C cases (`st01`..`st18`, `st28`..`st31`,
valid C and C++) and 9 C++ cases (`st19`..`st27`). With a C compiler the C
cases run; with a C++ compiler (`g++`) all 31 run (the C cases then
compiled as C++). The
self-test is a separate **tool qualification** step, not part of every
test run:

```
qimpact.py --selftest COMPILER [WORKDIR]
make instr_selftest          (from any test's host directory)
```

`make instr_selftest` passes exactly the compiler and base flags of the unit
tests (`$(CC) $(CFLAGS_BASE)` or `$(CPP) $(CPPFLAGS_BASE)`) and keeps the
compiler outputs in `build/qimpact_selftest/`. It lists every case and exits
with 0 when all verdicts match, 3 otherwise (the tool must then not be used
with that compiler and those flags). Archive its output with the tool
qualification (`TQR_QP`), once for SafeQP/C and once for SafeQP/C++.
For the Certification Kit, the runs are made with
`doc\CERT\TQR_evidence\qimpact\qimpact_qualify.bat`, which calls
`make instr_selftest` in `TUN_QP_qep_hsm\test\host` of both editions with
`BIN_DIR` redirected next to the script (nothing is written into safe-qpc or
safe-qpcpp) and stores the outputs there with a time stamp.

Repeat the self-test whenever the tool, the compiler (version), or the base
compiler flags change: the dump format is internal to GCC, and a flag change
alone can change it (`-g -O` adds debug markers). To tie each check to a
qualified configuration, every report records the SHA-256 of `qimpact.py`,
the compiler version line, the compiler's target triple (`-dumpmachine`,
e.g. `i686-w64-mingw32` or `x86_64-linux-gnu`), and the base flags; these
must match those of the archived self-test run.

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

The self-test has caught three defects during development (a
`while`/`do-while` change, two alignment artifacts), one compiler-flag
dependence (`-g -O` debug markers in the dumps), and, for C++, a false
assignment found inside a brace initializer (`{.m_prio=...}`); in 1.5.2 the
cases written for the rules not yet covered (R4, R3 `goto`) found that a
label of the source code caused a false tool error (C dump) and that a
test-only `goto` to such a label was not checked. All are fixed and kept as
regression cases. Add a case whenever a new kind of difference is
found.

## Expected verdicts (host, current sources)

Obtained with `make instr` in every host test directory, through the shared
Makefiles, with GCC/G++ 11.4.0 on Linux (target `x86_64-linux-gnu`,
posix-qutest port). The self-test
passed with the same compiler and base flags (18/18 C, 27/27 C++). These
results document the plumbing and the current findings; the qualified
configuration is the one listed in `TQR_QP` (GCC 15.2.0 from the QP-bundle,
MinGW on Windows), with which the self-test must be repeated and archived. A
different compiler (version or target) is a different configuration and
needs its own self-test run.

SafeQP/C:

| Test | Module | Verdict | Finding |
|------|--------|---------|---------|
| TUN_QP_qep_hsm, _qep_msm, _qf_act, _qf_defer, _qf_mem, _qf_qact32/64, _qf_qeq, _qf_qmact | same | PASS | |
| TUN_QP_qutest | qutest.c | N/A | whole module is test support (no production code) |
| TUN_QP_qf_dyn | qf_dyn.c | FAIL | `QMPool_get`/`QMPool_put` get `qsId` 0 (production) vs `poolNum + 64` (test) |
| TUN_QP_qf_ps | qf_ps.c | FAIL | `QActive_post_` gets `sender` 0 (production) vs `sender` (test) |
| TUN_QP_qf_time | qf_time.c | FAIL | same `sender` difference in `QTimeEvt_tick_`; REVIEW: `++prev->ctr` only under `Q_SPY` |
| TUN_QP_qk, TIN_QP_qk | qk.c | FAIL | `init`/`dispatch` virtual calls get `qsId` 0 (production) vs `prio`/`p` (test) in `QActive_start` and `QK_activate_` |
| TUN_QP_qv, TIN_QP_qv | qv.c | FAIL | same `qsId` difference in `QActive_start` and `QF_run` |
| TUN_QP_qf_actq | qf_actq.c | ERROR | production variant does not compile with the QUTest port (`QS_tstPriv_` used by `qp_port.h`; `Q_UTEST` exception) |

SafeQP/C++:

| Test | Module | Verdict | Finding |
|------|--------|---------|---------|
| TUN_QP_qep_msm, _qf_act, _qf_defer, _qf_mem, _qf_qact32/64, _qf_qeq, _qf_qmact, _qk, _qv, TIN_QP_qk, TIN_QP_qv | same | PASS | |
| TUN_QP_qutest | qutest.cpp | N/A | whole module is test support |
| TUN_QP_qep_hsm | qep_hsm.cpp | REVIEW | `QHsm::init`: test-only `priv_.flags = priv_.flags \| 1` (static member, class not visible in the dump) |
| TUN_QP_qf_dyn | qf_dyn.cpp | FAIL | `QMPool::get`/`put` `qsId` difference (as in C); REVIEW: `poolInit` writes `obj_name`, and the `std::array<unsigned char, 9>` helpers it instantiates exist only in the test build |
| TUN_QP_qf_ps | qf_ps.cpp | FAIL | `QActive::post_` `sender` difference (as in C) |
| TUN_QP_qf_time | qf_time.cpp | FAIL | `sender` difference in `QTimeEvt::tick` (as in C) |
| TUN_QP_qf_actq | qf_actq.cpp | ERROR | production variant does not compile with the QUTest port (as in C) |

Compared with the earlier results: the `PROJECT` settings of the host
Makefiles are now correct (each test checks its own module), and the
SafeQP/C++ `qp.hpp` finding (`qm_entry()`/`qm_exit()` `const` only in
production) and the `QHsm::tran_complex_` finding no longer occur. (The SafeQP/C++
`TUN_QP_qk` host Makefile, whose `INCLUDES` lacked `-I` before
`$(QP_PORT_DIR)`, has been fixed.)

## Changes

- 1.5.2: labels of the source code: the C dump's label declaration
  `void L = <<< error >>>;` is no longer taken for a compilation error, and
  R3/R4 also check `goto`s to such labels (before, only compiler labels);
  compiler comments (`// predicted ...`) are not scanned for jumps; 4 new
  self-test cases (st28..st31) cover R4 and the R3 `goto` rule.
- 1.5.1: every report and the self-test record the compiler's target triple
  (`COMPILER -dumpmachine`), since the version line does not identify the
  target and the base flags no longer select it; `--help` examples without
  `-m32`.
- 1.5.0: SafeQP/C++ support (`g++`, `gnu-host_cpp.mak`, 9 C++ self-test
  cases); a missing object file is a tool error (failed compilation);
  depth-aware detection of assignments (R5); compact report (identical
  functions only counted); the self-test keeps its compiler outputs in a
  given directory (`make instr_selftest`: `build/qimpact_selftest/`);
  "impact" replaces "harmlessness" in all texts; tool renamed from `qinstr`
  to `qimpact` (`qimpact.py`, `qimpact_selftest/`, Makefile variables
  `QIMPACT`, `IMP_*`; the Makefile targets `instr` and `instr_selftest`
  keep their names).
- 1.4.0: the self-test is no longer run on every check; it is a separate
  qualification step (`--selftest`, `make instr_selftest`). Normal runs do not
  need the self-test cases. Reports record the SHA-256 of the tool.
- 1.3.0: compact positional command line (no argument file).
