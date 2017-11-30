![QTools Collection](https://state-machine.com/img/qtools_banner.jpg)

# What's New?
View QTools Revision History at: 
https://state-machine.com/qtools/history.html

---------------------------------------------------------------------------
# About QTools
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


---------------------------------------------------------------------------
# Installation
Installation of QTools is trivial and consists merely of unzipping the
qtools_<ver>.zip archive into a directory of your choice.

> NOTE: To use the QTools, you should add the <qtools>\bin directory to
the PATH, where <qtools> denotes the directory where you have unzipped
the qtools_<ver>.zip archive.

> NOTE: To use the QTools source code (such as parts of the QSPY) in
builds of the QP library, you need to define the environment variable
QTOOLS to point to the installation directory of QTools.

On Linux you must additionally make sure that the executables, such as
qspy, qclean, etc. have the 'executable' property set.


---------------------------------------------------------------------------
# Licensing
Most tools included in this collection are distributed under the terms
of the GNU General Public License (GPL) as published by the Free
Software Foundation, either version 2 of the License, or (at your
option) any later version. The text of GPL version 2 is included in the
file GPLv2.txt in the root directory of the QTools distribution.

The TCL/TK 8.4 package is distributed under the terms of the TCL LICENSE
AGREEMENT, included in the file TCL_LICENSE.txt in the root directory of
the QTools distribution.


---------------------------------------------------------------------------
# Documentation
The documentation of the tools is provided in the <qtools>\doc\
directory. Specifically, the PDF version of the make manual is included.
A we-link to the TCL/TK documentation is provided in the shortcut
tcl_tk_8.4.

### QUTest Tcl Script
The QUTest Tcl script for the QSPY utility are located in the
`<qtools>\qspy\tcl\` directory.

### QSPYView Tcl/Tk Scripts
The QSpyView Tcl/Tk scripts for the QSPY utility are located in the
<qtools>\qspy\tcl\ directory.

### QSPY Matlab Scripts
The Matlab/GNU Octave scripts for the QSPY utility are located in the
<qtools>\qspy\matlab\ directory.


---------------------------------------------------------------------------
# QSPY Sequence Diagrams Examples
The sequence diagram input files for the MSCgen utility and the
generated sequence diagrams (in the SVG format) are located in the
<qtools>\qspy\mscgen\ directory.


---------------------------------------------------------------------------
# Source Code
In compliance with GPL, this distribution contains the source code for
the utilities contributed by Quantum Leaps in the <qtools>\source\
subdirectory, except for the QSPY source code, which is provided in the
<qtools>\qspy\source directory. All tools with names starting with 'q'
have been developed and are copyrighted by Quantum Leaps.

### The GCC C and C++ compilers for Windows
Have been taken from the MinGW project at SourceForge.net:
- https://sourceforge.net/projects/mingw/files/Installer/mingw-get-inst/

The installer mingw-get-inst-20110211.exe has been used and after the
installation, the files have been pruned to reduce the size of the
distribution. Please refer to the MinGW project for the source code.

### The GNU-ARM (EABI) compilers for Windows
Have been takend from:
- http://gnutoolchains.com/arm-eabi/

The installer arm-eabi-gcc6.2.0-r3.exe has been used and after the
installation, the libraries for Cortex-A and Cortex-R5 have been pruned
to reduce the size of the distribution. Please refer to the GnuToolchains
project for the source code.

### The GNU make executable for Windows
Has been taken from the MinGW project at SourceForge.net:

- https://sourceforge.net/projects/mingw/files/MinGW/make/
make-3.82-mingw32/make-3.82-5-mingw32-bin.tar.lzma

The GNU make source (make-3.82-src.zip) has been taken from:
- https://sourceforge.net/projects/mingw/files/MinGW/make/
make-3.82-mingw32/make-3.82-5-mingw32-src.tar.lzma

The "GNU Make" manual (make.pdf) has been copied from the GNU make
project at:
- http://www.gnu.org/software/make


### The file and diff utilities
Have been taken from the UnixUtils project at SourceForge.net:
- http://prdownloads.sourceforge.net/unxutils/UnxUtils.zip

### The file and diff utilities source
(source/fileutils-3.16-src.zip) has been taken from:
- http://prdownloads.sourceforge.net/unxutils/UnxUtilsSrc.zip


### The AVRDUDE utility
Has been taken from:
- http://download.savannah.gnu.org/releases/avrdude/

### The Mscgen utility has been taken from the Mscgen project at:
- http://www.mcternan.me.uk/mscgen/


---------------------------------------------------------------------------
# Contact information:
- https://state-machine.com
- mailto:info@state-machine.com

