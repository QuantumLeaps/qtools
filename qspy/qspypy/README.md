About this Directory
====================
This directory contains the Python package "qspypy" that supports 
writing Python test scripts for the QUTest unit testing harness.

**NOTE:**
"qspypy" has been originally contributed by Dominic Valentino from
Lotus Engineering LLC (https://lotusengineeringllc.weebly.com/).
The package is licensed under the MIT open source license.


General Requirements
====================
The "qspypy" package requires Python 3.6.x or higher, which is
freely available from the Internet.


Installing "qpspypy"
====================
`qspypy` is a part of the [QTools collection](https://www.state-machine.com/qtools/start.html#qtools_qspypy).

Once QTools is installed on your disk, and the `QTOOLS` environment
variable is defined to point to the intallation directory of QTools,
you need to add the `qspypy` package to Python:

- on Windows you type:

`pip install %QTOOLS%\qspy`

- on Linux you type:

`pip install $QTOOLS/qspy`


Using "qpspypy"
===============
To launch any of the scripts in this directory, you need to first run
the QSPY console application (version 6.x or higher) with the -u option
(the -u option opens up the UDP socket for attaching "front-ends").
Once QSPY is running, you can "attach" to the UDP socket and start
communicating with the QSPY back-end or to the Target (through QSPY).

The usage of the "qpspypy" is as follows:

`python -m qspypy.qutest [test_scripts] [target_exe] [host_ip]`

where:
`[test_scripts]` - optional specification of the Python test scripts to run.
                 If not specified, the scripts are chosen by the general
                 rules of the PyTest framework.

`[target_exe]`   - optional specification of the target executable to
                 launch for testing embedded code on the host computer.
                 If not specified, an embedded target is assumed.

`[host_ip]`      - optional IP-address:port for the host running the QSpy
                 utility. If not specified, the default is localhost:7701.

Examples:
```
python -m qspypy.qutest
python -m qspypy.qutest *.py
python -m qspypy.qutest *.py mingw\dpp.exe
python -m qspypy.qutest *.py mingw\dpp.exe 192.168.1.100:7705
```

More Information
================
More information about the QP/QSPY software tracing system is available
online at:

https://www.state-machine.com/qtools/qpspy.html

More information about the QUTest unit testing harness is available
online at:

https://www.state-machine.com/qtools/qutest.html

