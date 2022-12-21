/*============================================================================
* QP/C Real-Time Embedded Framework (RTEF)
* Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
*
* SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
*
* This software is dual-licensed under the terms of the open source GNU
* General Public License version 3 (or any later version), or alternatively,
* under the terms of one of the closed source Quantum Leaps commercial
* licenses.
*
* The terms of the open source GNU General Public License version 3
* can be found at: <www.gnu.org/licenses/gpl-3.0>
*
* The terms of the closed source Quantum Leaps commercial licenses
* can be found at: <www.state-machine.com/licensing>
*
* Redistributions in source code must retain this top-level comment block.
* Plagiarizing this software to sidestep the license obligations is illegal.
*
* Contact information:
* <www.state-machine.com>
* <info@state-machine.com>
============================================================================*/
/*!
* @date Last updated on: 2022-12-15
* @version Last updated for version: 7.2.0
*
* @file
* @brief Back-End connection point for the external Front-Ends
*/
#include <stdint.h>
#include <stdbool.h>

#include "safe_std.h" /* "safe" <stdio.h> and <string.h> facilities */
#include "qspy.h"     /* QSPY data parser */
#include "be.h"       /* Back-End interface */
#include "pal.h"      /* Platform Abstraction Layer */

#define Q_SPY   1       /* this is QP implementation */
#define QP_IMPL 1       /* this is QP implementation */
#include "qpc_qs.h"     /* QS target-resident interface */
#include "qpc_qs_pkg.h" /* QS package-scope interface */

/*..........................................................................*/
static uint8_t  l_rxBeSeq;     /* receive  Back-End  sequence number */
static uint8_t  l_txBeSeq;     /* transmit Back-End sequence number */
static uint8_t  l_channels;    /* channels of the output (bitmask) */

enum Channels {
    BINARY_CH = (1 << 0),
    TEXT_CH   = (1 << 1)
};

/* send a packet to Front-End */
static void BE_sendShortPkt(int pktId);

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
    l_rxBeSeq  = 0U;
    l_txBeSeq  = 0U;
    l_channels = 0U;

#ifndef NDEBUG
    FOPEN_S(l_testFile, "fromFE.bin", "wb");
#endif
}
/*..........................................................................*/
void BE_onCleanup(void) {
    BE_sendShortPkt(QSPY_DETACH);
#ifndef NDEBUG
    fclose(l_testFile);
#endif
}

/*..........................................................................*/
void BE_parse(unsigned char *buf, uint32_t nBytes) {
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
        static uint8_t qbuf[QS_RECORD_SIZE_MAX]; /* encoded QS record */

        /* encode the packet according to the QS/QSPY protocol */
        uint32_t len = QSPY_encode(qbuf, sizeof(qbuf), buf, nBytes);
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
    uint8_t action;
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
            BE_sendShortPkt(QSPY_ATTACH);

            break;
        }
        case QSPY_DETACH: {   /* detach from the Front-End */
            PAL_detachFE();
            l_channels = 0U; /* detached from a Front-End */
            SNPRINTF_LINE("   <F-END> Detached %s",
                          "######################################");
            QSPY_printInfo();
            break;
        }
        case QSPY_SAVE_DICT: { /* save dictionaries collected so far */
            QSPY_command('d', CMD_OPT_TOGGLE);
            break;
        }
        case QSPY_TEXT_OUT: {
            action = CMD_OPT_TOGGLE;
            if (qrec->tot_len > 2U) { /* payload contains action? */
                action = qrec->start[2];
            }
            QSPY_command('o', action);
            break;
        }
        case QSPY_BIN_OUT: {
            action = CMD_OPT_TOGGLE;
            if (qrec->tot_len > 2U) { /* payload contains action? */
                action = qrec->start[2];
            }
            QSPY_command('s', action);
            break;
        }
        case QSPY_MATLAB_OUT: {
            action = CMD_OPT_TOGGLE;
            if (qrec->tot_len > 2U) { /* payload contains action? */
                action = qrec->start[2];
            }
            QSPY_command('m', action);
            break;
        }
        case QSPY_SEQUENCE_OUT: {
            action = CMD_OPT_TOGGLE;
            if (qrec->tot_len > 2U) { /* payload contains action? */
                action = qrec->start[2];
            }
            QSPY_command('g', action);
            break;
        }

        case QSPY_SEND_EVENT: {
            QSPY_sendEvt(qrec);
            break;
        }
        case QSPY_SEND_AO_FILTER:
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
        case QSPY_SHOW_NOTE: {
            QSPY_showNote(qrec);
            break;
        }
        case QSPY_CLEAR_SCREEN: {
            QSPY_command('c', CMD_OPT_OFF);
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
void BE_sendShortPkt(int pktId) {
    if ((pktId >= 128) || ((l_channels & BINARY_CH) != 0)) {
        uint8_t buf[4];
        uint8_t *pos = &buf[0];
        ++l_txBeSeq;
        *pos++ = l_txBeSeq;
        *pos++ = (uint8_t)pktId;
        PAL_send2FE(buf, (pos - &buf[0]));
    }
}
/*..........................................................................*/
void BE_sendLine(void) {
    if ((l_channels & TEXT_CH) != 0) {
        /* filter for permanently enabled QS records
         * that should NOT be forwarded to BE:
         * QS_EMPTY,
         * QS_SIG_DICT,
         * QS_OBJ_DICT,
         * QS_FUN_DICT,
         * QS_USR_DICT,
         * QS_ENUM_DICT,
         * QS_TARGET_INFO
         */
        static uint8_t const dont_forward[32] = {
            0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x40U, 0xF0U,
            0x01U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
            0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U,
            0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U
        };
        uint8_t rec = (uint8_t)QSPY_output.rec;

        /* should this QS record be forwarded? */
        if ((dont_forward[rec >> 3] & (1U << (rec & 7U))) == 0U) {
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
