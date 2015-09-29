/*****************************************************************************
* Product: QSPY -- Back-End connection point for the external Front-Ends
* Last updated for version 5.5.0
* Last updated on  2015-09-16
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) Quantum Leaps, www.state-machine.com.
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
* Web:   www.state-machine.com
* Email: info@state-machine.com
*****************************************************************************/
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

#ifndef Q_SPY
#define Q_SPY 1
#endif
#include "qs_copy.h" /* copy of the target-resident QS interface */

/*..........................................................................*/
static uint8_t  l_buf[1024];   /* the output buffer [bytes] */
static uint8_t *l_pos;         /* current position in the output buffer */
static uint8_t  l_rxBeSeq;     /* receive  Back-End  sequence number */
static uint8_t  l_txBeSeq;     /* transmit Back-End sequence number */
static uint8_t  l_attached;    /* attached status 0, 1, 2 */

/*..........................................................................*/
#ifndef NDEBUG
static FILE *l_testFile;
#endif

void BE_onStartup(void) {
    l_pos      = &l_buf[0];
    l_rxBeSeq  = 0U;
    l_txBeSeq  = 0U;
    l_attached = 0U;

#ifndef NDEBUG
    FOPEN_S(l_testFile, "fromFE.bin", "wb");
#endif
}
/*..........................................................................*/
void BE_onCleanup(void) {
    BE_initPkt(DETACH);
    BE_sendPkt();
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

    /* check the continuity of the data from Front-End... */
    if (l_attached > 0U) {
        ++l_rxBeSeq;
        if (buf[0] != l_rxBeSeq) {
            printf("!Back-End: data discontinuity: seq=%u -> seq=%u\n",
                   (unsigned)l_rxBeSeq, (unsigned)buf[0]);
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
                 fprintf(stderr,
                   "!Back-End: errors sending record to the Target\n");
            }
#ifndef NDEBUG
            if (l_testFile != (FILE *)0) {
                fwrite(qbuf, 1, len, l_testFile);
            }
#endif
        }
        else {
            fprintf(stderr,
                   "!Back-End: Target packet too big\n");
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
        case ATTACH: {   /* attach to the Front-End */
            if (l_attached == 0U) {  /* just getting attached? */
                printf(">Back-End: attached\n");
            }

            l_rxBeSeq  = qrec->start[0]; /* re-start the receive  sequence */
            l_txBeSeq  = 0U;             /* re-start the transmit sequence */
            l_attached = 1U;

            /* send the attach packet back to the Front-End */
            BE_initPkt(ATTACH);
            BE_sendPkt();

            break;
        }
        case DETACH: {   /* detach from the Front-End */
            PAL_detachFE();
            l_attached = 0U;
            printf(">Back-End: detached\n");
            break;
        }
        case SAVE_DIC: { /* save dictionaries collected so far */
            QSPY_command('d');
            break;
        }
        case SCREEN_OUT: {
            QSPY_command('o');
            break;
        }
        case BIN_OUT: {
            QSPY_command('s');
            break;
        }

        case MATLAB_OUT: {
            QSPY_command('m');
            break;
        }
        case MSCGEN_OUT: {
            QSPY_command('g');
            break;
        }

        default: {
            fprintf(stderr, "!Back-End: unrecognized command %d\n",
                            (int)qrec->rec);
            break;
        }
    }
}
/*..........................................................................*/
int BE_parseRecFromTarget(QSpyRecord * const qrec) {
    if (l_attached == 1U) {
        if (qrec->rec == QS_TARGET_INFO) {
            /* forward the Target Info record to the Front-End... */
            PAL_send2FE(qrec->start, qrec->tot_len - 1U);
            l_attached = 2U;
        }
    }
    else {
        /* forward the Target record to the Front-End... */
        PAL_send2FE(qrec->start, qrec->tot_len - 1U);
    }
    return 1; /* continue with the standard QSPY processing */
}

/*--------------------------------------------------------------------------*/
void BE_initPkt(int pktId) {
    l_pos = &l_buf[0];
    ++l_txBeSeq;
    *l_pos++ = l_txBeSeq;
    *l_pos++ = (uint8_t)pktId;
}
/*..........................................................................*/
void BE_sendPkt(void) {
    PAL_send2FE(l_buf, (l_pos - &l_buf[0]));
}

/*--------------------------------------------------------------------------*/
void BE_putU8(uint8_t d) {
    *l_pos++ = d;
}
/*..........................................................................*/
void BE_putU16(uint16_t d) {
    *l_pos++ = (uint8_t)d;
    d >>= 8;
    *l_pos++ = (uint8_t)d;
}
/*..........................................................................*/
void BE_putU32(uint16_t d) {
    *l_pos++ = (uint8_t)d;
    d >>= 8;
    *l_pos++ = (uint8_t)d;
    d >>= 8;
    *l_pos++ = (uint8_t)d;
    d >>= 8;
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
