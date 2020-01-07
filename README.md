![QTools Collection](https://www.state-machine.com/img/qtools_banner.jpg)

# What's New?
View QTools Revision History at: 
https://www.state-machine.com/qtools/history.html


---------------------------------------------------------------------------
# Documentation
The offline HTML documentation for **this** particular version of QTools
is located in the folder html/. To view the offline documentation, open
the file html/index.html in your web browser.

The online HTML documention for the **latest** version of QTools is located
at: https://www.state-machine.com/qtools/


---------------------------------------------------------------------------
# About QTools
QTools is a collection of various open source tools for working with the
[QP Real-Time Embedded Frameworks (RTEFs)][QP] on desktop platforms, such
as Windows, Linux, and Mac OS X.

The following open-source tools are currently provided (NOTE: tools
starting with 'q' are contributed by Quantum Leaps)

1. qspy     - host application for receiving and displaying the
              real-time data from embedded targets running the QS
              software tracing.

2. qutest   - Tcl extension of the QSPY host application for **uint testing**
              specifically designed for embedded systems, but also supports
              unit testing of embedded code on host computers
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

8. Unity    - traditional unit testing harness (framework) for embedded C
              (version 2.4.3)

Additionally, QTools for Windows contains the following open-source,
3rd-party tools:

10. GNU-make for Windows (32-bit version 4.2.1)

11. GNU C/C++ toolset for Windows (mingw32 version 9.2.0)

12. GNU C/C++ toolset for ARM-EABI (GCC version 9.2.1)

13. Python for Windows (32-bit version 3.8.0 for x86)

14. Tcl/Tk for Windows (32-bit version 8.6) with UDP sockets extension

15. ResEdit utility (32-bit version 1.6.6-x64)

16. LMFlash for Windows (32-bit build 1613)


---------------------------------------------------------------------------
# Downloading and Installation
The most recommended way of obtaining QTools is by downloading the
[QP-bundle](https://www.state-machine.com/#Downloads), which includes QTools
and also all [QP frameworks](https://www.state-machine.com/products/) and
the [QM modeling tool](https://www.state-machine.com/qm/). The main advantage
of obtaining QTools bundled together like that is that you get all components,
tools and examples ready to go.

> NOTE: [QP-bundle](https://www.state-machine.com/#Downloads) is the most
recommended way of downloading and installing QTools. However,
if you are allergic to installers and GUIs or don't have administrator
privileges you can also **download and install QM separately**
as described below.

Alternatively, you can download QTools **separately** as described below:

--------------------
## QTools on Windows
On Windows, installation of QTools consists of unzipping the
`qtools-windows_<ver>.zip` archive into a directory of your choice,
although the recommended default is `C:\qp`.
 
After unzipping the archive, you need to add the following directories
to the PATH:

- <qp>\qtools\bin
- <qp>\qtools\mingw32\bin

Also, to use the [QUTest unit testing][QUTest] you need to
define the environment variable `QTOOLS` to point to the
installation directory of QTools.


--------------------
## QTools on Linux/MacOS
On Linux/MacOS, installation of QTools consists of unzipping the
`qtools-posix_<ver>.zip` archive into a directory of your choice,
although the recommended default is `~/qp`.

> NOTE: To use the QTools, you first need to **build** the tools on
your machine.

For example, to build the QSPY host application, you need to go to the
directory qtools/qspy/posix and type make to build the executable.
The provided Makefile will automatically copy the qspy executable to
the qtools/bin directory.

Simiarly, you need to build the QCLEAN and QFSGEN utilities.

> NOTE: To use the [QUTest unit testing][QUTest] you need to
define the environment variable `QTOOLS` to point to the
installation directory of QTools.


---------------------------------------------------------------------------
# Licensing
Most tools included in this collection are distributed under the terms
of the GNU General Public License (GPL) as published by the Free
Software Foundation, either version 2 of the License, or (at your
option) any later version. The text of GPL version 2 is included in the
file GPLv2.txt in the licenses/ subdirectory of the QTools distribution.

Some of the tools are distributed under the terms of the MIT open source
license. The complete text of the MIT license is included in the comments.

The Python package is distributed under the terms of the PYTHON LICENSE
AGREEMENT, included in the file PYTHON_LICENSE.txt in the licenses/
subdirectory of the QTools distribution.

The TCL/TK package is distributed under the terms of the TCL LICENSE
AGREEMENT, included in the file TCL_LICENSE.txt in the licenses/
subdirectory of the QTools distribution.

The LMFlash utility for Windows is is distributed under the terms of the
LMFlash license, included in the file LMFlash_LICENSE.rtf in the licenses/
subdirectory of the QTools distribution. Specifically, the LMFlash utility
is distributed according to Section 2a "Demonstration License".


---------------------------------------------------------------------------
# Documentation
The links to the documentation of the tools are provided in the
`<qtools>\doc\` directory.

### QUTest Python Scripting Support
The QUTest Python scripting support for the QSPY utility are located in the
`<qtools>\qspy\py\` directory.

### QUTest Tcl Scripting Support
The QUTest Tcl scripting support for the QSPY utility are located in the
`<qtools>\qspy\tcl\` directory.

A web-link to the TCL/TK documentation is provided in the shortcut
[tcl_tk_8.6](https://www.tcl.tk/man/tcl8.6/).

### QSPYView Tcl/Tk Scripts
The QSpyView Tcl/Tk scripts for the QSPY utility are located in the
`<qtools>\qspy\tcl\` directory.

### QSPY Matlab Scripts
The Matlab/GNU Octave scripts for the QSPY utility are located in the
`<qtools>\qspy\matlab\` directory.

### Unity Unit Testing Framework
The documentation of the Unity Testing Framework for Embedded C is
located in the `<qtools>\unity\doc\` directory.


---------------------------------------------------------------------------
# Source Code
In compliance with GPL, this distribution contains the source code for
the utilities contributed by Quantum Leaps in the <qtools>\source\
subdirectory, except for the QSPY source code, which is provided in the
<qtools>\qspy\source directory. All tools with names starting with 'q'
have been developed and are copyrighted by Quantum Leaps.

### The GCC C and C++ compilers for Windows
Have been taken from the MSYS2 project at
- https://www.msys2.org/

The installer mingw-get-setup.exe has been used and after the
installation, the files have been pruned to reduce the size of the
distribution. Please refer to the MinGW project for the source code.

### The GNU-ARM Embedded Toolchain for Windows
Have been takend from:
- https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads

The installer gcc-arm-none-eabi-8-2018-q4-major-win32-sha1.exe has been used.
(Version 8-2018-q4-major Released: December 20, 2018)

### The GNU make executable for Windows
Has been taken from the MinGW project at SourceForge.net:

### The file and diff utilities
Have been taken from the UnixUtils project at SourceForge.net:
- http://prdownloads.sourceforge.net/unxutils/UnxUtils.zip

### The file and diff utilities source
(source/fileutils-3.16-src.zip) has been taken from:
- http://prdownloads.sourceforge.net/unxutils/UnxUtilsSrc.zip

### The Unity Unit Testing Harness for Embedded C
Has been taken from the GitHub at:
- https://github.com/ThrowTheSwitch/Unity


---------------------------------------------------------------------------
# Contact information:
- https://www.state-machine.com
- info@state-machine.com

   [QP]: <https://www.state-machine.com/products/#QP>
   [QUTest]: <https://www.state-machine.com/qtools/qutest.html>
