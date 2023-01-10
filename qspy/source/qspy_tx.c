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
* @date Last updated on: 2023-01-11
* @version Last updated for version: 7.2.1
*
* @file
* @brief QSPY transmit facilities
*/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#include "safe_std.h" /* "safe" <stdio.h> and <string.h> facilities */
#include "qspy.h"     /* QSPY data parser */
#include "pal.h"      /* Platform Abstraction Layer */

#define Q_SPY   1       /* this is QP implementation */
#define QP_IMPL 1       /* this is QP implementation */
#include "qpc_qs.h"     /* QS target-resident interface */
#include "qpc_qs_pkg.h" /* QS package-scope interface */

/*..........................................................................*/
static uint8_t   l_dstBuf[1024]; /* for encoding from FE to Target */
static uint8_t   l_txTargetSeq;  /* transmit Target sequence number */
static ObjType   l_currSM;       /* current State Machine Object from FE */

/****************************************************************************/
/*! helper macro to insert an un-escaped byte into the QSPY buffer */
#define QSPY_INSERT_BYTE(b_)                      \
    *dst++ = (b_);                                \
    if ((uint8_t)(dst - &dstBuf[0]) >= dstSize) { \
        return 0U;                                \
    }

/*! helper macro to insert an escaped byte into the QS buffer */
#define QSPY_INSERT_ESC_BYTE(b_)                  \
    chksum = (uint8_t)(chksum + (b_));            \
    if (((b_) != QS_FRAME) && ((b_) != QS_ESC)) { \
        QSPY_INSERT_BYTE(b_)                      \
    }                                             \
    else {                                        \
        QSPY_INSERT_BYTE(QS_ESC)                  \
        QSPY_INSERT_BYTE((uint8_t)((b_) ^ QS_ESC_XOR)) \
    }

/*..........................................................................*/
uint32_t QSPY_encode(uint8_t *dstBuf, uint32_t dstSize,
                   uint8_t const *srcBuf, uint32_t srcBytes)
{
    uint8_t chksum = 0U;
    uint8_t *dst = &dstBuf[0];
    uint8_t const *src = &srcBuf[1]; /* skip the sequence from the source */

    --srcBytes; /* account for skipping the sequence number in the source */

    /* supply the sequence number */
    ++l_txTargetSeq;
    uint8_t b = l_txTargetSeq;
    QSPY_INSERT_ESC_BYTE(b); /* insert esceped sequence */

    for (; srcBytes > 0U; ++src, --srcBytes) {
        b = *src;
        QSPY_INSERT_ESC_BYTE(b) /* insert all escaped bytes */
    }

    b = chksum;
    b ^= 0xFFU;                /* invert the bits of the checksum */
    QSPY_INSERT_ESC_BYTE(b)    /* insert the escaped checksum */
    QSPY_INSERT_BYTE(QS_FRAME) /* insert un-escaped frame */

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
    unsigned sigSize = QSPY_conf.sigSize;
    unsigned n = 3U + sigSize;
    unsigned len = (qrec->start[n] | (qrec->start[n + 1U] << 8));

    n += (2U + len);
    if (n >= qrec->tot_len - 2U) {
        SNPRINTF_LINE("   <F-END> ERROR    %s",
                      "command 'SEND_EVENT' incorrect");
        QSPY_printError();
        return;
    }
    else {
        char const *name = (char const *)&qrec->start[n];
        SigType sig = QSPY_findSig(name, l_currSM);
        if (sig == SIG_NOT_FOUND) {
            SNPRINTF_LINE("   <F-END> ERROR    "
                          "Signal Dictionary not found for Sig=%s", name);
            QSPY_printError();
        }
        else {
            uint32_t nBytes;
            uint8_t *evtPkt = (uint8_t *)&qrec->start[0]; /*cast const away*/

            evtPkt[0] = 0U;
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
                SNPRINTF_LINE("   <COMMS> ERROR    %s",
                              "Encoding QS_RX_EVENT");
                QSPY_printError();
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
    unsigned objPtrSize = QSPY_conf.objPtrSize;
    unsigned n = 3U + objPtrSize;
    char const *name = (char const *)&qrec->start[n];

    KeyType key = QSPY_findObj(name);

    if (key == KEY_NOT_FOUND) {
        SNPRINTF_LINE("   <F-END> ERROR    Object Dictionary not found for "
                      "Name=%s", name);
        QSPY_printError();
    }
    else {
        uint32_t nBytes;
        uint8_t *objPkt = (uint8_t *)&qrec->start[0]; /* cast const away */

        objPkt[0] = 0U;
        if (qrec->rec == QSPY_SEND_AO_FILTER) {
            objPkt[1] = (uint8_t)QS_RX_AO_FILTER;
        }
        else if (qrec->rec == QSPY_SEND_CURR_OBJ) {
            objPkt[1] = (uint8_t)QS_RX_CURR_OBJ;

            /* see enum QSpyObjKind in qs_copy.h */
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
            SNPRINTF_LINE("   <COMMS> ERROR    %s",
                          "Encoding QS_RX_CURR_OBJ");
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

    KeyType key;
    if (QSPY_conf.version < 714U) {
        key = QSPY_findUsr(name);
    }
    else {
        key = QSPY_findEnum(name, QS_CMD);
    }

    if (key == KEY_NOT_FOUND) {
        SNPRINTF_LINE("   <F-END> ERROR    Command Dictionary not found for "
                      "Name=%s", name);
        QSPY_printError();
        return;
    }

    uint32_t nBytes;
    uint8_t *cmdPkt = (uint8_t *)&qrec->start[0]; /* cast const away */

    cmdPkt[0] = 0U;
    cmdPkt[1] = (uint8_t)QS_RX_COMMAND;
    cmdPkt[2] = (uint8_t)key;

    nBytes = QSPY_encode(l_dstBuf, sizeof(l_dstBuf), cmdPkt, n);
    if (nBytes == 0) {
        SNPRINTF_LINE("   <COMMS> ERROR    %s",
                      "Encoding QS_RX_COMMAND");
        QSPY_onPrintLn();
    }
    else {
         (void)(*PAL_vtbl.send2Target)(l_dstBuf, nBytes);
         /* send2Target() reports error by itself */
    }
}
/*..........................................................................*/
void QSPY_sendTP(QSpyRecord const * const qrec) {
    unsigned funPtrSize = QSPY_conf.funPtrSize;
    unsigned n = 2U + 4U + funPtrSize;
    char const *name = (char const *)&qrec->start[n];

    KeyType key = QSPY_findFun(name);

    if (key == KEY_NOT_FOUND) {
        SNPRINTF_LINE("   <F-END> ERROR    Function Dictionary not found for "
                      "Name=%s", name);
        QSPY_printError();
    }
    else {
        uint32_t nBytes;
        uint8_t *tpPkt = (uint8_t *)&qrec->start[0]; /* cast const away */

        tpPkt[0] = 0U;
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
            SNPRINTF_LINE("   <COMMS> ERROR    %s",
                          "Encoding QS_RX_PROBE_POINT");
            QSPY_onPrintLn();
        }
        else {
             (void)(*PAL_vtbl.send2Target)(l_dstBuf, nBytes);
             /* send2Target() reports error by itself */
        }
    }
}
/*..........................................................................*/
void QSPY_showNote(QSpyRecord const * const qrec) {
    char const *note = (char const *)&qrec->start[3];
    SNPRINTF_LINE("%s", note);
    switch (qrec->start[2]) {
        case 0xFFU: QSPY_output.type = TST_OUT; break;
        default:    QSPY_output.type = USR_OUT; break;
    }
    QSPY_onPrintLn();
}
/*..........................................................................*/
void QSPY_txReset(void) {
    l_txTargetSeq = 0U;
    l_currSM = (ObjType)(~0U); /* invalidate */
}
