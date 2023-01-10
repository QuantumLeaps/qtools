![QTools Collection](https://www.state-machine.com/img/qtools_banner.jpg)

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

1. qspy     - host application for receiving and displaying the
              real-time data from embedded targets running the QS
              software tracing.

2. qutest   - Python extension of the QSPY host application for **uint testing**
              specifically designed for embedded systems, but also supports
              unit testing of embedded code on host computers
              ("dual targeting").

3. qview    - Python extension of the QSPY host application for
              visualization and monitoring of the QS real-time tracing
              data from embedded targets at real-time. QView enables
              developers to quickly build both GUI-based and "headless"
              scripts for their specific applications.

4. qwin     - QWIN GUI toolkit for prototyping embedded systems on
              Windows in the C programming language. QWIN allows you
              to build realistic embedded front panels consisting of
              LCD displays (both graphical and segmented), buttons,
              and LEDs. QWIN is based on the Win32 API.

5. qclean   - for cleanup of source code files

6. qfsgen   - for generating ROM-based file systems to be used
              in embedded web pages served by the HTTP server

7. Unity    - traditional unit testing harness (framework) for embedded C
              (version 2.5.2)

Additionally, QTools for Windows contains the following open-source,
3rd-party tools:

8. GNU-make for Windows (32-bit version 4.2.1)

9. LMFlash for Windows (32-bit build 1613)


Additionally, the QTools directory in the QP-bundle contains the
following 3rd-party tools:

10. GNU C/C++ toolset for Windows (MinGW 32-bit version 9.2.0)

11. GNU C/C++ toolset for ARM-EABI (GCC version 10.3-2021.10)

12. Python for Windows (version 3.10 32-bit)


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
to the PATH:

- <qp>\qtools\bin
- <qp>\qtools\mingw32\bin

Also, to use the [QUTest unit testing][QUTest] you need to
define the environment variable `QTOOLS` to point to the
installation directory of QTools.


## QTools on Linux/macOS
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


# Licensing
The various Licenses for distributed components are located in the
LICENSES/ sub-directory of this QTools distribution.

Most tools included in this collection are distributed under the terms
of the GNU General Public License (GPL) as published by the Free
Software Foundation, either version 2 of the License, or (at your
option) any later version. The text of GPL version 2 is included in the
file GPL-2.0-or-later.txt in the LICENSES/ sub-directory.

Some of the tools are distributed under the terms of the MIT open source
license. The complete text of the MIT license is included in the comments
and also in the file LICENSE-MIT.txt in the LICENSES/ sub-directory.


The Python package is distributed under the terms of the PYTHON LICENSE
AGREEMENT, included in the file LICENSE-Python.txt in the LICENSES/
sub-directory.

The LMFlash utility for Windows is is distributed under the terms of the
LMFlash license, included in the file LICENSE-LMFlash.txt in the LICENSES/
sub-directory. Specifically, the LMFlash utility is distributed according
to Section 2a "Demonstration License".


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
Have been takend from:
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
If you like this project, please give it a star (in the upper-right corner of your browser window):

![GitHub star](doxygen/images/github-star.jpg)


# Contact information:
- https://www.state-machine.com
- info@state-machine.com

   [QP]: <https://www.state-machine.com/products/#QP>
   [QUTest]: <https://www.state-machine.com/qtools/qutest.html>
