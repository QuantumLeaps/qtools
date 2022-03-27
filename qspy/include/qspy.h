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
* @date Last updated on: 2022-03-12
* @version Last updated for version: 7.0.0
*
* @file
* @brief Host API
* @ingroup qpspy
*/
#ifndef QSPY_H
#define QSPY_H

#define QSPY_VER "7.0.0"

#ifdef __cplusplus
extern "C" {
#endif

/*! low-level facilities for configuring QSpy and parsing QS records ...*/
typedef enum {
    QSPY_ERROR,
    QSPY_SUCCESS
} QSpyStatus;

/*! QSPY record being processed */
typedef struct {
    uint8_t const *start; /*!< start of the record */
    uint8_t const *pos;   /*!< current position in the stream */
    uint32_t tot_len;     /*!< total length of the record, including chksum */
    int32_t  len;         /*!< current length of the stream */
    uint8_t  rec;         /*!< the record-ID (see enum QSpyRecords in qs.h) */
} QSpyRecord;

/* limits */
enum {
    QS_RECORD_SIZE_MAX  = 512,  /* max QS record size [bytes] */
    QS_LINE_LEN_MAX     = 1000, /* max length of a QSPY line [chars] */
    QS_FNAME_LEN_MAX    = 256,  /* max length of filenames [chars] */
    QS_SEQ_LIST_LEN_MAX = 300,  /* max length of the Seq list [chars] */
    QS_DNAME_LEN_MAX    = 64,   /* max dictionary name length [chars] */
};

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
/*! QSPY configuration parameters. @sa QSPY_config() */
typedef struct {
    uint16_t version;
    uint8_t endianness;
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

void QSPY_config(QSpyConfig const *config,
                 QSPY_CustParseFun custParseFun);
void QSPY_configTxReset(QSPY_resetFun txResetFun);
void QSPY_configMatFile(void *matFile);

void QSPY_reset(void);
void QSPY_parse(uint8_t const *buf, uint32_t nBytes);
void QSPY_txReset(void);

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

void QSPY_cleanup(void); /* cleanup after the run */

char const* QSPY_tstampStr(void);

void QSPY_onPrintLn(void); /* callback to print the last line of output */

/* prints information message to the QSPY output (without sending it to FE) */
void QSPY_printInfo(void);

/* prints error message to the QSPY output (sending it to FE) */
void QSPY_printError(void);

/* last human-readable line of output from QSPY ............................*/
#define QS_LINE_OFFSET  8
enum QSPY_LastOutputType { REG_OUT, INF_OUT, ERR_OUT };
typedef struct {
    char buf[QS_LINE_OFFSET + QS_LINE_LEN_MAX];
    int  len;  /* the length of the composed string */
    int  rec;  /* the corresponding QS record ID */
    int  type; /* the type of the output */
    int  rx_status; /* the type of the RX status */
} QSPY_LastOutput;

enum QSRreRecGroup {
    GRP_ERR,
    GRP_INF,
    GRP_DIC,
    GRP_TST,
    GRP_SM,
    GRP_AO,
    GRP_EQ,
    GRP_MP,
    GRP_TE,
    GRP_QF,
    GRP_SC,
    GRP_USR
};

typedef struct {
    char const *name; /* name of the record, e.g. "QS_QF_PUBLISH" */
    int  const group; /* group of the record (for rendering/coloring) */
} QSpyRecRender;

/* last output generated */
extern QSPY_LastOutput QSPY_output;

/* begining of QSPY line to print */
extern char const * const QSPY_line;

/* rendering information for QSPY records */
extern QSpyRecRender const QSPY_rec[];

#define SNPRINTF_LINE(format_, ...) do {                       \
    int n_ = SNPRINTF_S(&QSPY_output.buf[QS_LINE_OFFSET],      \
                (QS_LINE_LEN_MAX - QS_LINE_OFFSET),            \
                format_,  ##__VA_ARGS__);                      \
    if ((0 < n_) && (n_ < QS_LINE_LEN_MAX - QS_LINE_OFFSET)) { \
        QSPY_output.len = n_;                                  \
    }                                                          \
    else {                                                     \
        QSPY_output.len = QS_LINE_LEN_MAX - QS_LINE_OFFSET;    \
    }                                                          \
} while (0)

#define SNPRINTF_APPEND(format_, ...) do {                                 \
    int n_ = SNPRINTF_S(&QSPY_output.buf[QS_LINE_OFFSET + QSPY_output.len],\
                (QS_LINE_LEN_MAX - QS_LINE_OFFSET - QSPY_output.len),      \
                format_, ##__VA_ARGS__);                                   \
    if ((0 < n_)                                                           \
        && (n_ < QS_LINE_LEN_MAX - QS_LINE_OFFSET - QSPY_output.len)) {    \
        QSPY_output.len += n_;                                             \
    }                                                                      \
    else {                                                                 \
        QSPY_output.len = QS_LINE_LEN_MAX - QS_LINE_OFFSET;                \
    }                                                                      \
} while (0)

#define CONFIG_UPDATE(member_, new_, diff_) \
    if (QSPY_conf.member_ != (new_)) {      \
        QSPY_conf.member_ =  (new_);        \
        (diff_) = 1U;                       \
    } else (void)0

/* Dictionaries ............................................................*/
typedef struct {
    KeyType key;
    char    name[QS_DNAME_LEN_MAX];
} DictEntry;

typedef struct {
    DictEntry  notFound;
    DictEntry* sto;
    int        capacity;
    int        entries;
    int        keySize;
} Dictionary;

void Dictionary_ctor(Dictionary* const me,
    DictEntry* sto, uint32_t capacity);
void Dictionary_config(Dictionary* const me, int keySize);
char const* Dictionary_at(Dictionary* const me, unsigned idx);
void Dictionary_put(Dictionary* const me, KeyType key, char const* name);
char const* Dictionary_get(Dictionary* const me, KeyType key, char* buf);
int Dictionary_find(Dictionary* const me, KeyType key);
KeyType Dictionary_findKey(Dictionary* const me, char const* name);
void Dictionary_reset(Dictionary* const me);

typedef struct SigDictEntryTag {
    SigType sig;
    ObjType obj;
    char    name[QS_DNAME_LEN_MAX];
} SigDictEntry;

typedef struct SigDictionaryTag {
    SigDictEntry  notFound;
    SigDictEntry* sto;
    int           capacity;
    int           entries;
    int           ptrSize;
} SigDictionary;

void SigDictionary_ctor(SigDictionary* const me,
    SigDictEntry* sto, uint32_t capacity);
void SigDictionary_config(SigDictionary* const me, int ptrSize);
void SigDictionary_put(SigDictionary* const me,
    SigType sig, ObjType obj, char const* name);
char const* SigDictionary_get(SigDictionary* const me,
                              SigType sig, ObjType obj, char* buf);
int SigDictionary_find(SigDictionary* const me,
                       SigType sig, ObjType obj);
SigType SigDictionary_findSig(SigDictionary* const me,
                             char const* name, ObjType obj);
void SigDictionary_reset(SigDictionary* const me);
void QSPY_resetAllDictionaries(void);

/*==========================================================================*/
/* facilities used by the QSPY host app only (but not for QSPY parser) */
#ifdef QSPY_APP

/*! commands to QSPY; @sa "packet IDs" in qspy.tcl script */
typedef enum {
    QSPY_ATTACH = 128,    /*!< attach to the QSPY Back-End */
    QSPY_DETACH,          /*!< detach from the QSPY Back-End */
    QSPY_SAVE_DICT,       /*!< save dictionaries to a file in QSPY */
    QSPY_SCREEN_OUT,      /*!< toggle screen output to a file in QSPY */
    QSPY_BIN_OUT,         /*!< toggle binary output to a file in QSPY */
    QSPY_MATLAB_OUT,      /*!< toggle Matlab output to a file in QSPY */
    QSPY_SEQUENCE_OUT,    /*!< toggle Sequence output to a file in QSPY */
    QSPY_SEND_EVENT,      /*!< send event (QSPY supplying signal) */
    QSPY_SEND_AO_FILTER,  /*!< send Local Filter (QSPY supplying addr) */
    QSPY_SEND_CURR_OBJ,   /*!< send current Object (QSPY supplying addr) */
    QSPY_SEND_COMMAND,    /*!< send command (QSPY supplying cmdId) */
    QSPY_SEND_TEST_PROBE  /*!< send Test-Probe (QSPY supplying apiId) */
    /* ... */
} QSpyCommands;

extern QSpyConfig    QSPY_conf;
extern Dictionary    QSPY_funDict;
extern Dictionary    QSPY_objDict;
extern Dictionary    QSPY_usrDict;
extern SigDictionary QSPY_sigDict;

void QSPY_setExternDict(char const* dictName);
QSpyStatus QSPY_readDict(void);
QSpyStatus QSPY_writeDict(void);

bool QDIC_isActive(void);

void Dictionary_write(Dictionary const* const me, FILE* stream);
bool Dictionary_read(Dictionary* const me, FILE* stream);

void SigDictionary_write(SigDictionary const* const me, FILE* stream);
bool SigDictionary_read(SigDictionary* const me, FILE* stream);
char const* QSPY_getMatDict(char const* s);

void QSEQ_configFile(void *seqFile);
bool QSEQ_isActive(void);
void QSEQ_config(void* seqFile, const char* seqList);
void QSEQ_updateDictionary(char const* name, KeyType key);
int  QSEQ_find(KeyType key);
void QSEQ_genHeader(void);
void QSEQ_genPost(uint32_t tstamp, int src, int dst, char const* sig,
                bool isAttempt);
void QSEQ_genPostLIFO(uint32_t tstamp, int src, char const* sig);
void QSEQ_genTran(uint32_t tstamp, int obj, char const* state);
void QSEQ_genPublish(uint32_t tstamp, int obj, char const* sig);
void QSEQ_genAnnotation(uint32_t tstamp, int obj, char const* ann);
void QSEQ_genTick(uint32_t rate, uint32_t nTick);
void QSEQ_dictionaryReset(void);

#endif /* QSPY_APP */

#ifdef __cplusplus
}
#endif

#endif /* QSPY_H */
