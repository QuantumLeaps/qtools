##
# @file
# @ingroup qpspypy
# @brief Interface to the QSPY "back end".

## @cond
#-----------------------------------------------------------------------------
# Last updated for version: 2.0.0
# Last updated on: 2018-08-16
#
# Copyright (c) 2018 Lotus Engineering, LLC
# Copyright (c) 2018 Quantum Leaps, LLC
#
# MIT License:
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Contact information:
# https://www.state-machine.com
# mailto:info@state-machine.com
#-----------------------------------------------------------------------------
# @endcond

from enum import IntFlag, IntEnum
import socket
import struct
import time
import threading


## Enumeration for packet IDs that are interpreted by QSPY
class QSPY(IntEnum):
    ATTACH = 128,
    DETACH = 129,
    SAVE_DICT = 130,
    SCREEN_OUT = 131,
    BIN_OUT = 132,
    MATLAB_OUT = 133,
    MSCGEN_OUT = 134,
    SEND_EVENT = 135,
    SEND_LOC_FILTER = 136,
    SEND_CURR_OBJ = 137,
    SEND_COMMAND = 138,
    SEND_TEST_PROBE = 139


## Enumeration for packet IDs that are sent directly to the target
class QS_RX(IntEnum):
    INFO = 0
    COMMAND = 1
    RESET = 2
    TICK = 3
    PEEK = 4
    POKE = 5
    FILL = 6
    TEST_SETUP = 7
    TEST_TEARDOWN = 8
    TEST_PROBE = 9
    GLB_FILTER = 10
    LOC_FILTER = 11
    AO_FILTER = 12
    CURR_OBJ = 13
    CONTINUE = 14
    RESERVED1 = 15
    EVENT = 16

## Records from client Must be kept in sync with qs_copy.h
class QSpyRecords(IntEnum):
    # /* [0] QS session (not maskable) */
    QS_TEXT = 0,  # QS_EMPTY = 0, #/*!< QS record for cleanly starting a session */

    # /* [1] SM records */
    QS_QEP_STATE_ENTRY = 1,  # /*!< a state was entered */
    QS_QEP_STATE_EXIT = 2,  # /*!< a state was exited */
    QS_QEP_STATE_INIT = 3,  # /*!< an initial transition was taken in a state */
    QS_QEP_INIT_TRAN = 4,  # /*!< the top-most initial transition was taken */
    QS_QEP_INTERN_TRAN = 5,  # /*!< an internal transition was taken */
    QS_QEP_TRAN = 6,  # /*!< a regular transition was taken */
    QS_QEP_IGNORED = 7,  # /*!< an event was ignored (silently discarded) */
    QS_QEP_DISPATCH = 8,  # /*!< an event was dispatched (begin of RTC step) */
    QS_QEP_UNHANDLED = 9,  # /*!< an event was unhandled due to a guard */

    # /* [10] AO records */
    QS_QF_ACTIVE_DEFER = 10,  # /*!< AO deferred an event */
    QS_QF_ACTIVE_RECALL = 11,  # /*!< AO recalled an event */
    QS_QF_ACTIVE_SUBSCRIBE = 12,  # /*!< an AO subscribed to an event */
    QS_QF_ACTIVE_UNSUBSCRIBE = 13,  # /*!< an AO unsubscribed to an event */
    # /*!< an event was posted (FIFO) directly to AO */
    QS_QF_ACTIVE_POST_FIFO = 14,
    # /*!< an event was posted (LIFO) directly to AO */
    QS_QF_ACTIVE_POST_LIFO = 15,
    QS_QF_ACTIVE_GET = 16,  # /*!< AO got an event and its queue is not empty */
    QS_QF_ACTIVE_GET_LAST = 17,  # /*!< AO got an event and its queue is empty */
    QS_QF_ACTIVE_RECALL_ATTEMPT = 18,  # /*!< AO attempted to recall an event */

    # /* [19] EQ records */
    # /*!< an event was posted (FIFO) to a raw queue */
    QS_QF_EQUEUE_POST_FIFO = 19,
    # /*!< an event was posted (LIFO) to a raw queue */
    QS_QF_EQUEUE_POST_LIFO = 20,
    QS_QF_EQUEUE_GET = 21,  # /*!< get an event and queue still not empty */
    QS_QF_EQUEUE_GET_LAST = 22,  # /*!< get the last event from the queue */

    QS_QF_RESERVED2 = 23,

    # /* [24] MP records */
    QS_QF_MPOOL_GET = 24,  # /*!< a memory block was removed from memory pool */
    QS_QF_MPOOL_PUT = 25,  # /*!< a memory block was returned to memory pool */

    # /* [26] QF records */
    QS_QF_PUBLISH = 26,  # /*!< an event was published */
    QS_QF_NEW_REF = 27,  # /*!< new event reference was created */
    QS_QF_NEW = 28,  # /*!< new event was created */
    QS_QF_GC_ATTEMPT = 29,  # /*!< garbage collection attempt */
    QS_QF_GC = 30,  # /*!< garbage collection */
    QS_QF_TICK = 31,  # /*!< QF_tickX() was called */

    # /* [32] TE records */
    QS_QF_TIMEEVT_ARM = 32,  # /*!< a time event was armed */
    QS_QF_TIMEEVT_AUTO_DISARM = 33,  # /*!< a time event expired and was disarmed */
    # /*!< attempt to disarm a disarmed QTimeEvt */
    QS_QF_TIMEEVT_DISARM_ATTEMPT = 34,
    QS_QF_TIMEEVT_DISARM = 35,  # /*!< true disarming of an armed time event */
    QS_QF_TIMEEVT_REARM = 36,  # /*!< rearming of a time event */
    QS_QF_TIMEEVT_POST = 37,  # /*!< a time event posted itself directly to an AO */

    # /* [38] QF records */
    QS_QF_DELETE_REF = 38,  # /*!< an event reference is about to be deleted */
    QS_QF_CRIT_ENTRY = 39,  # /*!< critical section was entered */
    QS_QF_CRIT_EXIT = 40,  # /*!< critical section was exited */
    QS_QF_ISR_ENTRY = 41,  # /*!< an ISR was entered */
    QS_QF_ISR_EXIT = 42,  # /*!< an ISR was exited */
    QS_QF_INT_DISABLE = 43,  # /*!< interrupts were disabled */
    QS_QF_INT_ENABLE = 44,  # /*!< interrupts were enabled */

    # /* [45] AO records */
    QS_QF_ACTIVE_POST_ATTEMPT = 45,  # /*!< attempt to post an evt to AO failed */

    # /* [46] EQ records */
    QS_QF_EQUEUE_POST_ATTEMPT = 46,  # /*!< attempt to post an evt to QEQueue failed */

    # /* [47] MP records */
    QS_QF_MPOOL_GET_ATTEMPT = 47,  # /*!< attempt to get a memory block failed */

    # /* [48] SC records */
    QS_MUTEX_LOCK = 48,  # /*!< a mutex was locked */
    QS_MUTEX_UNLOCK = 49,  # /*!< a mutex was unlocked */
    QS_SCHED_LOCK = 50,  # /*!< scheduler was locked */
    QS_SCHED_UNLOCK = 51,  # /*!< scheduler was unlocked */
    QS_SCHED_NEXT = 52,  # /*!< scheduler found next task to execute */
    QS_SCHED_IDLE = 53,  # /*!< scheduler became idle */
    QS_SCHED_RESUME = 54,  # /*!< scheduler resumed previous task (not idle) */

    # /* [55] QEP records */
    QS_QEP_TRAN_HIST = 55,  # /*!< a tran to history was taken */
    QS_QEP_TRAN_EP = 56,  # /*!< a tran to entry point into a submachine */
    QS_QEP_TRAN_XP = 57,  # /*!< a tran to exit  point out of a submachine */

    # /* [58] Miscellaneous QS records (not maskable) */
    QS_TEST_PAUSED = 58,  # /*!< test has been paused */
    QS_TEST_PROBE_GET = 59,  # /*!< reports that Test-Probe has been used */
    QS_SIG_DICT = 60,  # /*!< signal dictionary entry */
    QS_OBJ_DICT = 61,  # /*!< object dictionary entry */
    QS_FUN_DICT = 62,  # /*!< function dictionary entry */
    QS_USR_DICT = 63,  # /*!< user QS record dictionary entry */
    QS_TARGET_INFO = 64,  # /*!< reports the Target information */
    QS_TARGET_DONE = 65,  # /*!< reports completion of a user callback */
    QS_RX_STATUS = 66,  # /*!< reports QS data receive status */
    QS_MSC_RESERVED1 = 67,
    QS_PEEK_DATA = 68,  # /*!< reports the data from the PEEK query */
    QS_ASSERT_FAIL = 69,  # /*!< assertion failed in the code */

    # /* [70] Application-specific (User) QS records */
    QS_USER1 = 70  # /*!< the first record available to QS users */
    QS_USER2 = 71  # /*!< the first record available to QS users */
    QS_USER3 = 72  # /*!< the first record available to QS users */
    QS_USER4 = 73  # /*!< the first record available to QS users */
    QS_USER5 = 74  # /*!< the first record available to QS users */
    QS_USER6 = 75  # /*!< the first record available to QS users */
    QS_USER7 = 76  # /*!< the first record available to QS users */
    QS_USER8 = 77  # /*!< the first record available to QS users */
    QS_USER9 = 78  # /*!< the first record available to QS users */
    QS_USER10 = 79  # /*!< the first record available to QS users */
    QS_USER11 = 80  # /*!< the first record available to QS users */
    QS_USER12 = 81  # /*!< the first record available to QS users */
    QS_USER13 = 82  # /*!< the first record available to QS users */


## Enumeration for channel type, this must match enum in "be.c"
class QS_CHANNEL(IntFlag):
    BINARY = 1,
    TEXT = 2


class QS_OBJ_KIND(IntEnum):
    SM = 0,  # State Machine
    AO = 1,  # Active Object
    MP = 2,  # Memory Pool
    EQ = 3,  # Event Queue
    TE = 4,  # Time Event
    AP = 5,  # Application-Specific
    SM_AO = 6  # Active object and state machine

## QS record groups for filters
class FILTER (IntEnum):
    ON = 1,   # all maskable QS records on
    OFF = 2,  # all maskable QS records on
    SM = 3,   # State Machine QS records
    AO = 4,   # Active Object QS records
    EQ = 5,   # Event Queues QS records
    MP = 6,   # Memory Pools QS records
    TE = 7,   # Time Events QS records
    QF = 8,   # QF QS records
    SC = 9,   # Scheduler QS records
    U0 = 10,  # User Group 70-79 records
    U1 = 11,  # User Group 80-89 records
    U2 = 12,  # User Group 90-99 records
    U3 = 13,  # User Group 100-109 records
    U4 = 14,  # User Group 110-124 records
    UA = 15   # All User records

## Port specific formats used in struct.pack
theFmt = {
    'objPtr': 'I',
    'funPtr': 'I',
    'tstamp': 'I',
    'sig': 'h',
    'evtSize': 'h',
    'queueCtr': 'B',
    'poolCtr': 'h',
    'poolBlk': 'h',
    'tevtCtr': 'h'
}

## Special priority values used to send commands
class PRIO_COMMAND(IntEnum):
    PUBLISH = 0,         # Publish event
    DISPATCH = 255,      # dispatch event to the Current Object(SM)
    # take the top-most initial transition in the Current Object (SM)
    DO_INIT_TRANS = 254,
    POST = 253           # post event to the Current Object (AO)

class qspy(threading.Thread):

    def __init__(self):
        super().__init__()
        self.tx_packet_seq = 0
        self.socket = None
        self.alive = threading.Event()
        self.rx_packet_seq = 0
        self.rx_packet_errors = 0
        self.rx_record_seq = 0
        self.rx_record_errors = 0

    def __del__(self):
        if self.socket is not None:
            self.socket.close

    # Socket receive thread
    def run(self):
        while self.alive.isSet():
            try:
                packet = self.socket.recv(1024)
                if len(packet) < 2:
                    continue

                rx_sequence = packet[0]
                recordID = packet[1]

                if recordID < 128:
                    method_name = "OnRecord_" + QSpyRecords(recordID).name
                    if self.rx_record_seq != rx_sequence:
                        print("Rx Record sequence error!")
                        self.rx_record_errors += 1
                        self.rx_record_seq = rx_sequence  # resync
                    self.rx_record_seq += 1
                    self.rx_record_seq &= 0xFF
                else:
                    method_name = "OnPacket_" + QSPY(recordID).name
                    if self.rx_packet_seq != rx_sequence:
                        print("Rx Packet sequence error!")
                        self.rx_packet_errors += 1
                        self.rx_packet_seq = rx_sequence  # resync
                    self.rx_packet_seq += 1
                    self.rx_packet_seq &= 0xFF

                #print("Seq:{0}, {1}({2})".format(rx_sequence, method_name, packet.hex()))

                try:
                    method = getattr(self.client, method_name)
                    # Call client callback with packet
                    method(packet)

                except AttributeError:
                    raise NotImplementedError("Class `{}` does not implement `{}`".format(
                        self.client.__class__.__name__, method_name))

            except IOError as e:
                # We expect this for now until we refactor the detach()
                #print("QSpy Socket error:", str(e) )
                e
                pass

    @classmethod
    def parse_QS_TEXT(cls, packet):
        """ Returns a tuple of (record, line) of types QSpyRecords, string respectively
        """
        assert QSpyRecords(
            packet[1]) == QSpyRecords.QS_TEXT, "Wronge record type for parser"
        return (QSpyRecords(packet[2]), packet[3:].decode("utf-8"))

    def attach(self, client, host='localhost', port=7701, channels=QS_CHANNEL.TEXT, local_port=None):
        """ Attach to the QSpy backend

        Keyword arguments:
        host -- host IP address of QSpy (default 'localhost')
        port -- socket port of QSpy (default 7701)
        channels -- what channels to attach to (default QPChannels.TEXT)
        local_port -- the local/client port to use (default None for automatic)
        """

        # Store client for callback
        self.client = client

        # Store address info
        self.host = host
        self.port = port
        self.channels = channels
        self.local_port = local_port

        # Create socket and connect
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        if local_port is not None:
            self.socket.bind(('', local_port))
        self.socket.connect((host, port))

        # Start receive thread
        self.alive.set()
        self.start()

        self.sendAttach(channels)

    def detach(self):
        self.alive.clear()
        self.sendPacket(struct.pack('<B', QSPY.DETACH.value))
        time.sleep(0.300)
        self.socket.shutdown(socket.SHUT_RDWR)
        self.socket.close()
        self.socket = None
        self.client = None
        threading.Thread.join(self)

    def sendAttach(self, channels):
        self.sendPacket(struct.pack(
            '<BB', QSPY.ATTACH.value, channels.value))

    def sendLocalFilter(self, object_kind, object_id):
        """ Sends a local filter

        Arguments:
        object_kind -- kind of object from QS_OBJ_KIND
        object_id -- the object which can be an address integer or a dictionary name string
        """
        assert isinstance(object_kind, QS_OBJ_KIND)

        format_string = '<BB' + theFmt['objPtr']

        if isinstance(object_id, int):
            # Send directly to Target
            packet = struct.pack(
                format_string, QS_RX.LOC_FILTER, object_kind.value, object_id)
        else:
            # Have QSpy interpret object_id string and send filter
            packet = bytearray(struct.pack(
                format_string, QSPY.SEND_LOC_FILTER, object_kind.value, 0))
            packet.extend(qspy.string_to_binary(object_id))

        self.sendPacket(packet)

    def sendGlobalFilters(self, *args):

        filter0 = 0
        filter1 = 0
        filter2 = 0
        filter3 = 0

        for _filter in args:
            if _filter == FILTER.OFF:
                pass
            elif _filter == FILTER.ON:
                # all filters on
                filter0 = 0xFFFFFFFF
                filter1 = 0xFFFFFFFF
                filter2 = 0xFFFFFFFF
                filter3 = 0x1FFFFFFF
                break  # no point in continuing
            elif _filter == FILTER.SM:  # state machines
                filter0 |= 0x000003FE
                filter1 |= 0x03800000
            elif _filter == FILTER.AO:   # active objects
                filter0 |= 0x0007FC00
                filter1 |= 0x00002000
            elif _filter == FILTER.EQ:  # raw queues (for deferral)
                filter0 |= 0x00780000
                filter2 |= 0x00004000
            elif _filter == FILTER.MP:  # raw memory pools
                filter0 |= 0x03000000
                filter2 |= 0x00008000
            elif _filter == FILTER.QF:  # framework
                filter0 |= 0xFC000000
                filter1 |= 0x00001FC0
            elif _filter == FILTER.TE:  # time events
                filter1 |= 0x0000007F
            elif _filter == FILTER.SC:  # scheduler
                filter1 |= 0x007F0000
            elif _filter == FILTER.U0:  # user 70-79
                filter2 |= 0x0000FFC0
            elif _filter == FILTER.U1:  # user 80-89
                filter2 |= 0x03FF0000
            elif _filter == FILTER.U2:  # user 90-99
                filter2 |= 0xFC000000
                filter3 |= 0x0000000F
            elif _filter == FILTER.U3:  # user 100-109
                filter3 |= 0x00003FF0
            elif _filter == FILTER.U4:  # user 110-124
                filter3 |= 0x1FFFC000
            elif _filter == FILTER.UA:  # user 70-124 (all)
                filter2 |= 0xFFFFFFC0
                filter3 |= 0x1FFFFFFF
            else:
                assert 0, 'invalid filter group'

        self.sendPacket(struct.pack(
            '<BBLLLL', QS_RX.GLB_FILTER,  16, filter0, filter1, filter2, filter3))

    def sendFill(self, offset, size, num, item):
        """ Sends fill packet """

        if size == 1:
            item_fmt = 'B'
        elif size == 2:
            item_fmt = 'H'
        elif size == 4:
            item_fmt = 'I'
        else:
            assert False, "size for sendFill must be 1, 2, or 4!"

        format_string = '<BHBB' + item_fmt
        packet = struct.pack(format_string, QS_RX.FILL, offset, size, num, item)
        self.sendPacket(packet)

    def sendPeek(self, offset, size, num):
        """ Sends poke packet """

        format_string = '<BHBB'
        packet = struct.pack(format_string, QS_RX.PEEK, offset, size, num)
        self.sendPacket(packet)

    def sendPoke(self, offset, size, num, data):
        """ Sends peek packet """

        format_string = '<BHBB'
        packet = bytearray(struct.pack(format_string, QS_RX.POKE, offset, size, num))
        packet.extend(data)
        self.sendPacket(packet)

    def sendCommand(self, command_id, param1=0, param2=0, param3=0):
        """ Sends command packet """

        format_string = '<BBIII'
        if isinstance(command_id, int):
            packet = struct.pack(format_string, QS_RX.COMMAND,
                                 command_id, param1, param2, param3)
        else:
            packet = bytearray(struct.pack(
                format_string, QSPY.SEND_COMMAND, 0, param1, param2, param3))
            # Add string command ID to end
            packet.extend(qspy.string_to_binary(command_id))
        self.sendPacket(packet)

    def sendCurrentObject(self, object_kind, object_id):

        format_string = '<BB' + theFmt['objPtr']
        # Build packet according to object_id type
        if isinstance(object_id, int):
            packet = struct.pack(format_string, QS_RX.CURR_OBJ,
                                 object_kind.value, object_id)
        else:
            packet = bytearray(struct.pack(
                format_string, QSPY.SEND_CURR_OBJ, object_kind.value, 0))
            # add string object ID to end
            packet.extend(qspy.string_to_binary(object_id))
        self.sendPacket(packet)

    def sendTestProbe(self, function, data):
        format_string = '<BI' + theFmt['funPtr']

        if isinstance(function, int):
            # Send directly to target
            packet = struct.pack(
                format_string, QS_RX.TEST_PROBE, data, function)
        else:
            # Send to QSPY to provide 'function' from Fun Dictionary
            packet = bytearray(struct.pack(
                format_string, QSPY.SEND_TEST_PROBE, data, 0))
            # add string function name to end
            packet.extend(qspy.string_to_binary(function))

        self.sendPacket(packet)

    def sendTick(self, rate):
        packet = struct.pack('<BB', QS_RX.TICK, rate)
        self.sendPacket(packet)

    def sendEvent(self, ao_priority, signal, parameters=None):
        """ Sends and event to an active object

        Args:
            ao_priority : ao priority or one of the PRIO_COMMAND enums
            signal : signal string or value
            parameters : (optional) bytes or bytesarray of payload
        """
        format_string = '<BB' + theFmt['sig'] + 'h'

        if parameters is not None:
            length = len(parameters)
        else:
            length = 0

        if isinstance(signal, int):
            packet = bytearray(struct.pack(
                format_string, QS_RX.EVENT, ao_priority, signal, length))
            if parameters is not None:
                packet.extend(parameters)
        else:
            packet = bytearray(struct.pack(
                format_string, QSPY.SEND_EVENT, ao_priority, 0, length))
            if parameters is not None:
                packet.extend(parameters)
            packet.extend(qspy.string_to_binary(signal))

        self.sendPacket(packet)

    def sendReset(self):
        self.sendPacket(struct.pack('<B', QS_RX.RESET))

    def sendContinue(self):
        self.sendPacket(struct.pack('<B', QS_RX.CONTINUE))

    def sendSetup(self):
        self.sendPacket(struct.pack('<B', QS_RX.TEST_SETUP))

    def sendTeardown(self):
        self.sendPacket(struct.pack('<B', QS_RX.TEST_TEARDOWN))

    def sendPacket(self, packet):
        """ sends a packet

        Arguments:
        packet -- packet to send either a bytes() or bytearray() object
        """

        tx_packet = bytearray({self.tx_packet_seq})
        tx_packet.extend(packet)

        self.socket.send(tx_packet)

        self.tx_packet_seq += 1
        self.tx_packet_seq &= 0xFF
        #print("Qspy tx seq >>", self.tx_packet_seq)

    @staticmethod
    def string_to_binary(string_packet):
        packed_string = bytes(string_packet, 'utf-8')
        format_string = '{0}sB'.format(len(packed_string) + 1)
        # Null terminate and return
        return(struct.pack(format_string, packed_string, 0))
