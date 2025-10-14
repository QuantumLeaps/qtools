//============================================================================
// QSPY software tracing host-side utility
//
//                   Q u a n t u m  L e a P s
//                   ------------------------
//                   Modern Embedded Software
//
// Copyright(C) 2005 Quantum Leaps, LLC.All rights reserved.
//
// This software is licensed under the terms of the Quantum Leaps
// QSPY SOFTWARE TRACING HOST UTILITY SOFTWARE END USER LICENSE.
// Please see the file LICENSE-qspy.txt for the complete license text.
//
// Quantum Leaps contact information :
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//============================================================================
#ifndef QSPY_H_
#define QSPY_H_

#define QSPY_VER "8.1.2"

#ifdef __cplusplus
extern "C" {
#endif

// low-level facilities for configuring QSpy and parsing QS records .....
typedef enum {
    QSPY_ERROR,
    QSPY_SUCCESS
} QSpyStatus;

// typedef for inclusion of qpc_qs.h
typedef uint16_t QSignal;

// QSPY record being processed
typedef struct {
    uint8_t const *start; // start of the record
    uint8_t const *pos;   // current position in the stream
    uint32_t tot_len;     // total length of the record, including chksum
    int32_t  len;         // current length of the stream
    uint8_t  rec;         // the record-ID (see enum QSpyRecords in qs.h)
} QSpyRecord;

// limits
enum {
    QS_MIN_VERSION      = 660,  // minimum required version
    QS_RECORD_SIZE_MAX  = 512,  // max QS record size [bytes]
    QS_LINE_LEN_MAX     = 65528, // max length of a QSPY line [chars]
    QS_FNAME_LEN_MAX    = 256,  // max length of filenames [chars]
    QS_SEQ_LIST_LEN_MAX = 1024, // max length of the Seq list [chars]
    QS_DNAME_LEN_MAX    = 128,  // max dictionary name length [chars]
};

// pointer to the callback function for customized QS record parsing
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

// QSPY configuration and high-level interface ...............................
// QSPY configuration parameters. @sa QSPY_config()
typedef struct {
    uint32_t qpDate;    // e.g., 241008, 0 means "no-target-info"
    uint16_t qpVersion; // e.g., 732
    uint8_t  qpType;    // 1==qpc, 2==qpcpp
    uint8_t  endianness;
    uint8_t  objPtrSize;
    uint8_t  funPtrSize;
    uint8_t  tstampSize;
    uint8_t  sigSize;
    uint8_t  evtSize;
    uint8_t  queueCtrSize;
    uint8_t  poolCtrSize;
    uint8_t  poolBlkSize;
    uint8_t  tevtCtrSize;
    uint8_t  tbuild[6];
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

// command options
enum {
    CMD_OPT_OFF,
    CMD_OPT_ON,
    CMD_OPT_TOGGLE,
};
bool QSPY_command(uint8_t cmdId, uint8_t opt);

void QSPY_sendEvt (QSpyRecord const * const qrec);
void QSPY_sendObj (QSpyRecord const * const qrec);
void QSPY_sendCmd (QSpyRecord const * const qrec);
void QSPY_sendTP  (QSpyRecord const * const qrec);
void QSPY_showNote(QSpyRecord const * const qrec);

uint32_t QSPY_encode(uint8_t *dstBuf, uint32_t dstSize,
                     uint8_t const *srcBuf, uint32_t srcBytes);
uint32_t QSPY_encodeResetCmd(uint8_t *dstBuf, uint32_t dstSize);
uint32_t QSPY_encodeInfoCmd (uint8_t *dstBuf, uint32_t dstSize);
uint32_t QSPY_encodeTickCmd (uint8_t *dstBuf, uint32_t dstSize,
                             uint8_t rate);

SigType QSPY_findSig(char const *name, ObjType obj);
KeyType QSPY_findObj(char const *name);
KeyType QSPY_findFun(char const *name);
KeyType QSPY_findUsr(char const *name);
KeyType QSPY_findEnum(char const *name, uint8_t group);

#define SIG_NOT_FOUND ((SigType)-1)
#define KEY_NOT_FOUND ((KeyType)-1)

void QSPY_cleanup(void); // cleanup after the run

char const* QSPY_tstampStr(void);

void QSPY_onPrintLn(void); // callback to print the last line of output

// prints information message to the QSPY output (without sending it to FE)
void QSPY_printInfo(void);

// prints error message to the QSPY output (sending it to FE)
void QSPY_printError(void);

// last human-readable line of output from QSPY ..............................
#define QS_LINE_OFFSET  8
enum QSPY_LastOutputType {
    // output forwarded to the back-end...
    REG_OUT, // regular output from the Target
    ERR_OUT, // error output from QSPY
    // ...
    BE_OUT,  // last message forwarded to the back-end

    // output NOT forwarded to the back-end...
    INF_OUT, // internal info from QSPY
    USR_OUT, // generic user message from BE
    TST_OUT, // test message from BE
};
typedef struct {
    char buf[QS_LINE_OFFSET + QS_LINE_LEN_MAX];
    int  len;  // the length of the composed string
    int  rec;  // the corresponding QS record ID
    int  type; // the type of the output
    int  rx_status; // the type of the RX status
} QSPY_LastOutput;

// Record groups (extend enum QS_Groups from qpc_qs.h)
enum QSpyGroups_plus {
    QSPY_GRP_ERR,
    QSPY_GRP_INF,
    QSPY_GRP_DIC,
    QSPY_GRP_TST,
};

// returns the "group" of a given QS record-ID
int QSPY_getGroup(int recId);

// last output generated
extern QSPY_LastOutput QSPY_output;

// beginning of QSPY line to print
extern char const * const QSPY_line;

#define SNPRINTF_LINE(format_, ...) do {                       \
    int n_ = SNPRINTF_S(&QSPY_output.buf[QS_LINE_OFFSET],      \
                (QS_LINE_LEN_MAX - QS_LINE_OFFSET),            \
                format_,  __VA_ARGS__);                        \
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
                format_, __VA_ARGS__);                                     \
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

// Dictionaries ..............................................................
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

typedef struct {
    SigType sig;
    ObjType obj;
    char    name[QS_DNAME_LEN_MAX];
} SigDictEntry;

typedef struct {
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

// simplified string_copy() implementation "good enough" for the intended use
int string_copy(char *dest, size_t dest_size, char const *src);

//============================================================================
// facilities used by the QSPY host app only (but not for QSPY parser)
#ifdef QSPY_APP

// commands to QSPY; @sa "packet IDs" in qutest.py or qview.py scripts
typedef enum {
    QSPY_ATTACH = 128,    // attach to the QSPY Back-End
    QSPY_DETACH,          // detach from the QSPY Back-End
    QSPY_SAVE_DICT,       // save dictionaries to a file in QSPY
    QSPY_TEXT_OUT,        // toggle text output to a file in QSPY
    QSPY_BIN_OUT,         // toggle binary output to a file in QSPY
    QSPY_MATLAB_OUT,      // toggle Matlab output to a file in QSPY
    QSPY_SEQUENCE_OUT,    // toggle Sequence output to a file in QSPY
    QSPY_SEND_EVENT,      // send event (QSPY supplying signal)
    QSPY_SEND_AO_FILTER,  // send Local Filter (QSPY supplying addr)
    QSPY_SEND_CURR_OBJ,   // send current Object (QSPY supplying addr)
    QSPY_SEND_COMMAND,    // send command (QSPY supplying cmdId)
    QSPY_SEND_TEST_PROBE, // send Test-Probe (QSPY supplying apiId)
    QSPY_CLEAR_SCREEN,    // clear the QSPY screen
    QSPY_SHOW_NOTE,       // show a note in QSPY output
    // ...
} QSpyCommands;

extern QSpyConfig    QSPY_conf;
extern Dictionary    QSPY_funDict;
extern Dictionary    QSPY_objDict;
extern Dictionary    QSPY_usrDict;
extern SigDictionary QSPY_sigDict;
extern Dictionary    QSPY_enumDict[8];

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
void QSEQ_config(void* seqFile, char const* seqList);
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

void QSPY_configChanged(void);

#endif // QSPY_APP

#ifdef __cplusplus
}
#endif

#endif // QSPY_H_
