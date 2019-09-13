/**
* @file
* @brief Host API
* @ingroup qpspy
* @cond
******************************************************************************
* Last updated for version 6.6.0
* Last updated on  2019-09-12
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005-2019 Quantum Leaps, LLC. All rights reserved.
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
* <www.state-machine.com>
* <info@state-machine.com>
******************************************************************************
* @endcond
*/
#ifndef QSPY_H
#define QSPY_H

#define QSPY_VER "6.6.0"

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
    QSPY_ATTACH = 128,    /*!< attach to the QSPY Back-End */
    QSPY_DETACH,          /*!< detach from the QSPY Back-End */
    QSPY_SAVE_DICT,       /*!< save dictionaries to a file in QSPY */
    QSPY_SCREEN_OUT,      /*!< toggle screen output to a file in QSPY */
    QSPY_BIN_OUT,         /*!< toggle binary output to a file in QSPY */
    QSPY_MATLAB_OUT,      /*!< toggle Matlab output to a file in QSPY */
    QSPY_MSCGEN_OUT,      /*!< toggle MscGen output to a file in QSPY */
    QSPY_SEND_EVENT,      /*!< send event (QSPY supplying signal) */
    QSPY_SEND_LOC_FILTER, /*!< send Local Filter (QSPY supplying addr) */
    QSPY_SEND_CURR_OBJ,   /*!< send current Object (QSPY supplying addr) */
    QSPY_SEND_COMMAND,    /*!< send command (QSPY supplying cmdId) */
    QSPY_SEND_TEST_PROBE  /*!< send Test-Probe (QSPY supplying apiId) */
    /* ... */
} QSpyCommands;

/*! QSPY record being processed */
typedef struct {
    uint8_t const *start; /*!< start of the record */
    uint8_t const *pos;   /*!< current position in the stream */
    uint32_t tot_len;     /*!< total length of the record, including chksum */
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

typedef uint64_t KeyType;
typedef uint32_t SigType;
typedef uint64_t ObjType;

/* the largest valid QS record size [bytes] */
#define QS_MAX_RECORD_SIZE  512

/* the maximum length of a single QSPY line [chars] */
#define QS_MAX_LINE_LENGTH  1000

/* pointer to the callback function for customized QS record parsing  */
typedef int (*QSPY_CustParseFun)(QSpyRecord * const me);
typedef void (*QSPY_resetFun)(void);

void        QSpyRecord_init     (QSpyRecord * const me,
                                 uint8_t const *start, uint32_t tot_len);
QSpyStatus  QSpyRecord_OK       (QSpyRecord * const me);
uint32_t    QSpyRecord_getUint32(QSpyRecord * const me, uint8_t size);
int32_t     QSpyRecord_getInt32 (QSpyRecord * const me, uint8_t size);
uint64_t    QSpyRecord_getUint64(QSpyRecord * const me, uint8_t size);
int64_t     QSpyRecord_getInt64 (QSpyRecord * const me, uint8_t size);
char const *QSpyRecord_getStr   (QSpyRecord * const me);
uint8_t const *QSpyRecord_getMem(QSpyRecord * const me,
                                 uint8_t size,
                                 uint32_t *pNum);

/* QSPY configuration and high-level interface .............................*/
void QSPY_config(
    uint16_t version,
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
QSpyConfig const *QSPY_getConfig(void);
void QSPY_configTxReset(QSPY_resetFun txResetFun);

void QSPY_configMatFile(void *matFile);
void QSPY_configMscFile(void *mscFile);

void QSPY_reset(void);
void QSPY_parse(uint8_t const *buf, uint32_t nBytes);
void QSPY_txReset(void);

void QSPY_setExternDict(char const *dictName);
QSpyStatus QSPY_readDict(void);
QSpyStatus QSPY_writeDict(void);

bool QSPY_command(uint8_t cmdId); /* execute an internal QSPY command */
void QSPY_sendEvt(QSpyRecord const * const qrec);
void QSPY_sendObj(QSpyRecord const * const qrec);
void QSPY_sendCmd(QSpyRecord const * const qrec);
void QSPY_sendTP (QSpyRecord const * const qrec);

uint32_t QSPY_encode(uint8_t *dstBuf, uint32_t dstSize,
                     uint8_t const *srcBuf, uint32_t srcBytes);
uint32_t QSPY_encodeResetCmd(uint8_t *dstBuf, uint32_t dstSize);
uint32_t QSPY_encodeInfoCmd (uint8_t *dstBuf, uint32_t dstSize);
uint32_t QSPY_encodeTickCmd (uint8_t *dstBuf, uint32_t dstSize, uint8_t rate);

SigType QSPY_findSig(char const *name, ObjType obj);
KeyType QSPY_findObj(char const *name);
KeyType QSPY_findFun(char const *name);
KeyType QSPY_findUsr(char const *name);

void QSPY_stop(void); /* orderly close all used files */

/* last human-readable line of output from QSPY */
#define QS_LINE_OFFSET  8
enum QSPY_LastOutputType { REG_OUT, INF_OUT, ERR_OUT };
typedef struct {
    char buf[QS_LINE_OFFSET + QS_MAX_LINE_LENGTH];
    int  len;  /* the length of the composed string */
    int  rec;  /* the corresponding QS record ID */
    int  type; /* the type of the output */
} QSPY_LastOutput;

extern QSPY_LastOutput QSPY_output;

void QSPY_onPrintLn(void); /* callback to print the last line of output */

#define SNPRINTF_LINE(format_, ...) do {                        \
    int n = SNPRINTF_S(&QSPY_output.buf[QS_LINE_OFFSET],        \
                (QS_MAX_LINE_LENGTH - QS_LINE_OFFSET),          \
                format_,  ##__VA_ARGS__);                       \
    if ((0 < n) && (n < QS_MAX_LINE_LENGTH - QS_LINE_OFFSET)) { \
        QSPY_output.len = n;                                    \
    }                                                           \
    else {                                                      \
        QSPY_output.len = QS_MAX_LINE_LENGTH - QS_LINE_OFFSET;  \
    }                                                           \
} while (0)

#define SNPRINTF_APPEND(format_, ...) do {                                 \
    int n = SNPRINTF_S(&QSPY_output.buf[QS_LINE_OFFSET + QSPY_output.len], \
                (QS_MAX_LINE_LENGTH - QS_LINE_OFFSET - QSPY_output.len),   \
                format_, ##__VA_ARGS__);                                   \
    if ((0 < n)                                                            \
        && (n < QS_MAX_LINE_LENGTH - QS_LINE_OFFSET - QSPY_output.len)) {  \
        QSPY_output.len += n;                                              \
    }                                                                      \
    else {                                                                 \
        QSPY_output.len = QS_MAX_LINE_LENGTH - QS_LINE_OFFSET;             \
    }                                                                      \
} while (0)

/* prints information message to the QSPY output (without sending it to FE) */
void QSPY_printInfo(void);

/* prints error message to the QSPY output (sending it to FE) */
void QSPY_printError(void);

/* for backwards compatibility */
#define QSPY_line (&QSPY_output.buf[QS_LINE_OFFSET])

#ifdef __cplusplus
}
#endif

#endif /* QSPY_H */
