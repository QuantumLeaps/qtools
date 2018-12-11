/**
* @file
* @brief Back-End connection point for the external Front-Ends
* @ingroup qpspy
* @cond
******************************************************************************
* Last updated for version 6.3.7
* Last updated on  2018-11-06
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005-2018 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Alternatively, this program may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GNU General Public License and are specifically designed for
* licensees interested in retaining the proprietary status of their code.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Contact information:
* https://www.state-machine.com
* mailto:info@state-machine.com
******************************************************************************
* @endcond
*/
#include <string.h>  /* for size_t */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "qspy.h"    /* QSPY data parser */
#include "be.h"      /* Back-End interface */
#include "pal.h"     /* Platform Abstraction Layer */

typedef char     char_t;
typedef float    float32_t;
typedef double   float64_t;
typedef int      enum_t;
typedef unsigned uint_t;
typedef void     QEvt;

#ifndef Q_SPY
#define Q_SPY 1
#endif
#include "qs_copy.h" /* copy of the target-resident QS interface */

/*..........................................................................*/
static uint8_t  l_buf[8*1024]; /* the output buffer [bytes] */
static uint8_t *l_pos;         /* current position in the output buffer */
static uint8_t  l_rxBeSeq;     /* receive  Back-End  sequence number */
static uint8_t  l_txBeSeq;     /* transmit Back-End sequence number */
static uint8_t  l_channels;    /* channels of the output (bitmask) */

enum Channels {
    BINARY_CH = (1 << 0),
    TEXT_CH   = (1 << 1)
};

#define BIN_FORMAT "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BIN(byte_)  \
  (byte_ & 0x80 ? '1' : '0'), \
  (byte_ & 0x40 ? '1' : '0'), \
  (byte_ & 0x20 ? '1' : '0'), \
  (byte_ & 0x10 ? '1' : '0'), \
  (byte_ & 0x08 ? '1' : '0'), \
  (byte_ & 0x04 ? '1' : '0'), \
  (byte_ & 0x02 ? '1' : '0'), \
  (byte_ & 0x01 ? '1' : '0')

/*..........................................................................*/
#ifndef NDEBUG
static FILE *l_testFile;
#endif

void BE_onStartup(void) {
    l_pos      = &l_buf[0];
    l_rxBeSeq  = 0U;
    l_txBeSeq  = 0U;
    l_channels = 0U;

#ifndef NDEBUG
    FOPEN_S(l_testFile, "fromFE.bin", "wb");
#endif
}
/*..........................................................................*/
void BE_onCleanup(void) {
    BE_sendPkt(QSPY_DETACH);
#ifndef NDEBUG
    fclose(l_testFile);
#endif
}

/*..........................................................................*/
void BE_parse(unsigned char *buf, size_t nBytes) {
    /* By nature of UDP, each transmission from the Front-End contains
    * one complete UDP packet, which contains one complete QS record.
    * This is a greatly simplifying assumption in this routine.
    *
    * NOTE: if the communication mechanism would change from UDP to
    * to something else (e.g. TCP), the following code would need to
    * change, as the packet-boundaries might not be preserved.
    */

    /* check the continuity of the data from the Front-End... */
    if (l_channels != 0U) { /* is a Front-End attached? */
        ++l_rxBeSeq;
        if (buf[0] != l_rxBeSeq) {
            SNPRINTF_LINE("   <F-END> ERROR    Data Discontinuity "
                          "Seq=%u->%u",
                          (unsigned)l_rxBeSeq, (unsigned)buf[0]);
            QSPY_printError();
            l_rxBeSeq = buf[0];
        }
    }

    /* check if this is a packet to be forwarded directly to the Target */
    if (buf[1] < 128) {
        static uint8_t qbuf[QS_MAX_RECORD_SIZE]; /* encoded QS record */
        uint32_t len;

        /* encode the packet according to the QS/QSPY protocol */
        len = QSPY_encode(qbuf, sizeof(qbuf), buf, nBytes);
        if (len > 0U) {
            if ((*PAL_vtbl.send2Target)(qbuf, len) != QSPY_SUCCESS) {
                SNPRINTF_LINE("   <COMMS> ERROR    Sedning Data "
                              "to the Target Rec=%d,Len=%d",
                              (int)buf[1], (int)len);
                QSPY_printError();
            }
#ifndef NDEBUG
            if (l_testFile != (FILE *)0) {
                fwrite(qbuf, 1, len, l_testFile);
            }
#endif
        }
        else {
            SNPRINTF_LINE("   <COMMS> ERROR    Target packet too big Len=%d",
                          (int)len);
            QSPY_printError();
        }
    }
    else {
        QSpyRecord qrec;
        QSpyRecord_init(&qrec, buf, nBytes); /* buf contains one QS record */
        BE_parseRecFromFE(&qrec);  /* parse the QS record from Front-End */
    }
}

/*..........................................................................*/
void BE_parseRecFromFE(QSpyRecord * const qrec) {
    switch (qrec->rec) {
        case QSPY_ATTACH: {   /* attach to the Front-End */
            if (l_channels == 0U) { /* no Front-End attached yet? */
                SNPRINTF_LINE("   <F-END> Attached Chan="BIN_FORMAT,
                              BYTE_TO_BIN(qrec->start[2]));
                QSPY_printInfo();
            }

            if (qrec->tot_len > 2U) { /* payload contains channels? */
                l_channels = qrec->start[2];
            }
            else { /* old payload without channels */
                l_channels = BINARY_CH; /* default to binary channel  */
            }
            l_rxBeSeq  = qrec->start[0]; /* re-start the receive  sequence */
            l_txBeSeq  = 0U;             /* re-start the transmit sequence */

            /* send the attach confirmation packet back to the Front-End */
            BE_sendPkt(QSPY_ATTACH);

            break;
        }
        case QSPY_DETACH: {   /* detach from the Front-End */
            PAL_detachFE();
            l_channels = 0U; /* detached from a Front-End */
            SNPRINTF_LINE(
            "   <F-END> Detached ######################################");
            QSPY_printInfo();
            break;
        }
        case QSPY_SAVE_DICT: { /* save dictionaries collected so far */
            QSPY_command('d');
            break;
        }
        case QSPY_SCREEN_OUT: {
            QSPY_command('o');
            break;
        }
        case QSPY_BIN_OUT: {
            QSPY_command('s');
            break;
        }
        case QSPY_MATLAB_OUT: {
            QSPY_command('m');
            break;
        }
        case QSPY_MSCGEN_OUT: {
            QSPY_command('g');
            break;
        }

        case QSPY_SEND_EVENT: {
            QSPY_sendEvt(qrec);
            break;
        }
        case QSPY_SEND_LOC_FILTER:
        case QSPY_SEND_CURR_OBJ: {
            QSPY_sendObj(qrec);
            break;
        }
        case QSPY_SEND_COMMAND: {
            QSPY_sendCmd(qrec);
            break;
        }
        case QSPY_SEND_TEST_PROBE: {
            QSPY_sendTP(qrec);
            break;
        }

        default: {
            SNPRINTF_LINE("   <F-END> ERROR    Unrecognized command Rec=%d",
                          (int)qrec->rec);
            QSPY_printError();
            break;
        }
    }
}
/*..........................................................................*/
int BE_parseRecFromTarget(QSpyRecord * const qrec) {
    if ((l_channels & BINARY_CH) != 0) {
        if (qrec->rec != QS_EMPTY) {
            /* forward the Target binary record to the Front-End... */
            PAL_send2FE(qrec->start, qrec->tot_len - 1U);
        }
    }
    else if (l_channels != 0U) {
        if (qrec->rec == QS_TARGET_INFO) {
            /* forward the Target Info record to the Front-End... */
            PAL_send2FE(qrec->start, qrec->tot_len - 1U);
        }
    }
    return 1; /* continue with the standard QSPY processing */
}

/*--------------------------------------------------------------------------*/
void BE_sendPkt(int pktId) {
    if ((pktId >= 128) || ((l_channels & 1U) != 0)) {
        l_pos = &l_buf[0];
        ++l_txBeSeq;
        *l_pos++ = l_txBeSeq;
        *l_pos++ = (uint8_t)pktId;
        PAL_send2FE(l_buf, (l_pos - &l_buf[0]));
    }
}
/*..........................................................................*/
void BE_sendLine(void) {
    /* global filter for permanently enabled QS records
    * minus the records that generate time stamps
    */
    static uint8_t const glbFilter[32] = {
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    if ((l_channels & 2U) != 0) {
        uint8_t rec = (uint8_t)QSPY_output.rec;

        /* is this QS record not permanently blocked? */
        if ((glbFilter[rec >> 3] & (1U << (rec & 7U))) == 0U) {
            ++l_txBeSeq;

            /* prepend the BE UDP packet header in front of the string */
            QSPY_output.buf[QS_LINE_OFFSET - 3] = l_txBeSeq;
            QSPY_output.buf[QS_LINE_OFFSET - 2] = QS_EMPTY;
            QSPY_output.buf[QS_LINE_OFFSET - 1] = rec;

            PAL_send2FE((uint8_t const *)&QSPY_output.buf[QS_LINE_OFFSET - 3],
                        QSPY_output.len + 3);
        }
    }
}

/*--------------------------------------------------------------------------*/
void BE_putU8(uint8_t d) {
    *l_pos++ = d;
}
/*..........................................................................*/
void BE_putU16(uint16_t d) {
    *l_pos++ = (uint8_t)d; d >>= 8;
    *l_pos++ = (uint8_t)d;
}
/*..........................................................................*/
void BE_putU32(uint32_t d) {
    *l_pos++ = (uint8_t)d;  d >>= 8;
    *l_pos++ = (uint8_t)d;  d >>= 8;
    *l_pos++ = (uint8_t)d;  d >>= 8;
    *l_pos++ = (uint8_t)d;
}
/*..........................................................................*/
void BE_putStr(char const *str) {
    while (*str != '\0') {
        *l_pos++ = (uint8_t)*str++;
    }
}
/*..........................................................................*/
void BE_putMem(uint8_t const *mem, uint8_t size) {
    *l_pos++ = size;
    while (size > 0) {
        *l_pos++ = (uint8_t)*mem++;
        --size;
    }
}
