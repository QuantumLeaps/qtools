/*! @page concepts Concepts and Structure

@tableofcontents

<p>In any real-life project, getting the code written, compiled, and successfully linked is only the first step. The system still needs to be tested, validated, and tuned for best performance and resource consumption. A single-step debugger is frequently not helpful because it stops the system and exactly hinders seeing **live interactions** within the application. Clogging up high-performance code with `printf` statements is usually too intrusive and simply unworkable in most embedded systems, which typically don't have adequate screens to print to. So the questions are: How can you monitor the behavior of a running real-time system without degrading the system itself? How can you discover and document elusive, intermittent bugs that are caused by subtle interactions among concurrent components? How do you design and execute repeatable unit and integration <strong>tests</strong> of your system? How do you ensure that a system runs reliably for long periods of time and achieves optimal performance?
</p>
Techniques based on **software tracing** can answer many of these questions. Software tracing is a method for obtaining diagnostic information in a live environment without the need to stop the application to get the system feedback. Software tracing always involves some form of a target system instrumentation to log interesting discrete events for subsequent retrieval from the system and analysis.

@note
Due to the inversion of a control, software tracing is particularly effective and powerful in combination with the event-driven programming model, such as the <a class="extern" target="_blank" href="http://www.state-machine.com/qp">QP active object frameworks</a>. An instrumented event-driven framework can provide much more comprehensive and detailed information than any traditional RTOS.


-------------------------------------------------------------------------------
@section qspy_concepts Software Tracing Concepts
In a nutshell, software tracing is similar to peppering the code with `printf` statements for logging and debugging, except that software tracing is much less intrusive and more selective than the primitive `printf`. This quick overview introduces the basic concepts and describes some features you can expect from a commercial-grade software tracing system.

@image html qspy4.gif "Typical setup for collecting software trace data"

The picture above shows a typical setup for software tracing. The embedded Target system is executing instrumented code, which logs the trace data into a RAM buffer inside the Target. From that buffer the trace data is sent over a data link to a Host computer, which stores, displays, and analyzes the information. This configuration means that a software tracing always requires two components: a <em>Target-resident component</em> for collecting and sending the trace data, and a <em>Host-resident component</em> to receive, decompress, visualize, and analyze the data.

@note
Software tracing instrumentation logs interesting discrete events that occur in the target system. These discrete events will be called <strong>trace records</strong>, to avoid confusing them with the application-level <em>events</em>.

A good tracing solution is minimally intrusive, which means that it can provide visibility into the running code with minimal impact on the target system behavior. Properly implemented and used, it will let you diagnose a live system without interrupting or significantly altering the behavior of the system under investigation.

Of course, it's always possible that the overhead of software tracing, no matter how small, will have some effect on the target system behavior, which is known as the <em>probe effect</em> (a.k.a. the "Heisenberg effect"). To help you determine whether that is occurring, you must be able to configure the instrumentation in and out both at compile-time as well as at run-time.

To minimize the "probe effect", a good trace system performs efficient, selective logging of trace records using as little processing and memory resources of the target as possible. Selective logging means that the tracing system provides user-definable, fine granularity filters so that the target-resident component only collects events of interest—you can filter as many or as few instrumented events as you need. That way you can make the tracing as noninvasive as necessary.

To minimize the RAM usage, the target-resident trace component typically uses a circular trace buffer that is continuously updated, and new data overwrites the old when the buffer "wraps around" due to limited size or transmission rate to the host. This reflects the typically applied <em>last-is-best</em> policy in collecting the trace data. In order to focus on certain periods of time, software trace provides configurable software triggers that can start and stop trace collection before the new data overwrites the old data of interest in the circular buffer.

To further maximize the amount of data collected in the trace buffer, the Target-resident component typically applies some form of <em>data compression</em> to squeeze more trace information into the buffer and to minimize the bandwidth required to uplink the data to the Host.

However, perhaps the most important characteristic of a flexible software tracing system is the separation of trace logging (<em>what</em> is being traced) from the data transmission mechanism (<em>how</em> and <em>when</em> exactly the data is sent to the Host). This separation of concerns allows the transmissions to occur in the least time-critical paths of the code, such as the <em>idle loop</em>. Also, clients should be able to employ any data transmission mechanism available on the Target, meaning both the physical transport layer (e.g., serial port, SPI, USB, Ethernet, etc.) as well as implementation strategy (polling, interrupt, DMA, etc.). The tracing facility should tolerate and be able to detect any RAM buffer overruns due to bursts of tracing data production rate or insufficient transmission rate to the host.

Finally, the tracing facility must allow consolidating data from all parts of the system, including concurrently executing threads and interrupts. This means that the instrumentation facilities must be <em>reentrant</em> (i.e., both thread-safe and interrupt-safe). Also, to be able to correlate all this data, most tracing systems provide precise <em>time-stamping</em> of the trace records.



-------------------------------------------------------------------------------
@section qspy_system Q-SPY Software Tracing System

As mentioned in the introduction, software tracing is especially effective and powerful in combination with the event-driven active object frameworks, such as QP/C or QP/C++. A running application built of active objects is a highly structured affair where all important system interactions funnel through the active object framework and the state-machine engine. This offers a unique opportunity to instrument these relatively small parts of the overall code to gain unprecedented insight into the entire system. For example, the Q-SPY trace data is thorough enough to produce complete sequence diagrams and detailed state machine activity for all state machines in the system. You can selectively monitor all event exchanges, event queues, event pools, and time events because all these elements are controlled by the framework. Additionally, if you use one of the kernels built into QP (the cooperative QV kernel or the preemptive QK kernel), you can obtain all the data available to a traditional RTOS as well, such as context switches and mutex activity.

Q-Spy is a software tracing system that enables a live monitoring of event-driven QP applications with minimal target system resources and without stopping or significantly slowing down the code. The Quantum Spy system consists of the target-resident component, called <a href="http://www.state-machine.com/qpc/group__qs.html" target="_blank" class="extern">QS</a>, and the host-resident component running on a host workstation, called @ref qspy "QSPY".

<div class="separate"></div>
@subsection qspy_target QS Target Component
The target-resident component of the Q-Spy tracing system is called QS. The QS target component consists of the QS ring buffer, the QS filters, as well as the instrumentation added to QEP, QF, QK/QV, and the application, as shown in figure below. Additionally, starting from QP 5.5.0, the QS target component contains the receive-channel (QS-RX), which can receive data from the QSPY host component.

@anchor fig_struct
@image html qspy5.gif "Structure of the Q-SPY software tracing system"


The main difference between the Q-SPY tracing system and peppering the code with `printf` statements is where the data <strong>formatting</strong> and <strong>sending</strong> is done. When you use `printf`s, the data formatting and sending occur in the time-critical paths through the Target code. In contrast, the QS Target-resident component inserts unformatted binary data into the QS ring buffer, so all the time-consuming formatting is removed from the Target system and is done <em>after</em> the fact in the Host. Additionally, in QS, data logging and sending to the Host are separated so that the target system can typically perform the transmission outside of the time-critical path, for example in the <em>idle processing</em> of the target CPU.

A nice byproduct of removing the data formatting from the Target is a natural <strong>data compression</strong>. For example, formatted output of a single byte takes two hexadecimal digits(and 3 decimal digits), so avoiding the formatting gives at least a factor of two in data density. On top of this natural compression, QS uses such techniques as data dictionaries, and compressed format information, which in practice result in compression factor of 4-5 compared to the expanded human-readable format.


<div class="separate"></div>
@subsection qs_tline Target Time-Stamps
Most QS trace records produced by the Target are <strong>time-stamped</strong> with high-resolution. QS provides an efficient API for obtaining platform-specific timestamp information. Given the right timer-counter resource in you Target system, you can provide QS with as precise timestamp information as required. The size of the timestamp is configurable to be 1, 2, or 4 bytes.


-------------------------------------------------------------------------------
@section qspy_filters Q-SPY Filters
Obviously, QS cannot eliminate completely the overhead of software tracing. But with the fine-granularity <strong>filters</strong> available in QS, you can make this impact as small as necessary. For greatest flexibility, QS uses two complementary levels of filters: **Global Filters** and **Local Filters** described below. Combination of such two complementary filtering criteria results in very selective tracing capabilities.

@note
The Global and Local filters are initialized (hard-coded) in the Target. Subsequently, if the QS receive channel is enabled (QS-RX), the filters can be changed from the @ref qspyview "QSpyView Front-End" at **runtime**.


<div class="separate"></div>
@subsection qspy_global Global Filters
The Global Filters are based on trace record type. This filter allows you to disable or enable each individual trace record type, such as entry to a state, exit from a state, state transition, event posting, event publishing, time event expiration, and over 60 other standard and application-specific event types (see ::QSpyRecords). This level works globally for all state machines and event publications in the entire system.

@sa @ref qspyview_global

<div class="separate"></div>
@subsection qspy_local Local Filters
 The Local Filters are component-specific. You can setup a filter to trace only a specific state machine object, only a specific active object, only a specific time event, etc. The following table summarizes the specific objects you can filter on:

@sa @ref qspyview_local

Object@n Type | Example | Applies to QS Records
--------------|---------|----------------------
State@n Machine | `QS_FILTER_SM_OBJ`@n `(&l_qhsmTst);` | ::QS_QEP_STATE_ENTRY,@n ::QS_QEP_STATE_EXIT,@n ::QS_QEP_STATE_INIT,@n ::QS_QEP_INIT_TRAN,@n ::QS_QEP_INTERN_TRAN,@n ::QS_QEP_TRAN,@n ::QS_QEP_IGNORED,@n ::QS_QEP_TRAN_HIST,@n ::QS_QEP_TRAN_EP,@n ::QS_QEP_TRAN_XP
Active@n Object | `QS_FILTER_AO_OBJ`@n `(&l_philo[3])` | ::QS_QF_ACTIVE_ADD,@n ::QS_QF_ACTIVE_REMOVE,@n ::QS_QF_ACTIVE_SUBSCRIBE,@n ::QS_QF_ACTIVE_UNSUBSCRIBE,@n ::QS_QF_ACTIVE_POST_FIFO, @n ::QS_QF_ACTIVE_POST_LIFO,@n ::QS_QF_ACTIVE_GET, @n ::QS_QF_ACTIVE_GET_LAST
Memory@n Pool | `QS_FILTER_MP_OBJ`@n `(l_medPoolSto);` | ::QS_QF_MPOOL_INIT,@n ::QS_QF_MPOOL_GET,@n ::QS_QF_MPOOL_PUT
Event@n Queue   | `QS_FILTER_EQ_OBJ`@n `(l_philQueueSto[0]);` | ::QS_QF_EQUEUE_INIT,@n ::QS_QF_EQUEUE_POST_FIFO,@n ::QS_QF_EQUEUE_POST_LIFO,@n ::QS_QF_EQUEUE_GET,@n ::QS_QF_EQUEUE_GET_LAST
Time@n Event | `QS_FILTER_TE_OBJ`@n `(&l_philo[0].timeEvt);`| ::QS_QF_TICK,@n ::QS_QF_TIMEEVT_ARM,@n ::QS_QF_TIMEEVT_AUTO_DISARM,@n ::QS_QF_TIMEEVT_DISARM_ATTEMPT,@n ::QS_QF_TIMEEVT_DISARM,@n ::QS_QF_TIMEEVT_REARM,@n ::QS_QF_TIMEEVT_POST
Generic@n Application@n Object | `QS_FILTER_AP_OBJ`@n `(&myAppObj);` | Application-specific records@n starting with@n ::QS_USER


-------------------------------------------------------------------------------
@section qspy_protocol Q-SPY Data Protocol
One of the greatest strengths of the Q-SPY tracing system is the data transmission protocol. The Q-SPY protocol is very lightweight, but has many the elements of the High Level Data Link Control (<a href="https://en.wikipedia.org/wiki/High-Level_Data_Link_Control" target="_blank" class="extern">HDLC</a>) protocol defined by the International Standards Organization (ISO). The protocol has provisions for detecting transmission errors and allows for instantaneous re-synchronization after any error, such as data dropouts due to RAM buffer overruns.

The Q-SPY protocol has been specifically designed to simplify the data management overhead in the target, yet to allow detection of any data dropouts due to the trace buffer overruns. The protocol has not only provisions for detecting gaps in the data and other errors, but allows for instantaneous re-synchronization after any error to minimize data loss.

@image html qspy6.gif "Q-SPY transmission protocol"

The QS protocol transmits each trace record in an HDLC-like frame. The upper part of the figure above shows the serial data stream transmitted from the target containing frames of different lengths. The bottom part of the figure above shows the details of a single frame:

1. Each frame starts with the Frame Sequence Number byte. The target QS component increments the Frame Sequence Number for every frame inserted into the circular buffer. The Sequence Number naturally rolls-over from 255 to 0. The Frame Sequence Number allows the QSPY host component to detect any data discontinuities.

2. Following the Fame Sequence Number is the Record ID byte, which is one of the predefined QS records, or an application-specific record.

3. Following the Record ID is zero or more Data bytes.

4. Following the data is the Checksum. The Checksum is computed over the Fame Sequence Number, the Record ID, and all the Data bytes. The next section gives the detailed Checksum computation formula.

5. Following the Checksum is the HDLC Flag, which delimits the frame. The HDLC flag is the `01111110` binary string (`0x7E` hexadecimal). Please note that the Q-SPY protocol uses only one HDLC Flag at the end of each frame an no HDLC Flag at the beginning of a frame. In other words, only one Flag is inserted between frames.

The QS target component performs the HDLC-like framing described above at the time the bytes are inserted into the circular trace buffer. This means that the data in the buffer is already cleanly divided into frames and can be transmitted in any chunks, typically not aligned with the frame boundaries.

<div class="separate"></div>
@subsection qspy_transparent Transparency
One of the most important characteristics of HDLC-type protocols is establishing very easily identifiable frames in the serial data stream. Any receiver of such a protocol can instantaneously synchronize to the frame boundary by simply finding the Flag byte. This is because the special Flag byte can never occur within the content of a frame. To avoid confusing unintentional Flag bytes that can naturally occur in the data stream with an intentionally sent Flag, HDLC uses a technique known as transparency (a.k.a. byte-stuffing or escaping) to make the Flag bytes transparent during the transmission. Whenever the transmitter encounters a Flag byte in the data, it inserts a two-byte escape sequence to the output stream. The first byte is the Escape byte, defined as binary `01111101` (hexadecimal `0x7D`). The second byte is the original byte XOR-ed with `0x20`.

Of course, now the Escape byte itself must also be transparent to avoid interpreting an unintentional Escape byte as the two-byte escape sequence. The procedure of escaping the Escape byte is identical to that of escaping the Flag byte.

The transparency of the Flag and Escape bytes complicates slightly the computation of the Checksum. The transmitter computes the Checksum over the Fame Sequence Number, the Record ID, and all Data bytes before performing any “byte-stuffing”. The receiver must apply the exact reversed procedure of performing the “byte-un-stuffing” before computing the Checksum.

An example may make this clearer. Suppose that the following trace record needs to be inserted to the trace buffer (the transparent bytes are shown in bold):

     Record ID = 0x7D, Record Data = 0x7D 0x08 0x01

Assuming that the current Fame Sequence Number is, say `0x7E`, the Checksum will be computed over the following bytes:

     Checksum == (uint8_t)(~(0x7E + 0x7D + 0x7D + 0x08 + 0x01)) == 0x7E

and the actual frame inserted into the QS trace buffer will be as follows:

     0x7D 0x5E 0x7D 0x5D 0x7D 0x5D 0x08 0x01 0x7D 0x5E 0x7E

Obviously, this is a degenerated example, where the Frame Sequence Number, the Record ID, a Data byte, and the Checksum itself turned out to be the transparent bytes. Typical overhead of transparency with real trace data is one escape sequence per several trace records.


<div class="separate"></div>
@subsection qspy_endianness Endianness
In addition to the HDLC-like framing, the QS transmission protocol specifies the endianness of the data to be <strong>little-endian</strong>. All multi-byte data elements, such as 16-, 32-, or 64-bit integers, pointers, and floating point numbers are inserted into the QS trace buffer in the little-endian byte order (least-significant byte first). The QS data inserting macros place the data in the trace buffer in a platform-neutral manner, meaning that the data is inserted into the buffer in the little-endian order regardless of the endianness of the CPU. Also, the data-inserting macros copy the data to the buffer one byte at a time, thus avoiding any potential data misalignment problems. Many embedded CPUs, such as ARM, require certain alignment of 16-, 32-, or 64-bit quantities.


<div class="separate"></div>
@subsection qspy_last_is_best Last-is-Best Data Policy
The QS Trace Buffers (transmit QS buffer and receive QS-RX buffer) store only complete HDLC frames, which is the pivotal point in the design of the QS target component and has two important consequences.

First, the use of HDLC frames in the buffers enables the <strong>last is best</strong> tracing policy. The QS transmission protocol maintains both the Frame Sequence Number and the Checksum over each trace record, which means that any data corruption caused by overrunning the old data with the new data can be always reliably detected. Therefore, the new trace data is simply inserted into the circular trace buffers, regardless if it perhaps overwrites the old data that hasn't been sent out yet, or is in the process of being sent. The burden of detection any data corruption is placed on the QSPY host component. When you start missing the frames (which the host component easily detects by discontinuities in the Frame Sequence Number), you have several options. Your can apply some additional filtering, increase the size of the buffer, or improve the data transfer throughput.

Second, the use of HDLC-formatted data in the trace buffers allows decoupling the data insertion into the trace buffers from the data removal out of the trace buffers. You can simply remove the data in whichever chunks you like, without any consideration for frame boundaries. You can employ just about any available physical data link available on the target for transferring the trace data from the target to the host.

@next{qspy}
*/
