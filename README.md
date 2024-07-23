<p align="center">
<a href="https://www.state-machine.com/products/qtools" title="QTools collection">
<img src="https://www.state-machine.com/img/qtools_banner.jpg"/>
</a>
</p>

# What's New?
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/QuantumLeaps/qtools)](https://github.com/QuantumLeaps/qtools/releases/latest)

View QTools Revision History at:
https://www.state-machine.com/qtools/history.html


# Documentation
The offline HTML documentation for **this** particular version of QTools
is located in the folder html/. To view the offline documentation, open
the file html/index.html in your web browser.

The online HTML documention for the **latest** version of QTools is located
at: https://www.state-machine.com/qtools/


# About QTools
QTools is a collection of various open source tools for working with the
[QP Real-Time Embedded Frameworks (RTEFs)][QP] on desktop platforms, such
as Windows, Linux, and macOS.

The following open-source tools are currently provided (NOTE: tools
starting with 'q' are contributed by Quantum Leaps)

1. [qspy](https://www.state-machine.com/qtools/qspy.html) -
   host application for receiving and displaying the real-time data from
   embedded targets running the QS software tracing.

2. [qutest](https://www.state-machine.com/qtools/qutest.html) -
   Python extension of the `qspy` host application for
   **unit and integration testing** specifically designed for embedded systems,
   but also supports unit testing of embedded code on host computers
   ("dual targeting").

3. [qview](https://www.state-machine.com/qtools/qview.html) -
   Python extension of the `qspy` host application for visualization and
   monitoring of the QS real-time tracing data from embedded targets at
   real-time. QView enables developers to quickly build both GUI-based and
   "headless" scripts for their specific applications.

4. [qwin](https://www.state-machine.com/qtools/qwin.html) -
   QWIN GUI toolkit for prototyping embedded systems on Windows in the
   C programming language. QWIN enables developers to build realistic embedded
   front panels consisting of LCD displays (both graphical and segmented),
   buttons, and LEDs. QWIN is based on the Win32 API.

5. [qcalc](https://www.state-machine.com/qtools/qcalc.html) -
   programmer's calculator specifically designed for embedded systems programmers.

6. [qclean](https://www.state-machine.com/qtools/qclean.html) -
   for *fast* cleanup of white space (tabs, trailing spaces, end-of-line)
   in source code files

7. [qfsgen](https://www.state-machine.com/qtools/qfsgen.html) -
   for generating ROM-based file systems to be used in embedded web pages
   served by the HTTP server

8. Unity - traditional unit testing harness (framework) for embedded C
   (version 2.5.2)

Additionally, QTools for Windows contains the following open-source,
3rd-party tools:

9. `make` for Windows (GNU-make-32-bit version 4.2.1)

10. `cmake` for Windows (version 3.29.0-rc1)

11. `ninja` for Windows (version 1.11.1)

12. Termite serial terminal for Windows (version 3.4)

13. LMFlash for Windows (32-bit build 1613)


Additionally, the QTools directory in the QP-bundle contains the
following 3rd-party tools:

14. GNU C/C++ toolset for Windows (MinGW 32-bit version 9.2.0)

15. GNU C/C++ toolset for ARM-EABI (GCC version 10.3-2021.10)

16. Python for Windows (version 3.10 32-bit)


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


## QTools on Windows
On Windows, installation of QTools consists of unzipping the
`qtools-windows_<ver>.zip` archive into a directory of your choice,
although the recommended default is `C:\qp`.

After unzipping the archive, you need to add the following directories
to the PATH (`<qp>` stands for the directory, where you installed qp):
- `<qp>\qtools\bin`
- `<qp>\qtools\mingw32\bin`

> NOTE: To use the [QUTest unit testing][QUTest] you need to define the
environment variable `QTOOLS` to point to the installation directory
of QTools.


## QTools on Linux/macOS
On Linux/MacOS, installation of QTools consists of unzipping the
`qtools-posix_<ver>.zip` archive into a directory of your choice,
although the recommended default is `~/qp`.

After unzipping the archive, you need to add the following directories
to the PATH (`<qp>` stands for the directory, where you installed qp):
- `<qp>/qtools/bin`

> NOTE: To use the [QUTest unit testing][QUTest] you need to define the
environment variable `QTOOLS` to point to the installation directory
of QTools.


# Licensing
The various Licenses for distributed components are located in the
LICENSES/ sub-directory of this QTools distribution.

- The [qspy host utility](https://www.state-machine.com/qtools/qspy.html)
is distributed under the terms of QSPY LICENSE AGREEMENT, included in the file
`LICENSE-qspy.txt` in the `LICENSES/` sub-directory.

- The [Termite host utility for Windows](https://www.compuphase.com/software_termite.htm)
is is distributed under the terms of the Termite license, included in the file
`LICENSE-Termite.txt` in the `LICENSES/` sub-directory.

- The [LMFlash host utility for Windows](https://www.ti.com/tool/LMFLASHPROGRAMMER)
is is distributed under the terms of the LMFlash license, included in the file
`LICENSE-LMFlash.txt` in the `LICENSES/` sub-directory. Specifically, the LMFlash
utility is distributed according to Section 2a "Demonstration License".

- The [Python package for Windows](https://www.python.org/) is distributed
under the terms of the PYTHON LICENSE AGREEMENT, included in the file
`LICENSE-Python.txt` in the `LICENSES/` sub-directory.

Most other tools included in this collection are distributed under the
terms of the GNU General Public License (GPL) as published by the Free
Software Foundation, either version 2 of the License, or (at your
option) any later version. The text of GPL version 2 is included in the
file GPL-2.0-or-later.txt in the `LICENSES/` sub-directory.

Some other the tools are distributed under the terms of the MIT open
source license. The complete text of the MIT license is included in the
comments and also in the file LICENSE-MIT.txt in the `LICENSES/`
sub-directory.


# Source Code
In compliance with GPL, this distribution contains the source code for
the utilities contributed by Quantum Leaps in the `<qtools>\source`
subdirectory, except for the QSPY source code, which is provided in the
`<qtools>\qspy\source` directory. All tools with names starting with 'q'
have been developed and are copyrighted by Quantum Leaps.

### The GCC C and C++ compilers for Windows
Have been taken from the MSYS2 project at
- https://www.msys2.org/

The installer mingw-get-setup.exe has been used and after the installation,
the files have been pruned to reduce the size of the distribution.
Please refer to the MinGW project for the source code.

### The GNU-ARM Embedded Toolchain for Windows
Have been taken from:
- https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads

The installer gcc-arm-none-eabi-8-2018-q4-major-win32-sha1.exe has been used.
(Version 8-2018-q4-major Released: December 20, 2018)

### The GNU make executable for Windows
Has been taken from the MinGW project at SourceForge.net:

### The UNIX file and directory utilities
Have been taken from the Gow (Gnu On Windows) project at GitHub:
- https://github.com/bmatzelle/gow

### The Unity Unit Testing Harness for Embedded C
Has been taken from the GitHub at:
- https://github.com/ThrowTheSwitch/Unity


# How to Help this Project?
If you like this project, please give it a star (in the upper-right corner
of your browser window):

<p align="center"><img src="https://www.state-machine.com/img/github-star.jpg"/></p>

# Contact information:
- https://www.state-machine.com
- info@state-machine.com

   [QP]: <https://www.state-machine.com/products/#QP>
   [QUTest]: <https://www.state-machine.com/qtools/qutest.html>
