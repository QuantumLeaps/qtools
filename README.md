![QP framework](https://state-machine.com/img/qtools_banner.jpg)

What's New?
===========
Scroll down a bit to the section "QTools Revision History".

About QTools
============
QTools is a collection of various open source tools for working with the
QP state machine frameworks on desktop platforms, such as Windows,
Linux, and Mac OS X.

The following open-source tools are currently provided by Quantum Leaps:

1. qspy     - host application for receiving and displaying the
              real-time data from embedded targets running the QS
              software tracing.

2. qutest   - Tcl extension of the QSPY host application for uint testing
              specifically designed for deeply embedded systems, but also
              supports unit testing of embedded code on host computers
              ("dual targeting").

3. qspyview - Tcl/Tk extension of the QSPY host application for
              visualization and monitoring of the QS real-time tracing
              data from embedded targets at real-time. QSpyView enables
              developers to rapidly build both GUI-based and "headless"
              scripts for their specific applications.

4. qwin     - QWIN GUI toolkit for prototyping embedded systems on
              Windows in the C programming language. QWIN allows you
              to build realistic embedded front panels consisting of
              LCD displays (both graphical and segmented), buttons,
              and LEDs. QWIN is based on the Win32 API.

5. qclean   - for cleanup of source code files

6. qfsgen   - for generating ROM-based file systems to be used
              in embedded web pages served by the HTTP server

7. qcalc    - programmer's calculator with C-syntax expressions

8. qudp     - for testing UDP connections to the embedded targets
9. qudps    - UDP server for testing UDP connections to the embedded
              targets.

Additionally, QTools for Windows contains the following open-source,
3rd-party tools:

10. GNU-make for Windows (version 3.82)

11. GNU C/C++ toolset for Windows (MinGW version 4.5.2)

12. GNU C/C++ toolset for ARM-EABI (GCC version 6.2.0)

13. Tcl/Tk for Windows (version 8.4) with UDP sockets extension

14. ResEdit utility (version 1.6.6-x64)

15. AVRDUDE for Windows (version 6.1)


Additionally, QTools for Windows contains the following PC-Lint option
files various compilers (in sub-directory lint):

15. co-gnu-arm.* (PC-Lint option files for GNU-ARM)

16. co-iar-arm.* (PC-Lint option files for IAR-ARM)


Installation
============
Installation of QTools is trivial and consists merely of unzipping the
qtools_<ver>.zip archive into a directory of your choice.

******
NOTE: To use the QTools, you should add the <qtools>\bin directory to
the PATH, where <qtools> denotes the directory where you have unzipped
the qtools_<ver>.zip archive.

NOTE: To use the QTools source code (such as parts of the QSPY) in
builds of the QP library, you need to define the environment variable
QTOOLS to point to the installation directory of QTools.
******

On Linux you must additionally make sure that the executables, such as
qspy, qclean, etc. have the 'executable' property set.


Licensing
=========
Most tools included in this collection are distributed under the terms
of the GNU General Public License (GPL) as published by the Free
Software Foundation, either version 2 of the License, or (at your
option) any later version. The text of GPL version 2 is included in the
file GPLv2.txt in the root directory of the QTools distribution.

The TCL/TK 8.4 package is distributed under the terms of the TCL LICENSE
AGREEMENT, included in the file TCL_LICENSE.txt in the root directory of
the QTools distribution.


Documentation
=============
The documentation of the tools is provided in the <qtools>\doc\
directory. Specifically, the PDF version of the make manual is included.
A we-link to the TCL/TK documentation is provided in the shortcut
tcl_tk_8.4.


QUTest Tcl Script
=================
The QUTest Tcl script for the QSPY utility are located in the
<qtools>\qspy\tcl\ directory.


QSPYView Tcl/Tk Scripts
=======================
The QSpyView Tcl/Tk scripts for the QSPY utility are located in the
<qtools>\qspy\tcl\ directory.


QSPY Matlab Scripts
===================
The Matlab/GNU Octave scripts for the QSPY utility are located in the
<qtools>\qspy\matlab\ directory.


QSPY Sequence Diagrams Examples
===============================
The sequence diagram input files for the MSCgen utility and the
generated sequence diagrams (in the SVG format) are located in the
<qtools>\qspy\mscgen\ directory.


Source Code
===========
In compliance with GPL, this distribution contains the source code for
the utilities contributed by Quantum Leaps in the <qtools>\source\
subdirectory, except for the QSPY source code, which is provided in the
<qtools>\qspy\source directory. All tools with names starting with 'q'
have been developed and are copyrighted by Quantum Leaps.

The GCC C and C++ compilers for Windows
---------------------------------------
Have been taken from the MinGW project at SourceForge.net:

https://sourceforge.net/projects/mingw/files/Installer/mingw-get-inst/

The installer mingw-get-inst-20110211.exe has been used and after the
installation, the files have been pruned to reduce the size of the
distribution. Please refer to the MinGW project for the source code.


The GNU-ARM (EABI) compilers for Windows
----------------------------------------
Have been takend from:

http://gnutoolchains.com/arm-eabi/

The installer arm-eabi-gcc6.2.0-r3.exe has been used and after the
installation, the libraries for Cortex-A and Cortex-R5 have been pruned
to reduce the size of the distribution. Please refer to the GnuToolchains
project for the source code.


The GNU make executable for Windows
-----------------------------------
Has been taken from the MinGW project at SourceForge.net:

https://sourceforge.net/projects/mingw/files/MinGW/make/
make-3.82-mingw32/make-3.82-5-mingw32-bin.tar.lzma

The GNU make source (make-3.82-src.zip) has been taken from:
https://sourceforge.net/projects/mingw/files/MinGW/make/
make-3.82-mingw32/make-3.82-5-mingw32-src.tar.lzma

The "GNU Make" manual (make.pdf) has been copied from the GNU make
project at:

http://www.gnu.org/software/make


The file and diff utilities
---------------------------
Have been taken from the UnixUtils project at SourceForge.net:

http://prdownloads.sourceforge.net/unxutils/UnxUtils.zip


The file and diff utilities source
----------------------------------
(source/fileutils-3.16-src.zip) has been taken from:

http://prdownloads.sourceforge.net/unxutils/UnxUtilsSrc.zip


The AVRDUDE utility
-------------------
Has been taken from:

http://download.savannah.gnu.org/releases/avrdude/

The Mscgen utility has been taken from the Mscgen project at:
http://www.mcternan.me.uk/mscgen/


Contact information:
====================
https://state-machine.com
mailto:info@state-machine.com


##############################################################################
######################## QTools Revision History #############################

QTools Version 5.9.3 (04-Jul-2017)
----------------------------------
Fixed bug#175 "QS_QF_ACTIVE_GET & QS_QF_EQUEUE_GET Record Mislabeled in
QSPY Output".

Added bin/qcalc.tcl to the GIT repository (so that it shows up on GitHub).


QTools Version 5.9.1 (04-Jun-2017)
----------------------------------
Added the GNU-ARM (EABI) toolset to the QTools Collection for Windows in the
directory qtools/gnu_arm-eabi. The addition of the GNU-ARM toolset matches the
changes made to Makefiles in QP/C/C++/nano 5.9.2.

To reduce the size of the QTools for Windows download, the self-extracting
archive qtools_win32_<ver>.exe has been prepared with the 7-Zip utility.


QTools Version 5.9.0 (19-May-2017)
----------------------------------
This release adds the QUTest (pronounced 'cutest') Unit Testing support
to QP/Spy software tracing. Specifically, this release adds a new head-
less (console-based) Tcl script tcl\qutest.tcl, which runs unit tests.

This release also adapts the QSPY host utility to support QUTEST unit
testing. Several new commands have been added and the structure of the
code has been expanded.

Also, the QSpyView Visualization extension has been moved to the
tcl\ sub-directory (the Tcl script tcl\qspyview.tcl).

The other Quantum Leaps utilities, like qclean, qfsgen, and qcalc have
been updated and greatly improved.

Finally, all utilities in the QTools collection have been documented in the
new QTools Manual available online at:

https://state-machine.com/qtools


QTools Version 5.7.0 (09-Sep-2016)
----------------------------------
Corrected QSPY software tracing host application to properly display
floating point numbers in user-defined trace records (QS_F32() and
QS_F64() macros). The problem was with incompatibility between Microsoft
VC++ and GCC floating-point format specifications. In the GCC software
build (which produces the QSPY executable in the qtools\bin directory),
the MS-VC++ floating point format resulted in all zeros
(e.g., 0.0000e+000).


QTools Version 5.6.5 (10-Jun-2016)
----------------------------------
Updated the QWIN GUI to use easier scaling and more efficient rendering
of graphical displays.


QTools Version 5.6.4 (04-May-2016)
----------------------------------
Added QWIN GUI to the collection (sub-directory qwin_gui).

Updated the QSPY software tracing host application for the QS trace
record name changes introduced in QP 5.6.2.


QTools Version 5.5.0 (21-Aug-2015)
----------------------------------
Extended the QSPY software tracing host application for bi-directional
communication with embedded targets (output and *input* into the
target). Added a UDP socket to QSPY, as an extensibility mechanism for
adding external GUIs and/or scripts to control the embedded targets.

Provided new QSpyView Tcl/Tk extension of the QSPY host application for
control testing, and visualization of the real-time tracing data from
embedded targets at real-time. QSpyView enables developers to rapidly
build both GUI-based and "headless" scripts for their specific
applications.


QTools Version 5.4.0 (27-Apr-2015)
----------------------------------
Added the avrdude utility to the Windows release. Modified the qclean
utility to use the Windows EOL convention for .txt files.


QTools Version 5.3.1 (21-Apr-2014)
----------------------------------
Corrected the version representation from hex to decimal, to match the
change in the QP framework. The version representation mismatch caused
problems in parsing newly modified trace records, when the qspy.c
implementation was inserted directly into the projects.


QTools Version 5.3.0 (31-Mar-2014)
----------------------------------
Added new trace records to QSPY host application: QS_QEP_TRAN_HIST,
QS_QEP_TRAN_EP, and QS_QEP_TRAN_XP. Changed labels for standard records
from Q_ENTRY, Q_EXIT, Q_INIT to ENTRY, EXIT, INIT.


QTools Version 5.1.1 (15-Oct-2013)
----------------------------------
Fixed the bug in the QSPY host application, which didn't handle
correctly object/functions/signal names longer than 32 characters. The
name limit has been raised to 64 characters and this version also
correctly truncates the names at the limit without printing any garbage
characters.


QTools Version 5.1.0b (01-Oct-2013)
-----------------------------------
Modified the stripped-down MinGW C++ compiler included in the QTools for
Windows distribution to correctly compile the std::cout stream from
<iostream>.


QTools Version 5.1.0a (18-Sep-2013)
-----------------------------------
Modified QSPY utility to support changes in QP 5.1.x:

-improved handling of target resets by adding an empty QS record
 before the QS_QP_RESET record. The empty record provides the frame
 delimiter in case the last record in incomplete, so that the
 following QS_QP_RESET record can be recognized.

-improved hanlding of internal object/function/signal dictionaries
 so that symbolic information is displayd for all occurrences of an
 object/function/signal, for which a dictionary record was received.


QTools Version 5.0.0a (08-Sep-2013)
-----------------------------------
Modified QSPY utility to support changes in QP 5.0.x. In particular:

-modified the standard trace records QS_QF_TICK, and QS_QF_TIMEEVT_*
 to contain the tick-rate number.

-added trace records QS_TEST_RUN and QS_TEST_FAIL for unit testing.

-added version compatibility level 5.0, whereas specifying version
 -v 4.5 runs qspy in the compatibility mode with QP 4.5.x.


QTools Version 4.5.06 (07-Apr-2013)
-----------------------------------
Added TCL/TK 8.4 to the QTools collection for Windows.

Fixed a bug in qspy.c for formatting of user-defined I32 elements
(used SPRINTF_AT() rahter then SPRINTF_LINE()).


QTools Version 4.5.05 (26-Mar-2013)
-----------------------------------
Modified QSPY source code (qspy.c) to use the standard PRIx64
macros defined in <inttypes.h> for portable formatting 64-bit
integers. This eliminates all format-related warnings generated
by the MinGW compiler.

Added a rudimentary <inttypes.h> header file to qspy\win32\vc
directory for compatibility with Microsoft Visual Studio.

Replaced the MinGW compiler included in the collection to smaller
version GCC 4.5.2.


QTools Version 4.5.04 (31-Jan-2013)
-----------------------------------
Modified the QSPY source code to consistently use the size_t type
in "hal.h" instead of "unsigned int", so that the QSPY utility
builds cleanly on 64-bit systems.

Modified the qtools\qspy\win32\mingw\make.bat file to use the
MinGW compiler now included in the QTools collection for Windows.


QTools Version 4.5.03 (26-Nov-2012)
-----------------------------------
Added the MinGW C/C++ compiler to the QTools collection for Windows.
Changed the location of executables from the root of QTools to
the bin\ subdirectory. This is done to accomodate the structure
of the MinGW compiler.


QTools Version 4.5.02 (21-Jul-2012)
-----------------------------------
Re-designed the QSPY interface to support more flexible parsing
of the trace records in desktop-based simulations (such as Windows
or Qt). Users can provide a custom parsing callback function to
QSPY_config(). Also added QS_RESET() macro to reset the internal
dictionaries (and other cleanup in the future) when the target
resets.


QTools Version 4.5.01 (25-Jun-2012)
-----------------------------------
Added the QS_USR_DICTIONARY() entry for storing dictionaries of
the user trace records. Replaced all remaining sprintf() calls
with snprintf().


QTools Version 4.5.00 (26-May-2012)
-----------------------------------
Re-designed the implementation of the QSPY host application, so
that it can be convenienty included as part of the QP library.
This allows direct QS tracing output to the screen for QP applications
running on the desktop. The QSPY application has been converted from
C++ to plain C for easier integration with QP/C.


QTools Version 4.3.00 (03-Nov-2011)
-----------------------------------
This QSPY version mataches the changes to the critical section
macros made in QP 4.3.00. The QS record names QS_QF_INT_LOCK and
QS_QF_INT_UNLOCK have been replaced with QS_QF_CRIT_ENTRY and
QS_QF_CRIT_EXIT, respectively.


QTools Version 4.2.04 (27-Sep-2011)
-----------------------------------
This QSPY version fixes the bug of incorrect reporting function
or object pointers for which the dictionary records are not
provided and which are repeated in one format line (bug #3405904).
For example, trace record AO.FIFO would report (incorrectly) as
follows:

0014004078 AO.FIFO: Sndr=200009B4 Obj=200009B4
Evt(Sig=00000009,Obj=200009B4, Pool= 0, Ref= 0)
Queue(nFree=  5, nMin=  5)

The Sndr= and Obj= are reported to be the same, but they were not.


QTools Version 4.2.01 (01-Aug-2011)
-----------------------------------
This QSPY version adds generation of sequence diagrams as
files to be processed by MscGen (www.mcternan.me.uk/mscgen/).
This version adds the option -g<msc_file> to generate .msc
file.

Also, this version of QSPY for Windows allows COM ports
larger than COM9.


QTools Version 4.2.00 (13-Jul-2012)
-----------------------------------
This QSPY version matches the changes made to QS target code in
QP/C/C++ 4.2.xx. These changes include sending the additinal byte
of sender priority in trace records:

- QS_QF_ACTIVE_POST_FIFO,
- QS_QF_ACTIVE_POST_LIFO,

Additional changes include sending the poolID and refCtr of events
in two bytes instead of just one byte. The changes affect the
following trace records:

- QS_QF_ACTIVE_POST_FIFO,

- QS_QF_ACTIVE_POST_LIFO,
- QS_QF_ACTIVE_GET,

- QS_QF_EQUEUE_GET,

- QS_QF_ACTIVE_GET_LAST,

- QS_QF_EQUEUE_GET_LAST,

- QS_QF_EQUEUE_POST_FIFO,

- QS_QF_EQUEUE_POST_LIFO,

- QS_QF_PUBLISH,

- QS_QF_GC_ATTEMPT, and

- QS_QF_GC.

Also, for compatibility with the QP 4.2.xx, this version changes
the defaults as follows:

signal size (-S) from 1 byte to 2 bytes, and

baud rate (-b) from 38400 to 115200

This version adds the following trace record:

QS_QF_TIMEEVT_CTR

The version also adds compatiblity with 64-bit targets (such
as 64-bit linux). This version can accept 8-byte pointers (both
object pointers and function pointers) as well as 64-bit
integers sent in user-defined trace records.

This version also adds the hex format for uint32_t integers
sent in the user-defined trace records.

Finally, this version adds command-line option -v to specify
the corresponding QP version running on the target. The default
version is -v4.2, but by specifying version -v4.1 or -v4.0 will
switch QSPY into the backwards-compatibility mode with the
earlier versions of QP.


QTools Version 4.1.06 (09-Feb-2011)
-----------------------------------
This is the intial standalone release of the QSPY host application.
QSPY is still available in the QP/C and QP/C++ distributions, but
other rapid prototyping platforms (such as mbed or Arduino) do not
use the standard QP distributions and have no easy access to the
QSPY tool. For these users, this pre-compiled standalone release
is more convenient.


