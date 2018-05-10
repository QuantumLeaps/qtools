/**
* @file
* @brief QSPY transmit facilities
* @ingroup qpspy
* @cond
******************************************************************************
* Last updated for version 6.3.0
* Last updated on  2018-05-10
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2002-2018 Quantum Leaps, LLC. All rights reserved.
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
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include "qspy.h"
#include "pal.h"

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
static uint8_t       l_dstBuf[1024]; /* for encoding from FE to Target */
static uint8_t       l_txTargetSeq;  /* transmit Target sequence number */
static ObjType       l_currSM;  /* Current State Machine Object from FE */

/****************************************************************************/
/*! helper macro to insert an un-escaped byte into the QS buffer */
#define QS_INSERT_BYTE(b_) \
    *dst++ = (b_); \
    if ((uint8_t)(dst - &dstBuf[0]) >= dstSize) { \
        return 0U; \
    }

/*! helper macro to insert an escaped byte into the QS buffer */
#define QS_INSERT_ESC_BYTE(b_) \
    chksum = (uint8_t)(chksum + (b_)); \
    if (((b_) != QS_FRAME) && ((b_) != QS_ESC)) { \
        QS_INSERT_BYTE(b_) \
    } \
    else { \
        QS_INSERT_BYTE(QS_ESC) \
        QS_INSERT_BYTE((uint8_t)((b_) ^ QS_ESC_XOR)) \
    }

/*..........................................................................*/
uint32_t QSPY_encode(uint8_t *dstBuf, uint32_t dstSize,
                     uint8_t const *srcBuf, uint32_t srcBytes)
{
    uint8_t chksum = 0U;
    uint8_t b;

    uint8_t *dst = &dstBuf[0];
    uint8_t const *src = &srcBuf[1]; /* skip the sequence from the source */

    --srcBytes; /* account for skipping the sequence number in the source */

    /* supply the sequence number */
    ++l_txTargetSeq;
    b = l_txTargetSeq;
    QS_INSERT_ESC_BYTE(b); /* insert esceped sequence into destination */

    for (; srcBytes > 0; ++src, --srcBytes) {
        b = *src;
        QS_INSERT_ESC_BYTE(b) /* insert all escaped bytes into destination */
    }

    b = chksum;
    b ^= 0xFFU;               /* invert the bits of the checksum */
    QS_INSERT_ESC_BYTE(b)     /* insert the escaped checksum */
    QS_INSERT_BYTE(QS_FRAME)  /* insert un-escaped frame */

    return dst - &dstBuf[0];  /* number of bytes in the destination */
}
/*..........................................................................*/
uint32_t QSPY_encodeResetCmd(uint8_t *dstBuf, uint32_t dstSize) {
    static uint8_t const s_QS_RX_RESET[] = { 0x00U, QS_RX_RESET };
    return QSPY_encode(dstBuf, dstSize,
                       s_QS_RX_RESET, sizeof(s_QS_RX_RESET));
}
/*..........................................................................*/
uint32_t QSPY_encodeInfoCmd(uint8_t *dstBuf, uint32_t dstSize) {
    static uint8_t const s_QS_RX_INFO[] = { 0x00U, QS_RX_INFO };
    return QSPY_encode(dstBuf, dstSize,
                       s_QS_RX_INFO, sizeof(s_QS_RX_INFO));
}
/*..........................................................................*/
uint32_t QSPY_encodeTickCmd (uint8_t *dstBuf, uint32_t dstSize, uint8_t rate){
    uint8_t a_QS_RX_TICK[] = { 0x00U, QS_RX_TICK, 0U };
    a_QS_RX_TICK[2] = rate;
    return QSPY_encode(dstBuf, dstSize,
                       a_QS_RX_TICK, sizeof(a_QS_RX_TICK));
}
/*..........................................................................*/
void QSPY_sendEvt(QSpyRecord const * const qrec) {
    unsigned sigSize = QSPY_getConfig()->sigSize;
    unsigned n = 3U + sigSize;
    unsigned len = (qrec->start[n] | (qrec->start[n + 1U] << 8));

    n += (2U + len);
    if (n >= qrec->tot_len - 2U) {
        SNPRINTF_LINE("   <F-END> ERROR    "
                      "command 'SEND_EVENT' incorrect");
        QSPY_printError();
        return;
    }
    else {
        char const *name = (char const *)&qrec->start[n];
        SigType sig = QSPY_findSig(name, l_currSM);
        if (sig == (SigType)0) {
            SNPRINTF_LINE("   <F-END> ERROR    "
                          "Signal Dictionary not found for "
                          "Sig=%s", name);
            QSPY_printError();
        }
        else {
            uint32_t nBytes;
            uint8_t *evtPkt = (uint8_t *)&qrec->start[0]; /*cast const away*/

            evtPkt[0] = (uint8_t)0;
            evtPkt[1] = (uint8_t)QS_RX_EVENT;

            /* insert the found Signal 'sig' into the binary record */
            evtPkt[3] = (uint8_t)(sig & 0xFFU);
            if (sigSize >= 2) {
                sig >>= 8;
                evtPkt[4] = (uint8_t)(sig & 0xFFU);
                if (sigSize == 4) {
                    sig >>= 8;
                    evtPkt[5] = (uint8_t)(sig & 0xFFU);
                    sig >>= 8;
                    evtPkt[6] = (uint8_t)(sig & 0xFFU);
                }
            }

            /* encode the QS_RX_EVENT record to the Target */
            nBytes = QSPY_encode(l_dstBuf, sizeof(l_dstBuf), evtPkt, n);
            if (nBytes == 0) {
                SNPRINTF_LINE("   <COMMS> ERROR    Encoding QS_RX_EVENT");
                QSPY_onPrintLn();
            }
            else {
                 (void)(*PAL_vtbl.send2Target)(l_dstBuf, nBytes);
                 /* send2Target() reports error by itself */
            }
        }
    }
}
/*..........................................................................*/
void QSPY_sendObj(QSpyRecord const * const qrec) {
    unsigned objPtrSize = QSPY_getConfig()->objPtrSize;
    unsigned n = 3U + objPtrSize;
    char const *name = (char const *)&qrec->start[n];

    KeyType key = QSPY_findObj(name);

    if (key == (KeyType)0) {
        SNPRINTF_LINE("   <F-END> ERROR    Object Dictionary not found for "
                      "Name=%s", name);
        QSPY_printError();
    }
    else {
        uint32_t nBytes;
        uint8_t *objPkt = (uint8_t *)&qrec->start[0]; /* cast const away */

        objPkt[0] = (uint8_t)0;
        if (qrec->rec == QSPY_SEND_LOC_FILTER) {
            objPkt[1] = (uint8_t)QS_RX_LOC_FILTER;
        }
        else if (qrec->rec == QSPY_SEND_CURR_OBJ) {
            objPkt[1] = (uint8_t)QS_RX_CURR_OBJ;

            /* see enum QSpyObjKind in qs_copy.c */
            switch (qrec->start[2]) { /* object-kind */
                case SM_OBJ:
                case AO_OBJ:
                case SM_AO_OBJ:
                    l_currSM = (ObjType)key; /* store for QSPY_sendEvent() */
                    break;
                case MP_OBJ:
                case EQ_OBJ:
                case TE_OBJ:
                case AP_OBJ:
                    break;
                default:
                    SNPRINTF_LINE("   <F-END> ERROR    "
                                  "Incorrect Object Kind=%d", qrec->start[2]);
                    QSPY_printError();
                    break;
            }
        }
        else {
            Q_ASSERT(0); /* QSPY_sendObj() should not have been called */
        }

        if (objPtrSize >= 2) {
            objPkt[3] = (uint8_t)(key & 0xFFU); key >>= 8;
            objPkt[4] = (uint8_t)(key & 0xFFU); key >>= 8;
            if (objPtrSize >= 4) {
                objPkt[5] = (uint8_t)(key & 0xFFU); key >>= 8;
                objPkt[6] = (uint8_t)(key & 0xFFU); key >>= 8;
                if (objPtrSize == 8) {
                    objPkt[7] = (uint8_t)(key & 0xFFU); key >>= 8;
                    objPkt[8] = (uint8_t)(key & 0xFFU); key >>= 8;
                    objPkt[9] = (uint8_t)(key & 0xFFU); key >>= 8;
                    objPkt[10]= (uint8_t)(key & 0xFFU); key >>= 8;
                }
            }
        }

        nBytes = QSPY_encode(l_dstBuf, sizeof(l_dstBuf), objPkt, n);
        if (nBytes == 0) {
            SNPRINTF_LINE("   <COMMS> ERROR    Encoding QS_RX_LOC_FILTER");
            QSPY_onPrintLn();
        }
        else {
             (void)(*PAL_vtbl.send2Target)(l_dstBuf, nBytes);
             /* send2Target() reports error by itself */
        }
    }
}
/*..........................................................................*/
void QSPY_sendCmd(QSpyRecord const * const qrec) {
    unsigned n = 2U + 1U + 3U*4U;
    char const *name = (char const *)&qrec->start[n];

    KeyType key = QSPY_findUsr(name);

    if (key == (KeyType)0) {
        SNPRINTF_LINE("   <F-END> ERROR    User Dictionary not found for "
                      "Name=%s", name);
        QSPY_printError();
    }
    else {
        uint32_t nBytes;
        uint8_t *cmdPkt = (uint8_t *)&qrec->start[0]; /* cast const away */

        cmdPkt[0] = (uint8_t)0;
        cmdPkt[1] = (uint8_t)QS_RX_COMMAND;
        cmdPkt[2] = (uint8_t)key;

        nBytes = QSPY_encode(l_dstBuf, sizeof(l_dstBuf), cmdPkt, n);
        if (nBytes == 0) {
            SNPRINTF_LINE("   <COMMS> ERROR    Encoding QS_RX_COMMAND");
            QSPY_onPrintLn();
        }
        else {
             (void)(*PAL_vtbl.send2Target)(l_dstBuf, nBytes);
             /* send2Target() reports error by itself */
        }
    }
}
/*..........................................................................*/
void QSPY_sendTP(QSpyRecord const * const qrec) {
    unsigned funPtrSize = QSPY_getConfig()->funPtrSize;
    unsigned n = 2U + 4U + funPtrSize;
    char const *name = (char const *)&qrec->start[n];

    KeyType key = QSPY_findFun(name);

    if (key == (KeyType)0) {
        SNPRINTF_LINE("   <F-END> ERROR    Function Dictionary not found for "
                      "Name=%s", name);
        QSPY_printError();
    }
    else {
        uint32_t nBytes;
        uint8_t *tpPkt = (uint8_t *)&qrec->start[0]; /* cast const away */

        tpPkt[0] = (uint8_t)0;
        tpPkt[1] = (uint8_t)QS_RX_TEST_PROBE;

        if (funPtrSize >= 2) {
            tpPkt[6] = (uint8_t)(key & 0xFFU); key >>= 8;
            tpPkt[7] = (uint8_t)(key & 0xFFU); key >>= 8;
            if (funPtrSize >= 4) {
                tpPkt[8] = (uint8_t)(key & 0xFFU); key >>= 8;
                tpPkt[9] = (uint8_t)(key & 0xFFU); key >>= 8;
                if (funPtrSize == 8) {
                    tpPkt[10] = (uint8_t)(key & 0xFFU); key >>= 8;
                    tpPkt[11] = (uint8_t)(key & 0xFFU); key >>= 8;
                    tpPkt[12] = (uint8_t)(key & 0xFFU); key >>= 8;
                    tpPkt[13] = (uint8_t)(key & 0xFFU); key >>= 8;
                }
            }
        }

        nBytes = QSPY_encode(l_dstBuf, sizeof(l_dstBuf), tpPkt, n);
        if (nBytes == 0) {
            SNPRINTF_LINE("   <COMMS> ERROR    Encoding QS_RX_PROBE_POINT");
            QSPY_onPrintLn();
        }
        else {
             (void)(*PAL_vtbl.send2Target)(l_dstBuf, nBytes);
             /* send2Target() reports error by itself */
        }
    }
}
/*..........................................................................*/
void QSPY_txReset(void) {
    l_txTargetSeq = (uint8_t)0;
    l_currSM = (ObjType)(~0U); /* invalidate */
}
