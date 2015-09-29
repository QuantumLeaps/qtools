/**
* @file
* @brief Public QEP/C interface
* @ingroup qep
* @cond
******************************************************************************
* Product: QSPY -- Host API
* Last updated for version 5.5.0
* Last updated on  2015-09-17
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) Quantum Leaps, LLC. All rights reserved.
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
* http://www.state-machine.com
* mailto:info@state-machine.com
******************************************************************************
* @endcond
*/
#ifndef qspy_h
#define qspy_h

#define QSPY_VER "5.5.0"

#ifdef __cplusplus
extern "C" {
#endif

/*! low-level facilities for configuring QSpy and parsing QS records ...*/
typedef enum {
    QSPY_ERROR,
    QSPY_SUCCESS
} QSpyStatus;

/*! commands to QSPY; @sa "packet IDs" in qspy.tcl script */
typedef enum {
    ATTACH = 128,    /*!< attach to the QSPY Back-End */
    DETACH,          /*!< detach from the QSPY Back-End */
    SAVE_DIC,        /*!< save dictionaries to a file in QSPY */
    SCREEN_OUT,      /*!< toggle screen output to a file in QSPY */
    BIN_OUT,         /*!< toggle binary output to a file in QSPY */
    MATLAB_OUT,      /*!< toggle Matlab output to a file in QSPY */
    MSCGEN_OUT,      /*!< toggle MscGen output to a file in QSPY */
    /* ... */
} QSpyCommands;

/*! QSPY record being processed */
typedef struct {
    uint8_t const *start; /*!< start of the record */
    uint8_t const *pos;   /*!< current position in the stream */
    uint32_t tot_len;     /*!< total length of the record (including chksum) */
    int32_t  len;         /*!< current length of the stream */
    uint8_t  rec;         /*!< the record-ID (see enum QSpyRecords in qs.h) */
} QSpyRecord;

/*! QSPY configuration parameters. @sa QSPY_config() */
typedef struct {
    uint16_t version;
    uint8_t objPtrSize;
    uint8_t funPtrSize;
    uint8_t tstampSize;
    uint8_t sigSize;
    uint8_t evtSize;
    uint8_t queueCtrSize;
    uint8_t poolCtrSize;
    uint8_t poolBlkSize;
    uint8_t tevtCtrSize;
    uint8_t tstamp[6];
} QSpyConfig;

/* the largest valid QS record size [bytes] */
#define QS_MAX_RECORD_SIZE  1024

/* pointer to the callback function for customized QS record parsing  */
typedef int (*QSPY_CustParseFun)(QSpyRecord * const me);

void        QSpyRecord_init     (QSpyRecord * const me,
                                 uint8_t const *start, uint32_t tot_len);
QSpyStatus  QSpyRecord_OK       (QSpyRecord * const me);
uint32_t    QSpyRecord_getUint32(QSpyRecord * const me, uint8_t size);
int32_t     QSpyRecord_getInt32 (QSpyRecord * const me, uint8_t size);
uint64_t    QSpyRecord_getUint64(QSpyRecord * const me, uint8_t size);
int64_t     QSpyRecord_getInt64 (QSpyRecord * const me, uint8_t size);
char const *QSpyRecord_getStr   (QSpyRecord * const me);
uint8_t const *QSpyRecord_getMem(QSpyRecord * const me, uint32_t *pLen);

/* QSPY configuration and high-level interface .............................*/
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
void QSPY_configMatFile(void *matFile);
void QSPY_configMscFile(void *mscFile);

QSpyConfig const *QSPY_getConfig(void);

void QSPY_parse(uint8_t const *buf, uint32_t nBytes);


char const *QSPY_writeDict(void);
QSpyStatus QSPY_readDict(void *dictFile);
bool QSPY_command(uint8_t cmdId); /* execute an internal QSPY command */

uint32_t QSPY_encode(uint8_t *dstBuf, uint32_t dstSize,
                     uint8_t const *srcBuf, uint32_t srcBytes);
uint32_t QSPY_encodeResetCmd(uint8_t *dstBuf, uint32_t dstSize);
uint32_t QSPY_encodeInfoCmd (uint8_t *dstBuf, uint32_t dstSize);
uint32_t QSPY_encodeTickCmd (uint8_t *dstBuf, uint32_t dstSize, uint8_t rate);

void QSPY_stop(void); /* orderly close all used files */

extern char QSPY_line[];
void QSPY_onPrintLn(void); /* print formatted line callback function */

#ifdef __cplusplus
}
#endif

#endif /* qspy_h */
