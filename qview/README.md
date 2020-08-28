![](https://www.state-machine.com/img/qview_banner.jpg)

The "qview" Python package is a powerful Visualization and Monitoring
facility, which allows embedded developers to create virtual Graphical
User Interfaces in Python to monitor and control their embedded devices
from a host (desktop) computer. The interfaces created by QView can
visualize the data produced by
[QP/Spy software tracing system](file:///C:/qp_lab/qtools/html/qpspy.html)
and can also interact with the embedded target by sending various commands.


General Requirements
====================
The "qview" package requires Python 3 with the
[tkinter](https://docs.python.org/3/library/tkinter.html) package, which
is included in the [QTools distribution](https://www.state-machine.com/qtools)
for Windows and is typically included with other operating systems, such as
Linux and MacOS.

To run "qview" in Python, you need to first launch the
[QSPY console application](https://www.state-machine.com/qtools/qspy.html)
to communicate with the chosen embedded target (or the host executable
if you are simulating your embedded target).

Once QSPY is running, from a separate terminal you can launch `qview.py`
and "attach" to the [QSPY UDP socket](https://www.state-machine.com/qtools/qspy_udp.html).
After this communication has been established, "qview" can interact with the
instrumented target and receive data from it (through QSPY).

**NOTE** The embedded C or C++ code running inside the target needs to be
built with the [QP/Spy software tracing system](file:///C:/qp_lab/qtools/html/qpspy.html)
instrumentation enabled. This is acheived by building the "Spy" build configuration.


![](https://www.state-machine.com/img/qview_targ.gif)


Installation
============
The `qview.py` script can be used standalone, **without** any
installation (see Using "qview" below).

Alternatively, you can **install** `qview.py` with `pip` from PyPi by
executing the following command:


`pip install qview`


Or directly from the sources directory (e.g., `/qp/qtools/qview`):


`python setup.py install --install-dir=/qp/qtools/qview`


Using "qview"
==============
If you are using `qview` as a standalone Python script, you invoke
it as follows:

`python /path-to-qview-script/qview.py [<cust_script> [<qspy_host> [<qspy_port>]]]`

Alternatively, if you've installed `qview` with `pip`, you invoke
it as follows:

`qview [cust_script] [qspy_host[:udp_port]] [local_port]`


Command-line Options
--------------------
- `cust_script` - optional customization Python scripts for your specific
target sytem. If not specified, qview will provide only the generic facilities
for interacting with the target (e.g., reset, setting QS filters,
posting events, etc.)

- `qspy_host[:udp_port]` - optional host-name/IP-address:port for the host
running the QSPY host utility. If not specified, the default
is 'localhost:7701'.

- `local_port` - optional the local UDP port to be used by "qview". If not
specified, the default is '0', which means that the operating sytem will
choose an open port.


Examples (for Windows):
-----------------------
`python %QTOOLS%\qview\qview.py`

opens the generic (not customized) "qview".

`python %QTOOLS%\qview\qview.py dpp.py`

opens "qview" with the customization provided in the `dpp.py` script
located in the current directory.

`qview ..\qview\dpp.py localhost:7701`

opens "qview" (installed with `pip`) with the customization provided in the
`dpp.py` script located in the directory `..\qview`.  The "qview" will
attach to the QSPY utility running at `localhost:7701`.

`qview dpp.py 192.168.1.100:7705`

opens "qview" (installed with `pip`) with the customization provided in the
`dpp.py` script located in the current directory. The "qview" will attach to
the QSPY utility running remotely at IP address `192.168.1.100:7705`.


Examples (for Linux/macOS):
---------------------------
`python $(QTOOLS)/qview/qview.py`

opens the generic (not customized) "qview".

`python $(QTOOLS)/qview/qview.py dpp.py`

opens "qview" with the customization provided in the `dpp.py` script
located in the current directory.

`qview *.py ../qview/dpp.py localhost:7701`

opens "qview" (installed with `pip`) with the customization provided in the
`dpp.py` script located in the directory `../qview`.  The "qview" will
attach to the QSPY utility running at `localhost:7701`.

`qview dpp.py 192.168.1.100:7705`

opens "qview" (installed with `pip`) with the customization provided in the
`dpp.py` script located in the current directory. The "qview" will attach to
the QSPY utility running remotely at IP address `192.168.1.100:7705`.


More Information
================
More information about the QView Visualization and Monitoring is available
online at:

- https://www.state-machine.com/qtools/qview.html

More information about the QP/QSPY software tracing system is available
online at:

- https://www.state-machine.com/qtools/qpspy.html


