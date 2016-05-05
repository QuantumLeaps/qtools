/*! @mainpage Getting Started

@tableofcontents

@section about About QSPY

As shown in the figure below, any @ref concepts "software tracing system" consist of the target-resident component and a host-resident component. @ref qspy "QSPY" is the host-resident component of the @ref tracing "Q-SPY software tracing system" built into the <a href="http://www.state-machine.com/qpc" target="_blank" class="extern">QP/C</a> and <a href="http://www.state-machine.com/qpcpp" target="_blank" class="extern">QP/C++</a> active object frameworks. QSPY is a console application available for Windows and Linux hosts. The job of QSPY is to receive the trace data produced by an instrumented QP/C or QP/C++ application running on an embedded Target, visualize the data, and optionally to save the data in various formats (such as MATLAB or MscGen). Also, starting with @ref qspy_5_5_0 "version 5.5.0", QSPY can <strong>send</strong> commands to the Target for testing and control (<strong>bi-directional</strong> QSPY).

@image html logo_qspy.gif

QSPY is easily adaptable to various target-host communication links. Out of the box, the QSPY host application supports serial (RS232), TCP/IP, and file communication links. Adding other communication links is straightforward, because the data link is accessed only through a generic Platform Abstraction Layer (PAL).

@image html qspy1.gif "Setup for QS/QSPY software tracing"

The QSPY application accepts several @ref qspy_command "command-line parameters" to configure the data link and all target dependencies, such as pointer sizes, signal sizes, etc. This means that the single QSPY host application can process data from any embedded target. The application has been tested with wide range of 8-, 16-, 32-, and 64-bit Targets.

@note
The QS-QSPY software tracing system is a very valuable addition to the QP frameworks, because it provides **visibility** into the live embedded system without significantly degrading the application itself. It enables developers to execute unit and integration tests and helps them to fine-tune their applications for the best long-time performance. Users of QP have also adapted QSPY to support all stages of product life cycle, from manufacturing to in-field diagnostics.


@subsection about_udp QSpyView Front-End
Starting with QSPY @ref qspy_5_5_0 "version 5.5.0", the QSPY application offers a <strong>UDP socket</strong> for attaching various "Front-Ends", such as GUI-based or head-less scripts to control and test the embedded Target. As an example of a GUI-based extensions, QSPY comes with a Tcl/Tk Front-End called @ref qspyview "QSpyView".

@image html qspy2.gif "Host-resident components: QSPY Back-End and QSpyView Front-End"

As you can see in the figure above, the QSPY console application (QSPY Back-End) opens a UDP socket, which forwards **all** trace data from the Target to the QSpyView "Front-End", so the Front-End has access to all this information. Additionally, the Front-End can send commands to QSPY and, ultimately, to the Target. Currently, the following commands are supported:

- Set global QS filters inside the Target
- Set local QS filters inside the Target
- Inject an arbitrary event to the Target (direct post or publish)
- Execute a user-defined callback function inside the Target with arguments supplied from QSPY
- Peek data inside the Target and send to QSPY
- Poke data (supplied from QSPY) into the Target
- Execute clock tick inside the Target
- Request target information (version, all sizes of objects, build time-stamp)
- Remotely reset of the Target


------------------------------------------------------------------------------
@section install Downloading and Installing QSPY

QSPY is included as a component inside the **Qtools** collection, which is available for download from <a class="extern" target="_blank" href="http://sourceforge.net/projects/qpc/files/Qtools">SourceForge.net</a>. The Qtools download is provided as a platform-independent ZIP or as a Windows EXE. (The EXE installer is recommended, because it is digitally signed by Quantum Leaps and provides an uninstaller). Uninstalling Qtools is easy and requires only deleting the `qtools` directory from your disk.

@image html qtools_install.png "Qtools installer on Windows (contains QSPY)"

@note
It is recommended to install Qtools into a directory that has <strong>no spaces or special characters</strong> in the name (e.g., "Program Files" or "Program Files (x86)" are poor choices).

<!-- <div class="separate"></div> -->
@subsection path Setting PATH
To use the Qtools, it is highly recommended to add the `<qtools>\bin` directory to the global `PATH` on your system, where `<qtools>` denotes the directory where you have unzipped the `qtools_<ver>.zip` archive.

@note
Adding `<qtools>\bin` to your `PATH` will enable you to invoke QSPY by simply typing `qspy` at the command prompt, regardless of your current directory. This will come handy, because typically you will not invoke `qspy` from its installation directory.


<!-- <div class="separate"></div> -->
@subsection env Setting QTOOLS Environment Variable
To use the Qtools source code (such as parts of the QSPY) in builds of the QP applications, it is highly recommended to define the environment variable `QTOOLS` to point to the installation directory of Qtools.


------------------------------------------------------------------------------
@section qspy_files Directories and Files

The following annotated directory tree in the standard <strong>Qtools</strong> distribution lists the top-level directories and files pertaining to QSPY:

<ul class="tag">
  <li><span class="img folder">qtools</span>
  </li>
  <ul class="tag">
    <li><span class="img folder">bin</span> &mdash; binaries (executables and libraries)
    </li>
    <ul class="tag">
      <li><span class="img file">qspy.exe</span> &mdash; QSPY executable
      </li>
      <li><span class="img file_tcl">tclsh.exe</span> &mdash; Tcl interpreter
      </li>
      <li><span class="img file_wish">wish.exe</span> &mdash; Tk interpreter
      </li>
      <li><span class="img file">mscgen.exe</span> &mdash; MscGen utility
      </li>
      <li>. . .
      </li>
    </ul>
    <li><span class="img folder">qspy</span> &mdash; @ref qspy "QSPY tool"
    </li>
    <ul class="tag">
      <li><span class="img folder">include</span> &mdash; QSPY includes
      </li>
      <li><span class="img folder">source</span> &mdash; QSPY sources
      </li>
      <li><span class="img folder">linux</span> &mdash; QSPY port to Linux
      </li>
      <li><span class="img folder">win32</span> &mdash; QSPY port to Windows
      </li>
      <li><span class="img folder">tcludp</span> &mdash; UDP extension to standard Tcl (needed for Linux)
      </li>
      <li><span class="img folder">qspyview</span> &mdash; @ref qspyview "QSpyView" sources
      </li>
      <ul class="tag">
        <li><span class="img folder">img</span> &mdash; images used by `QSpyView`
        </li>
        <li><span class="img file_tcl">qspy.tcl</span> &mdash; Tcl script for UDP communication with the QSPY Back-End (usable for both GUI-based and head-less scripts)
        </li>
        <li><span class="img file_wish">qspyview.tcl</span> &mdash; QSpyView Tcl/Tk script
        </li>
        <li><span class="img file_wish">default.tcl</span> &mdash; the default customization of the QSpyView Tcl/Tk script
        </li>
      </ul>
      <li><span class="img folder">matlab</span> &mdash; @ref qspy_matlab "QSPY MATLAB Support"
      </li>
      <ul class="tag">
        <li><span class="img file">qspy.m</span> &mdash; MATLAB script to analyze QSPY output
        </li>
        <li><span class="img file">dpp.m</span> &mdash; MATLAB script to analyze DPP example
        </li>
      </ul>
      <li><span class="img folder">mscgen</span> &mdash; @ref qspy_mscgen "QSPY MscGen Support"
      </li>
      <ul class="tag">
        <li><span class="img file">dpp.msc</span> &mdash; MscGen output from DPP example
        </li>
        <li><span class="img file">dpp.svg</span> &mdash; Message Sequence Chart generated by MscGen
        </li>
      </ul>
    </ul>
  </ul>
</ul>


------------------------------------------------------------------------------
@section example Example of a Software Tracing Session
To show you how software tracing works in practice this section presents an example of a software tracing session (on a Windows machine). This example uses the Dining Philosophers Problem (DPP) test application running on the <a href="http://www.state-machine.com/qpc/arm-cm_dpp_ek-tm4c123gxl.html" target="_blank" class="extern">EK-TM4C123GXL board</a> (TivaC LaunchPad), located in the QP/C and QP/C++ downloads (version 5.5.0 or newer). Specifically, the example is located in the directory `<qpx>\examples\arm-cm\dpp_ek-tm4c123gxl`, where `<qpx>` stands either for QP/C or QP/C++ installation directory on your system.


<div class="separate"></div>
@subsection exa_board Building and Loading QSPY Configuration to the Board
The first step is to prepare the embedded Target (the TivaC LaunchPad in this case) to run the <em>instrumented</em> application code. For that, you need to build the <strong>QSPY build configuration</strong>, because only in this configuration the instrumentation inside the QP framework and your application becomes <em>active</em>.

The `dpp_ek-tm4c123gxl` example is available with the cooperative QV kernel and the preemptive QK kernel. Each version is also available for the ARM-Keil toolset, GNU-ARM toolset, and IAR toolset. All of them provide the QSPY build configuration, but for the sake of this discussion, let's select the QK kernel and GNU-ARM toolset, located in the sub-directory `<qpx>\examples\arm-cm\dpp_ek-tm4c123gxl\qk\gnu`. At the command prompt, change to this directory (`cd c:\qp\qpc\examples\arm-cm\dpp_ek-tm4c123gxl\qk\gnu`) and type:

@verbatim
make CONF=spy
@endverbatim

Next, connect the TivaC LaunchPad to the USB port on your PC and program the produced image into the board by means of the `flash.bat` script. (This script assumes that you have installed the `LmFlash.exe` utility from Texas Instruments.)

@verbatim
flash spy
@endverbatim

After you reset the board (by pressing the Reset button), the board should start blinking the 3-color LED and it should also start producing Q-SPY data to the virtual COM port.

@note
The `dpp_ek-tm4c123gxl` example is also available for other toolsets, such as <a href="http://www.keil.com/arm" target="_blank" class="extern">ARM-Keil</a> and <a href="https://www.iar.com/iar-embedded-workbench/arm/" target="_blank" class="extern">IAR</a>. The DPP example comes with project files that can be opened in the IDEs provided with those tools. These IDEs are also used to load the code into the TivaC board.


<div class="separate"></div>
@subsection exa_qspy Launching QSPY Host Application
To view the Q-SPY data produced by the TivaC Target, you can't use a generic serial terminal, because the data is in @ref qspy_protocol "binary format". You need to use the special host-resident console application called @ref qspy "QSPY".

Assuming that you have added the `qtools\bin` directory to your `PATH`, you can run QSPY from any directory by simply typing `qspy` at the command prompt. However, you should choose the current directory carefully, because `qspy` will produce all @ref qspy_files "output files" into the directory from which it was launched.

@image html qspy_screen.gif "QSPY running in a Windows Command Prompt"

As you can see in the screen-shot above, `qspy` has been launched with the @ref qspy_command "command-line parameters" `-u -cCOM4`. The `-u` parameter means that `qspy` should open the UDP socket for attaching various "Front-Ends". The `-cCOM4` option instructs `qspy` to open the specified COM port for the communication with the Target. (<strong>NOTE:</strong> you should adjust the actual COM port number to the virtual COM port assigned to your TivaC board. The COM port is visible in the Windows Device Manager).

@note
The @ref qspy_text "human-readable output" in the screen-shot above shows hexadecimal addresses of various objects and numeric values of event signals. However, it is also possible to greatly enhance the readability of the output by providing the <strong>symbolic information</strong> about the object names to QSPY. This is accomplished when QSPY received @ref qspy_dict "QSPY Dictionaries" that are produced by the Target just after reset.

Regarding all other command-line QSPY options, you can obtain a <strong>quick summary of options</strong> by providing `-h` command line option, or by pressing the `h` when QSPY is running, as shown in the screen-shot below. The section @ref qspy_command for description of all options.

@image html qspy_help.gif "Quick summary of QSPY options and keyboard shortcuts"


Besides displaying the tracing data in human-readable format, the QSPY console application can accomplish the following tasks:

- @ref qspy_files "save data to files" in various other formats:
    - raw binary (the exact copy of the data received from the Target in @ref qspy_protocol "Q-SPY Protocol")
    - human-readable format (the text screen output)
    - @ref qspy_dict "save Dictionary records"
    - @ref qspy_matlab "MATLAB-compatible format"
    - @ref qspy_mscgen "MscGen-compatible format"
- send Reset command to the Target (the `r` key shortcut)
- send Info request to the Target (the `i` key shortcut)
- send Tick[0] command to the Target (the `t` key shortcut)
- send Tick[1] command to the Target (the `u` key shortcut)


<div class="separate"></div>
@subsection exa_qspyview Attaching QSpyView Front-End
Being just a console application, QSPY is limited in how it can interact with the User. However, starting with @ref qspy_5_5_0 "version 5.5.0", QSPY provides an extension mechanism for attaching various "Front-Ends", which might provide a GUI, or might be "head-less" to drive automated testing, for example.

The "Front-Ends" can interact with QSPY though a <strong>UDP socket</strong> that QSPY opens when it receives the <strong>`-u` command-line option</strong>. (The default port number for the UDP socket is `7701`, but this port number can be overridden by providing an optional value to the `-u` option. For example `-u8803` will open a UDP socket at port number `8803`).

The first available "Front-End" for QSPY is called @ref qspyview "QSpyView" and is written in <a href="https://en.wikipedia.org/wiki/Tcl" target="_blank" class="extern">Tcl/Tk</a>. QSpyView is itself extensible in order to support visualization and control of <em>specific</em> QP applications. For instance, the DPP application requires different views and control than a "Fly'n'Shoot" game.

To support such customizations, QSpyView is broken up into modules (separate Tcl scripts). The directory @ref qspy_files "qtools/qspy/qspyview/" contains the generic scripts for UDP communication with the QSPY "Back-End" (qspy.tcl), and a generic GUI (qspyview.tcl).

The script qspyview.tcl can be further customized by providing a command-line parameter in the invocation of the script, such as **dpp.tcl** for the DPP application. This latter script is co-located with the DPP example, which makes the much more sense than cluttering the @ref qspy_files "qtools/qspy/qspyview/" directory.

In the end, the QSpyView "Front-End", customized for the DPP application, can be very easily and conveniently launched by means of the **qspyview shortcut** located in the @ref qspy_files "qtools/qspy/qspyview/". The following screen shot shows how to adjust the shortcut in Windows.

@image html qspyview_shortcut.gif "qspyview shortcut properties"

@note
The **wish** interpreter is included in the Qtools collection for Windows. The shortcut assumes also that the `QTOOLS` environment variable is defined.

@image html qspyview.gif "qspyview.tcl with the dpp.tcl extension"

As shown in the screen shot above, the QSpyView "Front-End" provides in this case a customized views for the DPP example application running on the TivaC LaunchPad.

@note
The customizable **Canvas** view of QSpyView can be used to provide an external **Control Panel** for your embedded device. The Canvas can display data in attractive graphical form, as gauges, counter, and graphs, and it also can provide input to the Target, in form of buttons, knobs, and sliders.


@image html qspyview_menu.gif "QSpyView Commands menu"


The @ref qspyview "QSpyView Front-End" can perform many more actions in the Target, such as:
- Set global QS filters inside the Target
- Set local QS filters inside the Target
- Inject an arbitrary event to the Target (direct post or publish)
- Execute a user-defined callback function inside the Target with arguments supplied from QSPY
- Peek data inside the Target and send to QSPY
- Poke data (supplied from QSPY) into the Target
- Execute clock tick inside the Target
- Request target information (version, all sizes of objects, build time-stamp)
- Remotely reset of the Target


------------------------------------------------------------------------------
@section support Help and Support

Please post any **technical questions** to the <a class="extern" target="_blank" href="http://sourceforge.net/p/qpc/discussion/668726"><strong>Free Support Forum</strong></a> hosted on SourceForge.net. Posts to this forum benefit the whole community and are typically answered the same day.

Direct **Commercial Support** is available to the commercial licensees. Every commercial license includes one year of Technical Support for the licensed software. The support term can be extended annually.

Training and consulting services are also available from Quantum Leaps. Please refer to the <a class="extern" target="_blank" href="http://www.state-machine.com/support/">Support web-page</a> for more information.


------------------------------------------------------------------------------
@section qspy_licensing Licensing QS/QSPY

The QS target-resident component is part of the QP/C and QP/C++ active object frameworks and is <a class="extern" target="_blank" href="http://www.state-machine.com/licensing">licensed the same way as QP</a>.

The QSPY/QSpyView host applications are licensed under the GPL open source license.

@image html logo_ql_TM.jpg
Copyright &copy; 2002-2015 Quantum Leaps, LLC. All Rights Reserved.@n
http://www.state-machine.com


@next{concepts}
*/
