/**
\page rev_page_qspy QSPY Revision History

\section qspy_5_3_1 Version 5.3.1 Release date: Apr 21, 2014

Corrected the version representation from hex to decimal, to match the change in the QP framework. The version representation missmatch caused problems in parsing newly modified trace records, when the qspy.c implementation was inserted directly into the projects. 


<HR>
\section qspy_5_3_0 Version 5.3.0 Release date: Mar 31, 2014

Added new trace records to QSPY host application: QS_QEP_TRAN_HIST, QS_QEP_TRAN_EP, and QS_QEP_TRAN_XP. Changed labels for standard records from Q_ENTRY, Q_EXIT, Q_INIT to ENTRY, EXIT, INIT. 


<HR>
\section qspy_5_1_1 Version 5.1.1 Release date: Oct 15, 2013

Fixed the bug in the QSPY host application, which didn't handle
correctly object/functions/signal names longer than 32 characters. The
name limit has been raised to 64 characters and this version also
correctly truncates the names at the limit without printing any garbage
characters.


<HR>
\section qspy_5_1_0a Version 5.1.0a Release date: Sep 18, 2013

Modified QSPY utility to support changes in QP 5.1.x:

-improved handling of target resets by adding an empty QS record
 before the QS_QP_RESET record. The empty record provides the frame
 delimiter in case the last record in incomplete, so that the
 following QS_QP_RESET record can be recognized. 

-improved hanlding of internal object/function/signal dictionaries
 so that symbolic information is displayd for all occurrences of an
 object/function/signal, for which a dictionary record was received.  


<HR>
\section qspy_5_0_0a Version 5.0.0a Release date: Sep 08, 2013

Modified QSPY utility to support changes in QP 5.0.x:

-modified the standard trace records QS_QF_TICK, and QS_QF_TIMEEVT_*
 to contain the tick-rate number.

-added trace records QS_TEST_RUN and QS_TEST_FAIL for unit testing.

-added version compatibility level 5.0, whereas specifying version
 -v 4.5 runs qspy in the compatibility mode with QP 4.5.x. 

-added Find And Replace Text (FART) utility for Windows 


<HR>
\section qspy_4_5_02 Version 4.5.02 Release date: Jul 21, 2012

Re-designed the QSPY interface to support more flexible parsing
of the trace records in desktop-based simulations (such as Windows
or Qt). Users can provide a custom parsing callback function to
QSPY_config(). Also added QS_RESET() macro to reset the internal
dictionaries (and other cleanup in the future) when the target
resets.


<HR>
\section qspy_4_5_01 Version 4.5.01 Release date: Jun 25, 2012

Added the QS_USR_DICTIONARY() entry for storing dictionaries of
the user trace records. Replaced all remaining sprintf() calls
with snprintf().


<HR>
\section qspy_4_5_00 Version 4.5.00 Release date: May 26, 2012

Re-designed the implementation of the QSPY host application, so
that it can be convenienty included as part of the QP library.
This allows direct QS tracing output to the screen for QP applications
running on the desktop. The QSPY application has been converted from
C++ to plain C for easier integration with QP/C.


<HR>
\section qspy_4_3_00 Version 4.3.00 Release date: Nov 03, 2011

This QSPY version mataches the changes to the critical section
macros made in QP 4.3.00. The QS record names QS_QF_INT_LOCK and
QS_QF_INT_UNLOCK have been replaced with QS_QF_CRIT_ENTRY and
QS_QF_CRIT_EXIT, respectively.


<HR>
\section qspy_4_2_04 Version 4.2.04 Release date: Sep 27, 2011

This QSPY version fixes the bug of incorrect reporting function
or object pointers for which the dictionary records are not
provided and which are repeated in one format line (bug #3405904).
For example, trace record AO.FIFO would report (incorrectly) as
follows:

0014004078 AO.FIFO: Sndr=200009B4 Obj=200009B4
Evt(Sig=00000009,Obj=200009B4, Pool= 0, Ref= 0)
Queue(nFree=  5, nMin=  5)

The Sndr= and Obj= are reported to be the same, but they were not.


<HR>
\section qspy_4_2_01 Version 4.2.01 Release date: Aug 01, 2011

This QSPY version adds generation of sequence diagrams as
files to be processed by MscGen (www.mcternan.me.uk/mscgen/).
This version adds the option -g<msc_file> to generate .msc
file.

Also, this version of QSPY for Windows allows COM ports 
larger than COM9. 


<HR>
\section qspy_4_2_00 Version 4.2.00 Release date: Jul 13, 2011

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


<HR>
\section qspy_4_1_06 Version 4.1.06 Release date: Feb 09, 2011

This is the intial standalone release of the QSPY host application.
QSPY is still available in the QP/C and QP/C++ distributions, but
other rapid prototyping platforms (such as mbed or Arduino) do not
use the standard QP distributions and have no easy access to the
QSPY tool. For these users, this pre-compiled standalone release
is more convenient.

*/
