/*****************************************************************************
* Product: Quantum Spy -- Host application interface
* Last updated for version 5.3.0
* Last updated on  2014-03-14
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
#ifndef qspy_h
#define qspy_h

#define QSPY_VER "5.3.0"

#ifdef __cplusplus
extern "C" {
#endif

/* low-level facilities for parsing QSpyRecords ............................*/
typedef struct QSpyRecordTag {
    uint8_t rec;                           /* enumerated type of the record */
    uint8_t const *pos;                   /* current position in the stream */
    int32_t len;       /* current length of the stream (till the last byte) */
} QSpyRecord;

void     QSpyRecord_ctor (QSpyRecord * const me,
                          uint8_t rec, uint8_t const *pos, int32_t len);
int      QSpyRecord_OK          (QSpyRecord * const me);
uint32_t QSpyRecord_getUint32   (QSpyRecord * const me, uint8_t size);
int32_t  QSpyRecord_getInt32    (QSpyRecord * const me, uint8_t size);
uint64_t QSpyRecord_getUint64   (QSpyRecord * const me, uint8_t size);
int64_t  QSpyRecord_getInt64    (QSpyRecord * const me, uint8_t size);
char const    *QSpyRecord_getStr(QSpyRecord * const me);
uint8_t const *QSpyRecord_getMem(QSpyRecord * const me, uint8_t *pl);

/* QSPY configuration and high-level interface .............................*/
typedef int (*QSPY_CustParseFun)(QSpyRecord * const me);

void QSPY_config(uint16_t version,
                 uint8_t objPtrSize,
                 uint8_t funPtrSize,
                 uint8_t tstampSize,
                 uint8_t sigSize,
                 uint8_t evtSize,
                 uint8_t queueCtrSize,
                 uint8_t poolCtrSize,
                 uint8_t poolBlkSize,
                 uint8_t tevtCtrSize,
                 void   *matFile,
                 void   *mscFile,
                 QSPY_CustParseFun custParseFun);

void QSPY_parse(uint8_t const *buf, uint32_t nBytes);
void QSPY_stop(void);                       /* orderly close all used files */

extern char QSPY_line[];
void QSPY_onPrintLn(void);        /* print formatted line callback function */

#ifdef __cplusplus
}
#endif

#endif                                                            /* qspy_h */
