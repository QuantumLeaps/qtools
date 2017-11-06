/**
* @file
* @brief QSPY host uility implementation
* @ingroup qpspy
* @cond
******************************************************************************
* Last updated for version 5.9.3
* Last updated on  2017-06-30
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2005-2017 Quantum Leaps, LLC. All rights reserved.
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
* https://state-machine.com
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

/* global objects ..........................................................*/
QSPY_LastOutput QSPY_output;

/****************************************************************************/
/* name length for internal buffers (longer names truncated) ... */
#define MAX_NAME_LEN 64

/*..........................................................................*/
typedef struct {
    KeyType key;
    char    name[MAX_NAME_LEN];
} DictEntry;

typedef struct {
    DictEntry  notFound;
    DictEntry *sto;
    int        capacity;
    int        entries;
    int        keySize;
} Dictionary;

static void Dictionary_ctor(Dictionary * const me,
                            DictEntry *sto, uint32_t capacity);
static void Dictionary_config(Dictionary * const me, int keySize);
static char const *Dictionary_at(Dictionary * const me, unsigned idx);
static void Dictionary_put(Dictionary * const me,
                           KeyType key, char const *name);
static char const *Dictionary_get(Dictionary * const me,
                                  KeyType key, char *buf);
static int Dictionary_find(Dictionary * const me, KeyType key);
static KeyType Dictionary_findKey(Dictionary * const me, char const *name);
static void Dictionary_reset(Dictionary * const me);
static void Dictionary_write(Dictionary const * const me, FILE *stream);
static bool Dictionary_read(Dictionary * const me, FILE *stream);

static char const *getMatDict(char const *s);

/*..........................................................................*/
typedef struct SigDictEntryTag {
    SigType sig;
    ObjType obj;
    char    name[MAX_NAME_LEN];
} SigDictEntry;

typedef struct SigDictionaryTag {
    SigDictEntry  notFound;
    SigDictEntry *sto;
    int           capacity;
    int           entries;
    int           ptrSize;
} SigDictionary;

static void SigDictionary_ctor(SigDictionary * const me,
                        SigDictEntry *sto, uint32_t capacity);
static void SigDictionary_config(SigDictionary * const me, int ptrSize);
static void SigDictionary_put(SigDictionary * const me,
                        SigType sig, ObjType obj, char const *name);
static char const *SigDictionary_get(SigDictionary * const me,
                        SigType sig, ObjType obj, char *buf);
static int SigDictionary_find(SigDictionary * const me,
                              SigType sig, ObjType obj);
static SigType SigDictionary_findSig(SigDictionary * const me,
                                     char const *name, ObjType obj);
static void SigDictionary_reset(SigDictionary * const me);
static void SigDictionary_write(SigDictionary const * const me, FILE *stream);
static bool SigDictionary_read(SigDictionary * const me, FILE *stream);

/*..........................................................................*/
static DictEntry     l_funSto[512];
static DictEntry     l_objSto[256];
static DictEntry     l_mscSto[64];
static DictEntry     l_usrSto[128 + 1 - QS_USER];
static SigDictEntry  l_sigSto[512];
static Dictionary    l_funDict;
static Dictionary    l_objDict;
static Dictionary    l_mscDict;
static Dictionary    l_usrDict;
static SigDictionary l_sigDict;
static char          l_dictName[32]; /* dictionary file name */

/*..........................................................................*/
static QSpyConfig    l_config;
static FILE         *l_matFile;
static FILE         *l_mscFile;
static QSPY_CustParseFun l_custParseFun;
static QSPY_resetFun     l_txResetFun;

/* QS record names... NOTE: keep in synch with qs_copy.h */
static char const *  l_qs_rec[] = {
    "QS_EMPTY",

    /* [1] QEP records */
    "QS_QEP_STATE_ENTRY",
    "QS_QEP_STATE_EXIT",
    "QS_QEP_STATE_INIT",
    "QS_QEP_INIT_TRAN",
    "QS_QEP_INTERN_TRAN",
    "QS_QEP_TRAN",
    "QS_QEP_IGNORED",
    "QS_QEP_DISPATCH",
    "QS_QEP_UNHANDLED",

    /* [10] QF records */
    "QS_QF_ACTIVE_ADD",
    "QS_QF_ACTIVE_REMOVE",
    "QS_QF_ACTIVE_SUBSCRIBE",
    "QS_QF_ACTIVE_UNSUBSCRIBE",
    "QS_QF_ACTIVE_POST_FIFO",
    "QS_QF_ACTIVE_POST_LIFO",
    "QS_QF_ACTIVE_GET",
    "QS_QF_ACTIVE_GET_LAST",
    "QS_QF_EQUEUE_INIT",
    "QS_QF_EQUEUE_POST_FIFO",
    "QS_QF_EQUEUE_POST_LIFO",
    "QS_QF_EQUEUE_GET",
    "QS_QF_EQUEUE_GET_LAST",
    "QS_QF_MPOOL_INIT",
    "QS_QF_MPOOL_GET",
    "QS_QF_MPOOL_PUT",
    "QS_QF_PUBLISH",
    "QS_QF_RESERVED8",
    "QS_QF_NEW",
    "QS_QF_GC_ATTEMPT",
    "QS_QF_GC",
    "QS_QF_TICK",
    "QS_QF_TIMEEVT_ARM",
    "QS_QF_TIMEEVT_AUTO_DISARM",
    "QS_QF_TIMEEVT_DISARM_ATTEMPT",
    "QS_QF_TIMEEVT_DISARM",
    "QS_QF_TIMEEVT_REARM",
    "QS_QF_TIMEEVT_POST",
    "QS_QF_TIMEEVT_CTR",
    "QS_QF_CRIT_ENTRY",
    "QS_QF_CRIT_EXIT",
    "QS_QF_ISR_ENTRY",
    "QS_QF_ISR_EXIT",
    "QS_QF_INT_DISABLE",
    "QS_QF_INT_ENABLE",
    "QS_QF_ACTIVE_POST_ATTEMPT",
    "QS_QF_EQUEUE_POST_ATTEMPT",
    "QS_QF_MPOOL_GET_ATTEMPT",
    "QS_QF_RESERVED1",
    "QS_QF_RESERVED0",

    /* [50] built-in scheduler records */
    "QS_SCHED_LOCK",
    "QS_SCHED_UNLOCK",
    "QS_SCHED_NEXT",
    "QS_SCHED_IDLE",
    "QS_SCHED_RESUME",

    /* [55] Additional QEP records */
    "QS_QEP_TRAN_HIST",
    "QS_QEP_TRAN_EP",
    "QS_QEP_TRAN_XP",

    /* [58] Miscellaneous QS records (not maskable) */
    "QS_TEST_PAUSED",
    "QS_TEST_PROBE_GET",
    "QS_SIG_DICT",
    "QS_OBJ_DICT",
    "QS_FUN_DICT",
    "QS_USR_DICT",
    "QS_TARGET_INFO",
    "QS_TARGET_DONE",
    "QS_RX_STATUS",
    "QS_MSC_RESERVED1",
    "QS_PEEK_DATA",
    "QS_ASSERT_FAIL"

    /* [70] Application-specific records */
};

/* QS-RX record names... NOTE: keep in synch with qs_copy.h */
static char const *  l_qs_rx_rec[] = {
    "QS_RX_INFO",
    "QS_RX_COMMAND",
    "QS_RX_RESET",
    "QS_RX_TICK",
    "QS_RX_PEEK",
    "QS_RX_POKE",
    "QS_RX_FILL",
    "QS_RX_TEST_SETUP",
    "QS_RX_TEST_TEARDOWN",
    "QS_RX_TEST_PROBE",
    "QS_RX_GLB_FILTER",
    "QS_RX_LOC_FILTER",
    "QS_RX_AO_FILTER",
    "QS_RX_CURR_OBJ",
    "QS_RX_TEST_CONTINUE",
    "QS_RX_RESERVED1",
    "QS_RX_EVENT"
};

#define FPRINF_MATFILE(format_, ...) \
    if (l_matFile != (FILE *)0) { \
        fprintf(l_matFile, format_, ##__VA_ARGS__); \
    } else (void)0

/*..........................................................................*/
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
                 QSPY_CustParseFun custParseFun)
{
    l_config.version      = version;
    l_config.objPtrSize   = objPtrSize;
    l_config.funPtrSize   = funPtrSize;
    l_config.tstampSize   = tstampSize;
    l_config.sigSize      = sigSize;
    l_config.evtSize      = evtSize;
    l_config.queueCtrSize = queueCtrSize;
    l_config.poolCtrSize  = poolCtrSize;
    l_config.poolBlkSize  = poolBlkSize;
    l_config.tevtCtrSize  = tevtCtrSize;
    l_matFile      = (FILE *)matFile;
    l_mscFile      = (FILE *)mscFile;
    l_custParseFun = custParseFun;

    if (l_mscFile != (FILE *)0) {
        /* reserve space at the beginning of the MscGen file for the header,
        * which is known only at the end of the run in qsStop().
        */
        fprintf(l_mscFile,
            "                                        \n"
            "                                        \n"
            "                                        \n"
            "                                        \n"
            "                                        \n"
            "                                        \n"
            "                                        \n"
            "                                        \n");
    }

    Dictionary_ctor(&l_funDict, l_funSto,
                    sizeof(l_funSto)/sizeof(l_funSto[0]));
    Dictionary_ctor(&l_objDict, l_objSto,
                    sizeof(l_objSto)/sizeof(l_objSto[0]));
    Dictionary_ctor(&l_mscDict, l_mscSto,
                    sizeof(l_mscSto)/sizeof(l_mscSto[0]));
    Dictionary_ctor(&l_usrDict, l_usrSto,
                    sizeof(l_usrSto)/sizeof(l_usrSto[0]));
    SigDictionary_ctor(&l_sigDict, l_sigSto,
                       sizeof(l_sigSto)/sizeof(l_sigSto[0]));
    Dictionary_config(&l_funDict, l_config.funPtrSize);
    Dictionary_config(&l_objDict, l_config.objPtrSize);
    Dictionary_config(&l_usrDict, 1);
    SigDictionary_config(&l_sigDict, l_config.objPtrSize);

    SNPRINTF_LINE("-v%d", (unsigned)version);       QSPY_onPrintLn();
    SNPRINTF_LINE("-T%d", (unsigned)tstampSize);    QSPY_onPrintLn();
    SNPRINTF_LINE("-O%d", (unsigned)objPtrSize);    QSPY_onPrintLn();
    SNPRINTF_LINE("-F%d", (unsigned)l_config.funPtrSize); QSPY_onPrintLn();
    SNPRINTF_LINE("-S%d", (unsigned)sigSize);       QSPY_onPrintLn();
    SNPRINTF_LINE("-E%d", (unsigned)evtSize);       QSPY_onPrintLn();
    SNPRINTF_LINE("-Q%d", (unsigned)queueCtrSize);  QSPY_onPrintLn();
    SNPRINTF_LINE("-P%d", (unsigned)poolCtrSize);   QSPY_onPrintLn();
    SNPRINTF_LINE("-B%d", (unsigned)poolBlkSize);   QSPY_onPrintLn();
    SNPRINTF_LINE("-C%d", (unsigned)tevtCtrSize);   QSPY_onPrintLn();
    QSPY_line[0] = '\0'; QSPY_onPrintLn();
}
/*..........................................................................*/
void QSPY_configTxReset(QSPY_resetFun txResetFun) {
    l_txResetFun = txResetFun;
}
/*..........................................................................*/
void QSPY_configMatFile(void *matFile) {
    if (l_matFile != (FILE *)0) {
        fclose(l_matFile);
    }
    l_matFile = (FILE *)matFile;
}
/*..........................................................................*/
void QSPY_configMscFile(void *mscFile) {
    if (l_mscFile != (FILE *)0) {
        int i;
        fprintf(l_mscFile, "}\n");
        rewind(l_mscFile);
        fprintf(l_mscFile, "msc {\n");
        for (i = 0; ; ++i) {
            char const *entry = Dictionary_at(&l_mscDict, i);
            if (entry[0] != '\0') {
                if (i == 0) {
                    fprintf(l_mscFile, "\"%s\"", entry);
                }
                else {
                    fprintf(l_mscFile, ",\"%s\"", entry);
                }
            }
            else {
                break;
            }
        }
        fprintf(l_mscFile, ";\n");
        fclose(l_mscFile);
    }
    l_mscFile = (FILE *)mscFile;
}
/*..........................................................................*/
QSpyConfig const *QSPY_getConfig(void) {
    return &l_config;
}
/*..........................................................................*/
void QSpyRecord_init(QSpyRecord * const me,
                     uint8_t const *start, uint32_t tot_len)
{
    me->start   = start;
    me->tot_len = tot_len;
    me->pos     = start + 2;
    me->len     = tot_len - 3U;
    me->rec     = start[1];
}
/*..........................................................................*/
QSpyStatus QSpyRecord_OK(QSpyRecord * const me) {
    if (me->len != 0) {
        SNPRINTF_LINE("   <COMMS> ERROR    ");
        if (me->len > 0) {
            SNPRINTF_APPEND("%d bytes unused in ", me->len);
        }
        else {
            SNPRINTF_APPEND("%d bytes needed in ", (-me->len));
        }

        /* is this a standard QS record? */
        if (me->rec < sizeof(l_qs_rec)/sizeof(l_qs_rec[0])) {
            SNPRINTF_APPEND("Rec=%s", l_qs_rec[me->rec]);
        }
        else { /* USER-specific record */
            SNPRINTF_APPEND("Rec=USER+%3d", (int)(me->rec - QS_USER));
        }
        QSPY_onPrintLn();
        return QSPY_ERROR;
    }
    return QSPY_SUCCESS;
}
/*..........................................................................*/
uint32_t QSpyRecord_getUint32(QSpyRecord * const me, uint8_t size) {
    uint32_t ret = (uint32_t)0;

    if (me->len >= size) {
        if (size == (uint8_t)1) {
            ret = (uint32_t)me->pos[0];
        }
        else if (size == (uint8_t)2) {
            ret = (((uint32_t)me->pos[1] << 8) | (uint32_t)me->pos[0]);
        }
        else if (size == (uint8_t)4) {
            ret = ((((((uint32_t)me->pos[3] << 8)
                        | (uint32_t)me->pos[2]) << 8)
                          | (uint32_t)me->pos[1]) << 8)
                            | (uint32_t)me->pos[0];
        }
        else {
            Q_ASSERT(0);
        }
        me->pos += size;
        me->len -= size;
    }
    else {
        SNPRINTF_LINE("   <COMMS> ERROR    %d more bytes needed for uint%d_t ",
                     (int)(size - me->len), (int)(size*8U));
        me->len = -1;
        QSPY_onPrintLn();
    }
    return ret;
}
/*..........................................................................*/
int32_t QSpyRecord_getInt32(QSpyRecord * const me, uint8_t size) {
    int32_t ret = (int32_t)0;

    if (me->len >= size) {
        if (size == (uint8_t)1) {
            ret = (uint32_t)me->pos[0];
            ret <<= 24;
            ret >>= 24; /* sign-extend */
        }
        else if (size == (uint8_t)2) {
            ret = ((uint32_t)me->pos[1] << 8)
                        | (uint32_t)me->pos[0];
            ret <<= 16;
            ret >>= 16; /* sign-extend */
        }
        else if (size == (uint8_t)4) {
            ret = ((((((int32_t)me->pos[3] << 8)
                        | (uint32_t)me->pos[2]) << 8)
                          | (uint32_t)me->pos[1]) << 8)
                            | (uint32_t)me->pos[0];
        }
        else {
            Q_ASSERT(0);
        }
        me->pos += size;
        me->len -= size;
    }
    else {
        SNPRINTF_LINE("   <COMMS> ERROR    %d more bytes needed for int%d_t ",
                     (int)(size - me->len), (int)(size*8U));
        me->len = -1;
        QSPY_onPrintLn();
    }
    return ret;
}
/*..........................................................................*/
uint64_t QSpyRecord_getUint64(QSpyRecord * const me, uint8_t size) {
    uint64_t ret = (uint64_t)0;

    if (me->len >= size) {
        if (size == (uint8_t)1) {
            ret = (uint64_t)me->pos[0];
        }
        else if (size == (uint8_t)2) {
            ret = (((uint64_t)me->pos[1] << 8)
                       | (uint32_t)me->pos[0]);
        }
        else if (size == (uint8_t)4) {
            ret = ((((((uint64_t)me->pos[3] << 8)
                        | (uint64_t)me->pos[2]) << 8)
                          | (uint64_t)me->pos[1]) << 8)
                            | (uint64_t)me->pos[0];
        }
        else if (size == (uint8_t)8) {
            ret = ((((((((((((((uint64_t)me->pos[7] << 8)
                        | (uint64_t)me->pos[6]) << 8)
                          | (uint64_t)me->pos[5]) << 8)
                            | (uint64_t)me->pos[4]) << 8)
                              | (uint64_t)me->pos[3]) << 8)
                                | (uint64_t)me->pos[2]) << 8)
                                  | (uint64_t)me->pos[1]) << 8)
                                    | (uint64_t)me->pos[0];
        }
        else {
            Q_ASSERT(0);
        }
        me->pos += size;
        me->len -= size;
    }
    else {
        SNPRINTF_LINE("   <COMMS> ERROR    %d more bytes needed for uint%d_t ",
                     (int)(size - me->len), (int)(size*8U));
        me->len = -1;
        QSPY_onPrintLn();
    }
    return ret;
}
/*..........................................................................*/
int64_t QSpyRecord_getInt64(QSpyRecord * const me, uint8_t size) {
    int64_t ret = (int64_t)0;

    if (me->len >= size) {
        if (size == (uint8_t)1) {
            ret = (uint64_t)me->pos[0];
            ret <<= 56;
            ret >>= 56; /* sign-extend */
        }
        else if (size == (uint8_t)2) {
            ret = (((uint64_t)me->pos[1] << 8)
                       | (uint32_t)me->pos[0]);
            ret <<= 48;
            ret >>= 48; /* sign-extend */
        }
        else if (size == (uint8_t)4) {
            ret = ((((((uint64_t)me->pos[3] << 8)
                        | (uint64_t)me->pos[2]) << 8)
                          | (uint64_t)me->pos[1]) << 8)
                            | (uint64_t)me->pos[0];
            ret <<= 32;
            ret >>= 32; /* sign-extend */
        }
        else if (size == (uint8_t)8) {
            ret = ((((((((((((((uint64_t)me->pos[7] << 8)
                        | (uint64_t)me->pos[6]) << 8)
                          | (uint64_t)me->pos[5]) << 8)
                            | (uint64_t)me->pos[4]) << 8)
                              | (uint64_t)me->pos[3]) << 8)
                                | (uint64_t)me->pos[2]) << 8)
                                  | (uint64_t)me->pos[1]) << 8)
                                    | (uint64_t)me->pos[0];
        }
        else {
            Q_ASSERT(0);
        }
        me->pos += size;
        me->len -= size;
    }
    else {
        SNPRINTF_LINE("   <COMMS> ERROR    %d more bytes needed for int%d_t ",
                     (int)(size - me->len), (int)(size*8U));
        me->len = -1;
        QSPY_onPrintLn();
    }
    return ret;
}
/*..........................................................................*/
char const *QSpyRecord_getStr(QSpyRecord * const me) {
    uint8_t const *p;
    int32_t l;

    /* the following loop finds the beginning of the string
    * and removes or replaces special characters on-the-fly.
    * Specifically a leading '&' is removed and all brackets '[]'
    * are replaced with '<>'. This is to avoid any potential
    * conflicts in QUTEST with matching strings (e.g., with
    * string match command or regular explerssions.
    */
    for (l = me->len, p = me->pos; l > 0; --l, ++p) {
        if (*p == (uint8_t)0) {
            char const *s = (char const *)me->pos;
            me->len = l - 1;
            me->pos = p + 1;

            if (*s == '&') {
                ++s;  /* remove the leading '&' */
            }
            return s;
        }
        /* replace '[' with '<' */
        else if ((*p == '[') && ((p == me->pos) || (*(p - 1) != 27))) {
            *((uint8_t *)p) = '<'; /* cast 'const' away */
        }
        /* replace '[' with '<' */
        else if ((*p == ']') && ((p == me->pos) || (*(p - 1) != 27))) {
            *((uint8_t *)p) = '>'; /* cast 'const' away */
        }
    }

    /* error case... */
    SNPRINTF_LINE("   <COMMS> ERROR    %d more bytes needed for string",
                 (int)me->len);
    me->len = -1;
    QSPY_onPrintLn();
    return "";
}
/*..........................................................................*/
uint8_t const *QSpyRecord_getMem(QSpyRecord * const me,
                                 uint8_t size,
                                 uint32_t *pNum)
{
    if ((me->len >= 1) && ((*me->pos) <= me->len)) {
        uint8_t num = *me->pos;
        uint8_t const *mem = me->pos + 1;
        *pNum = num;
        me->len -= 1 + (num * size);
        me->pos += 1 + (num * size);
        return mem;
    }

    /* error case... */
    SNPRINTF_LINE("   <COMMS> ERROR    %d more bytes needed for memory-dump",
                 (int)me->len);
    me->len = -1;
    *pNum = (uint32_t)0;
    QSPY_onPrintLn();

    return (uint8_t *)0;
}
/*..........................................................................*/
static void QSpyRecord_processUser(QSpyRecord * const me) {
    uint8_t  fmt;
    int64_t  i64;
    uint64_t u64;
    int32_t  i32;
    uint32_t u32;
    static char const *ifmt[] = {
        "%li",   "%1li",  "%2li",  "%3li",
        "%4li",  "%5li",  "%6li",  "%7li",
        "%8li",  "%9li",  "%10li", "%11li",
        "%12li", "%13li", "%14li", "%15li"
    };
    static char const *ufmt[] = {
        "%lu",   "%1lu",  "%2lu",  "%3lu",
        "%4lu",  "%5lu",  "%6lu",  "%7lu",
        "%8lu",  "%9lu",  "%10lu", "%11lu",
        "%12lu", "%13lu", "%14lu", "%15lu"
    };
    static char const *uhfmt[] = {
        "0x%0lX",   "0x%01lX",  "0x%02lX",  "0x%03lX",
        "0x%04lX",  "0x%05lX",  "0x%06lX",  "0x%07lX",
        "0x%08lX",  "0x%09lX",  "0x%010lX", "0x%011lX",
        "0x%012lX", "0x%013lX", "0x%014lX", "0x%015lX"
    };
    static char const *ilfmt[] = {
        "%2"PRIi64,  "%4"PRIi64,  "%6"PRIi64,  "%8"PRIi64,
        "%10"PRIi64, "%12"PRIi64, "%14"PRIi64, "%16"PRIi64,
        "%18"PRIi64, "%20"PRIi64, "%22"PRIi64, "%24"PRIi64,
        "%26"PRIi64, "%28"PRIi64, "%30"PRIi64, "%32"PRIi64
    };
    static char const *ulfmt[] = {
        "%2"PRIu64,  "%4"PRIu64,  "%6"PRIu64,  "%8"PRIu64,
        "%10"PRIu64, "%12"PRIu64, "%14"PRIu64, "%16"PRIu64,
        "%18"PRIu64, "%20"PRIu64, "%22"PRIu64, "%24"PRIu64,
        "%26"PRIu64, "%28"PRIu64, "%30"PRIu64, "%32"PRIu64
    };
    static char const *efmt[] = {
        "%7.0e",   "%9.1e",   "%10.2e",  "%11.3e",
        "%12.4e",  "%13.5e",  "%14.6e",  "%15.7e",
        "%16.8e",  "%17.9e",  "%18.10e", "%19.11e",
        "%20.12e", "%21.13e", "%22.14e", "%23.15e",
    };

    u32 = QSpyRecord_getUint32(me, l_config.tstampSize);
    i32 = Dictionary_find(&l_usrDict, me->rec);
    if (i32 >= 0) {
        SNPRINTF_LINE("%010u %s", u32, Dictionary_at(&l_usrDict, i32));
    }
    else {
        SNPRINTF_LINE("%010u USER+%03d", u32, (int)(me->rec - QS_USER));
    }

    FPRINF_MATFILE("%3d %10u", (int)me->rec, u32);

    while (me->len > 0) {
        char const *s;
        fmt = (uint8_t)QSpyRecord_getUint32(me, 1);  /* get the format byte */

        SNPRINTF_APPEND(" ");
        FPRINF_MATFILE(" ");

        switch (fmt & 0x0F) {
            case QS_I8_T: {
                i32 = QSpyRecord_getInt32(me, 1);
                SNPRINTF_APPEND(ifmt[fmt >> 4], (int)i32);
                FPRINF_MATFILE(ifmt[fmt >> 4], (int)i32);
                break;
            }
            case QS_U8_T: {
                u32 = QSpyRecord_getUint32(me, 1);
                SNPRINTF_APPEND(ufmt[fmt >> 4], (unsigned)u32);
                FPRINF_MATFILE(ufmt[fmt >> 4], (unsigned)u32);
                break;
            }
            case QS_I16_T: {
                i32 = QSpyRecord_getInt32(me, 2);
                SNPRINTF_APPEND(ifmt[fmt >> 4], (int)i32);
                FPRINF_MATFILE(ifmt[fmt >> 4], (int)i32);
                break;
            }
            case QS_U16_T: {
                u32 = QSpyRecord_getUint32(me, 2);
                SNPRINTF_APPEND(ufmt[fmt >> 4], (unsigned)u32);
                FPRINF_MATFILE(ufmt[fmt >> 4], (unsigned)u32);
                break;
            }
            case QS_I32_T: {
                i32 = QSpyRecord_getInt32(me, 4);
                SNPRINTF_APPEND(ifmt[fmt >> 4], (int)i32);
                FPRINF_MATFILE(ifmt[fmt >> 4], (int)i32);
                break;
            }
            case QS_U32_T: {
                u32 = QSpyRecord_getUint32(me, 4);
                SNPRINTF_APPEND(ufmt[fmt >> 4], (unsigned)u32);
                FPRINF_MATFILE(ufmt[fmt >> 4], (unsigned)u32);
                break;
            }
            case QS_U32_HEX_T: {
                u32 = QSpyRecord_getUint32(me, 4);
                SNPRINTF_APPEND(uhfmt[fmt >> 4], (unsigned)u32);
                FPRINF_MATFILE(uhfmt[fmt >> 4], (unsigned)u32);
                break;
            }
            case QS_F32_T: {
                union {
                   uint32_t u;
                   float    f;
                } x;
                x.u = QSpyRecord_getUint32(me, 4);
                SNPRINTF_APPEND(efmt[fmt >> 4], (double)x.f);
                FPRINF_MATFILE(efmt[fmt >> 4], (double)x.f);
                break;
            }
            case QS_F64_T: {
                union F64Rep {
                    uint64_t u;
                    double   d;
                } data;
                data.u = QSpyRecord_getUint64(me, 8);
                SNPRINTF_APPEND(efmt[fmt >> 4], data.d);
                FPRINF_MATFILE(efmt[fmt >> 4], data.d);
                break;
            }
            case QS_STR_T: {
                s = QSpyRecord_getStr(me);
                SNPRINTF_APPEND(s);
                FPRINF_MATFILE(s);
                break;
            }
            case QS_MEM_T: {
                uint8_t const *mem = QSpyRecord_getMem(me, 1, &u32);
                for (; u32 > (uint32_t)0; --u32, ++mem) {
                    SNPRINTF_APPEND(" %02X", (int)*mem);
                    FPRINF_MATFILE(" %03d", (int)*mem);
                }
                break;
            }
            case QS_SIG_T: {
                u32 = QSpyRecord_getUint32(me, l_config.sigSize);
                u64 = QSpyRecord_getUint64(me, l_config.objPtrSize);
                SNPRINTF_APPEND(" %s,Obj=%s",
                    SigDictionary_get(&l_sigDict, u32, u64, (char *)0),
                    Dictionary_get(&l_objDict, u64, (char *)0));
                    FPRINF_MATFILE("%04u %20"PRId64, u32, u64);
                break;
            }
            case QS_OBJ_T: {
                u64 = QSpyRecord_getUint64(me, l_config.objPtrSize);
                SNPRINTF_APPEND(" %s",
                    Dictionary_get(&l_objDict, u64, (char *)0));
                FPRINF_MATFILE(" %20"PRId64, u64);
                break;
            }
            case QS_FUN_T: {
                u64 = QSpyRecord_getUint64(me, l_config.funPtrSize);
                SNPRINTF_APPEND(" %s",
                    Dictionary_get(&l_funDict, u64, (char *)0));
                FPRINF_MATFILE(" %20"PRId64, u64);
                break;
            }
            case QS_I64_T: {
                i64 = QSpyRecord_getInt64(me, 8);
                SNPRINTF_APPEND(ilfmt[fmt >> 4], i64);
                FPRINF_MATFILE(ilfmt[fmt >> 4], i64);
                break;
            }
            case QS_U64_T: {
                u64 = QSpyRecord_getUint64(me, 8);
                SNPRINTF_APPEND(ulfmt[fmt >> 4], u64);
                FPRINF_MATFILE(ulfmt[fmt >> 4], u64);
                break;
            }
            default: {
                SNPRINTF_APPEND("Unknown format");
                me->len = -1;
                break;
            }
        }
    }
    QSPY_onPrintLn();
    FPRINF_MATFILE("\n");
}
/*..........................................................................*/
static void QSpyRecord_process(QSpyRecord * const me) {
    uint32_t t, a, b, c, d, e;
    uint64_t p, q, r;
    char buf[256];
    char const *s = 0;
    char const *w = 0;

    /* set the current QS record-ID for any output in this function */
    QSPY_output.rec = me->rec;

    switch (me->rec) {
        /* Session start ...................................................*/
        case QS_EMPTY: {
            if (l_config.version < 550U) {
                if (QSpyRecord_OK(me)) {
                    SNPRINTF_LINE("########## Trg-RST  %u",
                                 (unsigned)l_config.version);
                    QSPY_onPrintLn();

                    Dictionary_reset(&l_funDict);
                    Dictionary_reset(&l_objDict);
                    Dictionary_reset(&l_mscDict);
                    Dictionary_reset(&l_usrDict);
                    SigDictionary_reset(&l_sigDict);
                }
            }
            else {
                /* silently ignore */
            }
            break;
        }

        /* QEP records .....................................................*/
        case QS_QEP_STATE_ENTRY:
            s = "St-Entry";
        case QS_QEP_STATE_EXIT: {
            if (s == 0) s = "St-Exit ";
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("===RTC===> %s Obj=%s,State=%s",
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %20"PRId64" %20"PRId64"\n",
                            (int)me->rec, p, q);
            }
            break;
        }
        case QS_QEP_STATE_INIT:
            s = "St-Init ";
        case QS_QEP_TRAN_HIST:
            if (s == 0) s = "St-Hist ";
        case QS_QEP_TRAN_EP:
            if (s == 0) s = "St-EP   ";
        case QS_QEP_TRAN_XP: {
            if (s == 0) s = "St-XP   ";
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            r = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("===RTC===> %s Obj=%s,State=%s->%s",
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0),
                       Dictionary_get(&l_funDict, r, buf));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %20"PRId64" %20"PRId64" %20"PRId64"\n",
                               (int)me->rec, p, q, r);
            }
            break;
        }
        case QS_QEP_INIT_TRAN: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Init===> Obj=%s,State=%s",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %20"PRId64"\n",
                               (int)me->rec, t, p, q);
            }
            break;
        }
        case QS_QEP_INTERN_TRAN: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u =>Intern Obj=%s,Sig=%s,State=%s",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %20"PRId64
                               " %20"PRId64"\n",
                               (int)me->rec, t, a, p, q);
            }
            break;
        }
        case QS_QEP_TRAN: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            r = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u ===>Tran "
                       "Obj=%s,Sig=%s,State=%s->%s",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0),
                       Dictionary_get(&l_funDict, r, buf));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %20"PRId64" %20"PRId64
                               " %20"PRId64"\n",
                               (int)me->rec, t, a, p, q, r);
                if (l_mscFile != (FILE *)0) {
                    if (Dictionary_find(&l_mscDict, p) >= 0) { /* found? */
                        fprintf(l_mscFile,
                                "\"%s\" rbox \"%s\" [label=\"%s\"];\n",
                                Dictionary_get(&l_mscDict, p, (char *)0),
                                Dictionary_get(&l_mscDict, p, buf),
                                Dictionary_get(&l_funDict, r, (char *)0));
                    }
                }
            }
            break;
        }
        case QS_QEP_IGNORED: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u =>Ignore Obj=%s,Sig=%s,State=%s",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %20"PRId64" %20"PRId64"\n",
                               (int)me->rec, t, a, p, q);
            }
            break;
        }
        case QS_QEP_DISPATCH: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Disp===> Obj=%s,Sig=%s,State=%s",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %20"PRId64" %20"PRId64"\n",
                               (int)me->rec, t, a, p, q);
            }
            break;
        }
        case QS_QEP_UNHANDLED: {
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("===RTC===> St-Unhnd Obj=%s,Sig=%s,State=%s",
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %4u %20"PRId64" %20"PRId64"\n",
                               (int)me->rec, a, p, q);
            }
            break;
        }

        /* QF records ......................................................*/
        case QS_QF_ACTIVE_ADD:
            s = "Add  ";
        case QS_QF_ACTIVE_REMOVE: {
            if (s == 0) s = "Remov";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            a = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u AO-%s Obj=%s,Pri=%u",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %2u\n",
                               (int)me->rec, t, p, a);
            }
            break;
        }
        case QS_QF_ACTIVE_SUBSCRIBE:
            s = "Subsc";
        case QS_QF_ACTIVE_UNSUBSCRIBE: {
            if (s == 0) s = "Unsub";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u AO-%s Obj=%s,Sig=%s",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %20"PRId64"\n",
                               (int)me->rec, t, a, p);
            }
            break;
        }
        case QS_QF_ACTIVE_POST_FIFO:
            s = "Post ";
        case QS_QF_ACTIVE_POST_ATTEMPT: {
            if (s == 0) s = "PostA";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            if (l_config.version < 420U) {
                q = (uint64_t)0;
            }
            else {
                q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            }
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            e = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u AO-%s Sdr=%s,Obj=%s,"
                       "Evt<Sig=%s,Pool=%u,Ref=%u>,"
                       "Que<Free=%u,%s=%u>",
                       t,
                       s,
                       Dictionary_get(&l_objDict, q, (char *)0),
                       Dictionary_get(&l_objDict, p, buf),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       b, c, d,
                       (me->rec == QS_QF_ACTIVE_POST_FIFO ? "Min" : "Mar"),
                       e);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %4u %20"PRId64
                               " %3u %3u %4u %4u\n",
                               (int)me->rec, t, q, a, p, b, c, d, e);
                if (l_mscFile != (FILE *)0) {
                    if (Dictionary_find(&l_mscDict, q) < 0) { /* not found? */
                        Dictionary_put(&l_mscDict, q,
                            Dictionary_get(&l_objDict, q, (char *)0));
                    }
                    if (Dictionary_find(&l_mscDict, p) < 0) { /* not found? */
                        Dictionary_put(&l_mscDict, p,
                            Dictionary_get(&l_objDict, p, (char *)0));
                    }
                    fprintf(l_mscFile,
                            "\"%s\"->\"%s\" [label=\"%u:%s\"];\n",
                            Dictionary_get(&l_mscDict, q, (char *)0),
                            Dictionary_get(&l_mscDict, p, buf),
                            t,
                            SigDictionary_get(&l_sigDict, a, p, (char *)0));
                }
            }
            break;
        }
        case QS_QF_ACTIVE_POST_LIFO: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            e = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u AO-LIFO  Obj=%s,"
                       "Evt<Sig=%s,Pool=%u,Ref=%u>,"
                       "Que<Free=%u,Min=%u>",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       b, c, d, e);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %20"PRId64" %3u %3u %4u %4u\n",
                               (int)me->rec, t, a, p, b, c, d, e);
                if (l_mscFile != (FILE *)0) {
                    if (Dictionary_find(&l_mscDict, p) < 0) { /* not found? */
                        Dictionary_put(&l_mscDict, p,
                            Dictionary_get(&l_objDict, p, (char *)0));
                    }
                    fprintf(l_mscFile,
                            "\"%s\"->\"%s\" [label=\"%u:%s-LIFO\"];\n",
                            Dictionary_get(&l_mscDict, p, (char *)0),
                            Dictionary_get(&l_mscDict, p, buf),
                            t,
                            SigDictionary_get(&l_sigDict, a, p, (char *)0));
                }
            }
            break;
        }
        case QS_QF_ACTIVE_GET:
            s = "AO-Get  ";
        case QS_QF_EQUEUE_GET: {
            if (s == 0) s = "EQ-Get  ";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Obj=%s,Evt<Sig=%s,Pool=%u,Ref=%u>,"
                       "Que<Free=%u>",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       b, c, d);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %20"PRId64" %3u %3u %4u\n",
                               (int)me->rec, t, a, p, b, c, d);
            }
            break;
        }
        case QS_QF_ACTIVE_GET_LAST:
            s = "AO-GetL ";
        case QS_QF_EQUEUE_GET_LAST: {
            if (s == 0) s = "EQ-GetL ";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Obj=%s,Evt<Sig=%s,Pool=%u,Ref=%u>",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       b, c);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %20"PRId64" %3u %3u\n",
                               (int)me->rec, t, a, p, b, c);
            }
            break;
        }
        case QS_QF_EQUEUE_INIT: {
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            b = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("           EQ-Init  Obj=%s,Len=%u",
                       Dictionary_get(&l_objDict, p, (char *)0),
                       b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %20"PRId64" %4u\n",
                               (int)me->rec, p, b);
            }
            break;
        }
        case QS_QF_EQUEUE_POST_FIFO:
            s = "Post ";
            w = "Min";
        case QS_QF_EQUEUE_POST_ATTEMPT:
            if (s == 0) s = "PostA";
            if (w == 0) w = "Mar";
        case QS_QF_EQUEUE_POST_LIFO: {
            if (s == 0) s = "LIFO";
            if (w == 0) w = "Min";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            e = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u EQ-%s Obj=%s,"
                       "Evt<Sig=%s,Pool=%u,Ref=%u>,"
                       "Que<Free=%u,%s=%u>",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       b, c, d,
                       w,
                       e);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %20"PRId64" %3u %3u %4u %4u\n",
                               (int)me->rec, t, a, p,
                               b, c, d, e);
            }
            break;
        }
        case QS_QF_MPOOL_INIT: {
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            b = QSpyRecord_getUint32(me, l_config.poolCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("           MP-Init  Obj=%s,Blcks=%u",
                       Dictionary_get(&l_objDict, p, (char *)0),
                       b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %20"PRId64" %4u\n",
                               (int)me->rec, p, b);
            }
            break;
        }
        case QS_QF_MPOOL_GET:
            s = "Get  ";
            w = "Min";
        case QS_QF_MPOOL_GET_ATTEMPT: {
            if (s == 0) s = "GetA ";
            if (w == 0) w = "Mar";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            b = QSpyRecord_getUint32(me, l_config.poolCtrSize);
            c = QSpyRecord_getUint32(me, l_config.poolCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u MP-%s Obj=%s,Free=%u,%s=%u",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       b,
                       w,
                       c);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %4u %4u\n",
                               (int)me->rec, t, p, b, c);
            }
            break;
        }
        case QS_QF_MPOOL_PUT: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            b = QSpyRecord_getUint32(me, l_config.poolCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u MP-Put   Obj=%s,Free=%u",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %4u\n",
                               (int)me->rec, t, p, b);
            }
            break;
        }

        /* QF */
        case QS_QF_PUBLISH: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            if (l_config.version < 420U) {
                p = (uint64_t)0;
                a = QSpyRecord_getUint32(me, l_config.sigSize);
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                p = QSpyRecord_getUint64(me, l_config.objPtrSize);
                a = QSpyRecord_getUint32(me, l_config.sigSize);
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u QF-Pub   Sdr=%s"
                       "Evt<Sig=%s,Pool=%u,Ref=%u>",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, 0, (char *)0),
                       b, c);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %4u %10u\n",
                               (int)me->rec, t, p, a, b);
                if (l_mscFile != (FILE *)0) {
                    if (Dictionary_find(&l_mscDict, p) < 0) { /* not found? */
                        Dictionary_put(&l_mscDict, p,
                            Dictionary_get(&l_objDict, p, (char *)0));
                    }
                    fprintf(l_mscFile,
                            "\"%s\"->* [label=\"%u:%s\""
                            ",textcolour=\"#0000ff\""
                            ",linecolour=\"#0000ff\"];\n",
                            Dictionary_get(&l_mscDict, p, (char *)0),
                            t,
                            SigDictionary_get(&l_sigDict, a, p, (char *)0));
                }
            }
            break;
        }
        case QS_QF_NEW: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.evtSize);
            c = QSpyRecord_getUint32(me, l_config.sigSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u QF-New   Sig=%s,Size=%u",
                       t,
                       SigDictionary_get(&l_sigDict, c, 0, (char *)0),
                       a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %4u\n",
                               (int)me->rec, t, a, c);
            }
            break;
        }
        case QS_QF_GC_ATTEMPT:
            s = "QF-gcA  ";
        case QS_QF_GC: {
            if (s == 0) s = "QF-gc   ";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Evt<Sig=%s,Pool=%d,Ref=%d>",
                       t,
                       s,
                       SigDictionary_get(&l_sigDict, a, 0, (char *)0),
                       b, c);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %3u %3u\n",
                               (int)me->rec, t, a, b, c);
            }
            break;
        }
        case QS_QF_TICK: {
            a = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("           Tick<%1u>  Ctr=%010u",
                        b,
                        a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u\n", (int)me->rec, a);
                if (l_mscFile != (FILE *)0) {
                    fprintf(l_mscFile,
                            "--- [label=\"tick %u\""
                            ",textcolour=\"#ff0000\""
                            ",linecolour=\"#ff0000\"];\n",
                            a);
                }
            }
            break;
        }
        case QS_QF_TIMEEVT_ARM:
            s = "Arm ";
        case QS_QF_TIMEEVT_DISARM: {
            if (s == 0) s = "Dis ";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            c = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            d = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TE%1u-%s Obj=%s,AO=%s,Tim=%u,Int=%u",
                       t,
                       b,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_objDict, q, buf),
                       c, d);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %20"PRId64
                               " %10u %10u\n",
                               (int)me->rec, t, p, q, c, d);
            }
            break;
        }
        case QS_QF_TIMEEVT_AUTO_DISARM: {
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("           TE%1u-ADis Obj=%s,AO=%s",
                       b,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_objDict, q, buf));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %20"PRId64" %20"PRId64"\n",
                               (int)me->rec, p, q);
           }
            break;
        }
        case QS_QF_TIMEEVT_DISARM_ATTEMPT: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TE%1u-DisA Obj=%s,AO=%s",
                       t,
                       b,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_objDict, q, buf));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %20"PRId64"\n",
                               (int)me->rec, t, p, q);
            }
            break;
        }
        case QS_QF_TIMEEVT_REARM: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            c = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            d = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            e = QSpyRecord_getUint32(me, 1);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TE%1u-Rarm Obj=%s,AO=%s,"
                       "Tim=%u,Int=%u,Was=%1u",
                       t,
                       b,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_objDict, q, buf),
                       c, d, e);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %20"PRId64
                               " %10u %10u %1u\n",
                               (int)me->rec, t, p, q, c, d, e);
            }
            break;
        }
        case QS_QF_TIMEEVT_POST: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TE%1u-Post Obj=%s,Sig=%s,AO=%s",
                       t,
                       b,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, q, (char *)0),
                       Dictionary_get(&l_objDict, q, buf));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %4u %20"PRId64"\n",
                               (int)me->rec, t, p, a, q);
            }
            break;
        }
        case QS_QF_TIMEEVT_CTR: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            c = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            d = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TE%1u-Ctr  Obj=%s,AO=%s,"
                       "Tim=%u,Int=%u",
                       t,
                       b,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_objDict, q, buf),
                       c, d);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %20"PRId64" %20"PRId64" %10u %10u\n",
                               (int)me->rec, t, p, q, c, d);
            }
            break;
        }
        case QS_QF_CRIT_ENTRY:
            s = "QF-CritE";
        case QS_QF_CRIT_EXIT: {
            if (s == 0) s = "QF-CritX";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Nest=%d",
                       t,
                       s,
                       a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %3u\n",
                               (int)me->rec, t, a);
           }
            break;
        }
        case QS_QF_ISR_ENTRY:
            s = "QF-IsrE";
        case QS_QF_ISR_EXIT: {
            if (s == 0) s = "QF-IsrX";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s  Nest=%u,Pri=%u",
                       t,
                       s,
                       a, b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %3u %3u\n",
                               (int)me->rec, t, a, b);
            }
            break;
        }

        /* built-in scheduler records ......................................*/
        case QS_SCHED_LOCK:
            if (s == 0) s = "Sch-Lock";
        case QS_SCHED_UNLOCK: {
            if (s == 0) s = "Sch-Unlk";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Ceil=%u->%u",
                       t,
                       s,
                       a, b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %3u %3u\n",
                               (int)me->rec, t, a, b);
            }
            break;
        }
        case QS_SCHED_NEXT: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Sch-Next Pri=%u->%u",
                       t, b, a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %3u %3u\n",
                               (int)me->rec, t, a, b);
            }
            break;
        }
        case QS_SCHED_IDLE: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Sch-Idle Pri=%u->0",
                       t, a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %3u\n",
                               (int)me->rec, t, a);
            }
            break;
        }
        case QS_SCHED_RESUME: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Sch-Rsme Prio=%u->%u",
                       t, b, a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %3u %3u\n",
                               (int)me->rec, t, a, b);
            }
            break;
        }

        /* Miscallaneous QS records ........................................*/
        case QS_TEST_PAUSED: {
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("           TstPause");
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_TEST_PROBE_GET: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            a = QSpyRecord_getUint32(me, 4U);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TstProbe Fun=%s,Data=%d",
                              t, Dictionary_get(&l_funDict, q, (char *)0), a);
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_SIG_DICT: {
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                SigDictionary_put(&l_sigDict, (SigType)a, p, s);
                if (l_config.objPtrSize <= 4) {
                    SNPRINTF_LINE("           Sig-Dict %08d,"
                                  "Obj=0x%08X->%s",
                                  a, (unsigned)p, s);
                }
                else {
                    SNPRINTF_LINE("           Sig-Dict %08d,"
                                  "Obj=0x%016"PRIX64"->%s",
                                  a, p, s);
                }
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %s=[%4u %20"PRId64"];\n",
                               (int)me->rec, getMatDict(s), a, p);
            }
            break;
        }

        case QS_OBJ_DICT: {
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&l_objDict, p, s);
                if (l_config.objPtrSize <= 4) {
                    SNPRINTF_LINE("           Obj-Dict 0x%08X->%s",
                                  (unsigned)p, s);
                }
                else {
                    SNPRINTF_LINE("           Obj-Dict 0x%016"PRIX64"->%s",
                                  p, s);
                }
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %s=%20"PRId64";\n",
                               (int)me->rec, getMatDict(s), p);
            }
            break;
        }

        case QS_FUN_DICT: {
            p = QSpyRecord_getUint64(me, l_config.funPtrSize);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&l_funDict, p, s);
                if (l_config.funPtrSize <= 4) {
                    SNPRINTF_LINE("           Fun-Dict 0x%08X->%s",
                                  (unsigned)p, s);
                }
                else {
                    SNPRINTF_LINE("           Fun-Dict 0x%016"PRIX64"->%s",
                                  p, s);
                }
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %s=%20"PRId64";\n",
                            (int)me->rec, getMatDict(s), p);
            }
            break;
        }

        case QS_USR_DICT: {
            a = QSpyRecord_getUint32(me, 1);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&l_usrDict, a, s);
                SNPRINTF_LINE("           Usr-Dict %08d->%s",
                        a, s);
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_TARGET_INFO: {
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 2);

            /* extract the data bytes... */
            for (e = 0U; e < 13U; ++e) {
                buf[e] = (char)QSpyRecord_getUint32(me, 1);
            }

            if (QSpyRecord_OK(me)) {

                /* apply the target info... */
                l_config.version      = (uint16_t)(b & 0xFFFFU);
                l_config.objPtrSize   = (uint8_t)(buf[3] & 0x0FU);
                l_config.funPtrSize   = (uint8_t)((buf[3] >> 4) & 0x0FU);
                l_config.tstampSize   = (uint8_t)(buf[4] & 0x0FU);
                l_config.sigSize      = (uint8_t)(buf[0] & 0x0FU);
                l_config.evtSize      = (uint8_t)((buf[0] >> 4) & 0x0FU);
                l_config.queueCtrSize = (uint8_t)(buf[1] & 0x0FU);
                l_config.poolCtrSize  = (uint8_t)((buf[2] >> 4) & 0x0FU);
                l_config.poolBlkSize  = (uint8_t)(buf[2] & 0x0FU);
                l_config.tevtCtrSize  = (uint8_t)((buf[1] >> 4) & 0x0FU);

                for (e = 0U; e < sizeof(l_config.tstamp); ++e) {
                    l_config.tstamp[e] = (uint8_t)buf[7U + e];
                }

                /* set the dictionary file name from the time-stamp... */
                SNPRINTF_S(l_dictName, sizeof(l_dictName),
                       "qspy%02u%02u%02u_%02u%02u%02u.dic",
                       (unsigned)l_config.tstamp[5],
                       (unsigned)l_config.tstamp[4],
                       (unsigned)l_config.tstamp[3],
                       (unsigned)l_config.tstamp[2],
                       (unsigned)l_config.tstamp[1],
                       (unsigned)l_config.tstamp[0]);

                if (a != 0U) {  /* is this also Target RESET? */
                    s = "Trg-RST ";
                    Dictionary_reset(&l_funDict);
                    Dictionary_reset(&l_objDict);
                    Dictionary_reset(&l_mscDict);
                    Dictionary_reset(&l_usrDict);
                    SigDictionary_reset(&l_sigDict);

                    /* reset the QSPY-Tx channel, if available */
                    if (l_txResetFun != (QSPY_resetFun)0) {
                        (*l_txResetFun)();
                    }

                    /*TBD: close and re-open MATLAB file, MSC file, etc. */
                }
                else {
                    FILE *dictFile = (FILE *)0;
                    s = "Trg-Info";

                    /* do we have dictionary name yet? */
                    if (l_dictName[0] != '\0') {
                        FOPEN_S(dictFile, l_dictName, "r");
                        if (dictFile != (FILE *)0) {
                            if (QSPY_readDict(dictFile) != QSPY_ERROR) {
                                SNPRINTF_LINE("           Dictionaries read "
                                              "from File=%s",
                                    l_dictName);
                                QSPY_onPrintLn();
                            }
                            fclose(dictFile);
                        }
                    }
                }

                SNPRINTF_LINE("########## %s QP-Ver=%u,"
                       "Build=%02u%02u%02u_%02u%02u%02u",
                       s,
                       b,
                       (unsigned)l_config.tstamp[5],
                       (unsigned)l_config.tstamp[4],
                       (unsigned)l_config.tstamp[3],
                       (unsigned)l_config.tstamp[2],
                       (unsigned)l_config.tstamp[1],
                       (unsigned)l_config.tstamp[0]);
                QSPY_onPrintLn();

            }
            else {
                l_dictName[0] = '\0';
            }
            break;
        }

        case QS_TARGET_DONE: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1U);
            if (QSpyRecord_OK(me)) {
                if (a < sizeof(l_qs_rx_rec)/sizeof(l_qs_rx_rec[0])) {
                    SNPRINTF_LINE("%010u Trg-Done %s",
                                 t, l_qs_rx_rec[a]);
                }
                else {
                    SNPRINTF_LINE("%010u Trg-Done %d",
                                 t, a);
                }
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_RX_STATUS: {
            if (l_config.version < 580U) {
                t = QSpyRecord_getUint32(me, l_config.tstampSize);
            }
            a = QSpyRecord_getUint32(me, 1U);
            if (QSpyRecord_OK(me)) {
                if (a < 128U) { /* Ack? */
                    if (a < sizeof(l_qs_rx_rec)/sizeof(l_qs_rx_rec[0])) {
                        SNPRINTF_LINE("           Trg-Ack  %s",
                                     l_qs_rx_rec[a]);
                    }
                    else {
                        SNPRINTF_LINE("           Trg-Ack  %d", a);
                    }
                }
                else {
                    a &= 0x7FU;
                    if (a < sizeof(l_qs_rx_rec)/sizeof(l_qs_rx_rec[0])) {
                        SNPRINTF_LINE("           Trg-ERR  %s",
                                     l_qs_rx_rec[a]);
                    }
                    else {
                        SNPRINTF_LINE("           Trg-ERR  0x%02X", a);
                    }
                }
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_PEEK_DATA: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 2);  /* offset */
            b = QSpyRecord_getUint32(me, 1);  /* data size */
            w = (char const *)QSpyRecord_getMem(me, b, &c);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Trg-Peek Offs=%d,Size=%d,Num=%d,Data=<",
                              t, a, b, c);
                for (; c > (uint32_t)1; --c, w += b) {
                    switch (b) {
                        case 1:
                            SNPRINTF_APPEND("%02X,", (int)(*w & 0xFFU));
                            break;
                        case 2:
                            SNPRINTF_APPEND("%04X,",
                                (int)(*(uint16_t *)w & 0xFFFFU));
                            break;
                        case 4:
                            SNPRINTF_APPEND("%08X,", (int)(*(uint32_t *)w));
                            break;
                    }
                }
                switch (b) {
                    case 1:
                        SNPRINTF_APPEND("%02X>", (int)(*w & 0xFFU));
                        break;
                    case 2:
                        SNPRINTF_APPEND("%04X>",
                            (int)(*(uint16_t *)w & 0xFFFFU));
                        break;
                    case 4:
                        SNPRINTF_APPEND("%08X>", (int)(*(uint32_t *)w));
                        break;
                }
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_ASSERT_FAIL: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 2);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u =ASSERT= Mod=%s,Loc=%u",
                       t, s, a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%3d %10u %4u %s\n",
                            (int)me->rec, (unsigned)t, (unsigned)a, s);
            }
            break;
        }

        /* User records ....................................................*/
        default: {
            if (me->rec >= QS_USER) {
                QSpyRecord_processUser(me);
            }
            else {
                SNPRINTF_LINE("           Unknown Rec=%d,Len=%d",
                       (int)me->rec, (int)me->len);
                QSPY_onPrintLn();
            }
            break;
        }
    }
}
/*..........................................................................*/
void QSPY_stop(void) {
    QSPY_configMatFile((void *)0);
    QSPY_configMscFile((void *)0);
}
/*..........................................................................*/
void QSPY_printError(void) {
    QSPY_output.rec = 0xFF; /* send this error to the Front-End */
    fputc('\n', stdout);
    QSPY_onPrintLn();
}

/****************************************************************************/
static uint8_t l_record[QS_MAX_RECORD_SIZE];
static uint8_t *l_pos   = l_record; /* position within the record */
static uint8_t l_chksum = (uint8_t)0;
static uint8_t l_esc    = (uint8_t)0;
static uint8_t l_seq    = (uint8_t)0;

/*..........................................................................*/
void QSPY_reset(void) {
    l_pos    = l_record; /* position within the record */
    l_chksum = (uint8_t)0;
    l_esc    = (uint8_t)0;
    l_seq    = (uint8_t)0;
}
/*..........................................................................*/
void QSPY_parse(uint8_t const *buf, uint32_t nBytes) {
    static bool isJustStarted = true;

    for (; nBytes != 0U; --nBytes) {
        uint8_t b = *buf++;

        if (l_esc) { /* escaped byte arrived? */
            l_esc = (uint8_t)0;
            b ^= QS_ESC_XOR;

            l_chksum = (uint8_t)(l_chksum + b);
            if (l_pos < &l_record[sizeof(l_record)]) {
                *l_pos++ = b;
            }
            else {
                SNPRINTF_LINE("   <COMMS> ERROR    Record too long at "
                           "Seq=%u,", (unsigned)l_seq);
                /* is it a standard QS record? */
                if (l_record[1] < sizeof(l_qs_rec)/sizeof(l_qs_rec[0])) {
                    SNPRINTF_APPEND("Rec=%s", l_qs_rec[l_record[1]]);
                }
                else { /* this is a USER-specific record */
                    SNPRINTF_APPEND("Rec=USER+%3u",
                               (unsigned)(l_record[1] - QS_USER));
                }
                QSPY_printError();
                l_chksum = (uint8_t)0;
                l_pos = l_record;
                l_esc = (uint8_t)0;
            }
        }
        else if (b == QS_ESC) {   /* transparent byte? */
            l_esc = (uint8_t)1;
        }
        else if (b == QS_FRAME) { /* frame byte? */
            if (l_chksum != QS_GOOD_CHKSUM) { /* bad checksum? */
                if (!isJustStarted) {
                    isJustStarted = false;
                    SNPRINTF_LINE("   <COMMS> ERROR    Bad checksum in ");
                    if (l_record[1] < sizeof(l_qs_rec)/sizeof(l_qs_rec[0])) {
                        SNPRINTF_APPEND("Rec=%s,", l_qs_rec[l_record[1]]);
                    }
                    else {
                        SNPRINTF_APPEND("Rec=USER+%3u,",
                                   (unsigned)(l_record[1] - QS_USER));
                    }
                    SNPRINTF_APPEND("Seq=%u", (unsigned)l_seq);
                    QSPY_printError();

                    if (l_mscFile != (FILE *)0) {
                        fprintf(l_mscFile, "...;\n"
                            "--- [label=\"Bad checksum at Seq=%u,Id=%u\""
                            ",textbgcolour=\"#ffff00\""
                            ",linecolour=\"#ff0000\"];\n",
                            (unsigned)l_seq, (unsigned)l_record[1]);
                    }
                }
            }
            else if (l_pos < &l_record[3]) { /* record too short? */
                SNPRINTF_LINE("   <COMMS> ERROR    Record too short at "
                           "Seq=%u,",
                           (unsigned)l_seq);
                if (l_record[1] < sizeof(l_qs_rec)/sizeof(l_qs_rec[0])) {
                    SNPRINTF_APPEND("Rec=%s", l_qs_rec[l_record[1]]);
                }
                else {
                    SNPRINTF_APPEND("Rec=USER+%3u",
                               (unsigned)(l_record[1] - QS_USER));
                }
                QSPY_printError();
                if (l_mscFile != (FILE *)0) {
                    fprintf(l_mscFile, "...;\n"
                    "--- [label=\"Record too short at Seq=%u,Id=%u\""
                        ",textbgcolour=\"#ffff00\""
                        ",linecolour=\"#ff0000\"];\n",
                    (unsigned)l_seq, (unsigned)l_record[1]);
                }
            }
            else { /* a healty record received */
                QSpyRecord qrec;
                int parse = 1;
                ++l_seq; /* increment with natural wrap-around */

                if (!isJustStarted) {
                    isJustStarted = false;

                    /* data discountinuity found?
                    * but not for the QS_EMPTY record?
                    */
                    if ((l_seq != l_record[0])
                         && (l_record[1] != QS_EMPTY))
                    {
                        SNPRINTF_LINE("   <COMMS> ERROR    Discontinuity "
                            "Seq=%u->%u",
                            (unsigned)(l_seq - 1), (unsigned)l_record[0]);
                        QSPY_onPrintLn();
                        if (l_mscFile != (FILE *)0) {
                            fprintf(l_mscFile,
                                "--- [label=\""
                                "Data discontinuity Seq=%u->%u\""
                                ",textbgcolour=\"#ffff00\""
                                ",linecolour=\"#ff0000\"];\n"
                                "...;\n",
                               (unsigned)(l_seq - 1), (unsigned)l_record[0]);
                        }
                    }
                }
                l_seq = l_record[0];

                QSpyRecord_init(&qrec, l_record, (int32_t)(l_pos - l_record));

                if (l_custParseFun != (QSPY_CustParseFun)0) {
                    parse = (*l_custParseFun)(&qrec);
                    if (parse) {
                        /* re-initialize the record for parsing again */
                        QSpyRecord_init(&qrec,
                            l_record, (int32_t)(l_pos - l_record));
                    }
                }
                if (parse) {
                    QSpyRecord_process(&qrec);
                }
            }

            /* get ready for the next record ... */
            l_chksum = (uint8_t)0;
            l_pos = l_record;
            l_esc = (uint8_t)0;
        }
        else {  /* a regular un-escaped byte */
            l_chksum = (uint8_t)(l_chksum + b);
            if (l_pos < &l_record[sizeof(l_record)]) {
                *l_pos++ = b;
            }
            else {
                SNPRINTF_LINE("   <COMMS> ERROR    Record too long at "
                           "Seq=%3u,",
                           (unsigned)l_seq);
                if (l_record[1] < sizeof(l_qs_rec)/sizeof(l_qs_rec[0])) {
                    SNPRINTF_APPEND("Rec=%s", l_qs_rec[l_record[1]]);
                }
                else {
                    SNPRINTF_APPEND("Rec=USER+%3u",
                               (unsigned)(l_record[1] - QS_USER));
                }
                QSPY_printError();
                l_chksum = (uint8_t)0;
                l_pos = l_record;
                l_esc = (uint8_t)0;
            }
        }
    }
}

/*..........................................................................*/
char const *QSPY_writeDict(void) {
    FILE *dictFile = (FILE *)0;

    /* do we have dictionary name yet? */
    if (l_dictName[0] == '\0') {
        return (char *)0;  /* no name yet, can't save dictionaries */
    }

    FOPEN_S(dictFile, l_dictName, "w");
    if (dictFile == (FILE *)0) {
        return (char *)0;
    }

    fprintf(dictFile, "-v%03d\n", l_config.version);
    fprintf(dictFile, "-T%01d\n", l_config.tstampSize);
    fprintf(dictFile, "-O%01d\n", l_config.objPtrSize);
    fprintf(dictFile, "-F%01d\n", l_config.funPtrSize);
    fprintf(dictFile, "-S%01d\n", l_config.sigSize);
    fprintf(dictFile, "-E%01d\n", l_config.evtSize);
    fprintf(dictFile, "-Q%01d\n", l_config.queueCtrSize);
    fprintf(dictFile, "-P%01d\n", l_config.poolCtrSize);
    fprintf(dictFile, "-B%01d\n", l_config.poolBlkSize);
    fprintf(dictFile, "-C%01d\n", l_config.tevtCtrSize);

    fprintf(dictFile, "Obj-Dic:\n");
    Dictionary_write(&l_objDict, dictFile);

    fprintf(dictFile, "Fun-Dic:\n");
    Dictionary_write(&l_funDict, dictFile);

    fprintf(dictFile, "Usr-Dic:\n");
    Dictionary_write(&l_usrDict, dictFile);

    fprintf(dictFile, "Sig-Dic:\n");
    SigDictionary_write(&l_sigDict, dictFile);

    fprintf(dictFile, "Msc-Dic:\n");
    Dictionary_write(&l_mscDict, dictFile);

    fclose(dictFile);
    return l_dictName;
}
/*..........................................................................*/
QSpyStatus QSPY_readDict(void *dictFile) {
    char type[10];

    while (fgets(type, sizeof(type), (FILE *)dictFile) != (char *)0) {
        switch (type[0]) {
            case '-':
                switch (type[1]) {
                    case 'v':
                        l_config.version = 100U*(type[2] - '0')
                                           + 10U*(type[3] - '0')
                                           + (type[4] - '0');
                        printf("-v%03d\n", l_config.version);
                        break;
                    case 'T':
                        l_config.tstampSize = (type[2] - '0');
                        printf("-T%1d\n", l_config.tstampSize);
                        break;
                    case 'O':
                        l_config.objPtrSize = (type[2] - '0');
                        printf("-O%1d\n", l_config.tstampSize);
                        break;
                    case 'F':
                        l_config.funPtrSize = (type[2] - '0');
                        printf("-F%1d\n", l_config.objPtrSize);
                        break;
                    case 'S':
                        l_config.sigSize = (type[2] - '0');
                        printf("-S%1d\n", l_config.sigSize);
                        break;
                    case 'E':
                        l_config.evtSize = (type[2] - '0');
                        printf("-E%1d\n", l_config.evtSize);
                        break;
                    case 'Q':
                        l_config.queueCtrSize = (type[2] - '0');
                        printf("-Q%1d\n", l_config.queueCtrSize);
                        break;
                    case 'P':
                        l_config.poolCtrSize = (type[2] - '0');
                        printf("-P%1d\n", l_config.poolCtrSize);
                        break;
                    case 'B':
                        l_config.poolBlkSize = (type[2] - '0');
                        printf("-B%1d\n", l_config.poolBlkSize);
                        break;
                    case 'C':
                        l_config.tevtCtrSize = (type[2] - '0');
                        printf("-C%1d\n", l_config.tevtCtrSize);
                        break;
                    default:
                        return QSPY_ERROR;
                        break;
                }
                break;
            case 'O':
                if (!Dictionary_read(&l_objDict, (FILE *)dictFile)) {
                    return QSPY_ERROR;
                }
                break;
            case 'F':
                if (!Dictionary_read(&l_funDict, (FILE *)dictFile)) {
                    return QSPY_ERROR;
                }
                break;
            case 'U':
                if (!Dictionary_read(&l_usrDict, (FILE *)dictFile)) {
                    return QSPY_ERROR;
                }
                break;
            case 'S':
                if (!SigDictionary_read(&l_sigDict, (FILE *)dictFile)) {
                    return QSPY_ERROR;
                }
                break;
            case 'M':
                if (!Dictionary_read(&l_mscDict, (FILE *)dictFile)) {
                    return QSPY_ERROR;
                }
                break;
            default:
                return QSPY_ERROR;
                break;
        }
    }

    return QSPY_SUCCESS;
}
/*..........................................................................*/
SigType QSPY_findSig(char const *name, ObjType obj) {
    return SigDictionary_findSig(&l_sigDict, name, obj);
}
/*..........................................................................*/
KeyType QSPY_findObj(char const *name) {
    return Dictionary_findKey(&l_objDict, name);
}
/*..........................................................................*/
KeyType QSPY_findFun(char const *name) {
    return Dictionary_findKey(&l_funDict, name);
}
/*..........................................................................*/
KeyType QSPY_findUsr(char const *name) {
    return Dictionary_findKey(&l_usrDict, name);
}

/*..........................................................................*/
static char const *getMatDict(char const *s) {
    static char dict[65];
    char *pc = dict;
    while ((*s != '\0') && (pc < &dict[sizeof(dict)] - 1)) {
        if ((*s == '[') || (*s == ']')
            || (*s == '.') || (*s == ':'))
        {
            *pc++ = '_';
            ++s;
        }
        else {
            *pc++ = *s++;
        }
    }
    *pc = '\0';
    return (dict[0] == '&' ? dict+1 : dict);
}

/****************************************************************************/
static int Dictionary_comp(void const *arg1, void const *arg2) {
    KeyType key1 = ((DictEntry const *)arg1)->key;
    KeyType key2 = ((DictEntry const *)arg2)->key;
    if (key1 > key2) {
        return 1;
    }
    else if (key1 < key2) {
        return -1;
    }
    else {
        return 0;
    }
}

/*..........................................................................*/
static void Dictionary_ctor(Dictionary * const me,
                            DictEntry *sto, uint32_t capacity)
{
    me->sto      = sto;
    me->capacity = capacity;
    me->entries  = 0;
    me->keySize  = 4;
}
/*..........................................................................*/
static void Dictionary_config(Dictionary * const me, int keySize) {
    me->keySize = keySize;
}
/*..........................................................................*/
static char const *Dictionary_at(Dictionary * const me, unsigned idx) {
    if (idx < (unsigned)me->entries) {
        return me->sto[idx].name;
    }
    else {
        return "";
    }
}
/*..........................................................................*/
static void Dictionary_put(Dictionary * const me,
                           KeyType key, char const *name)
{
    int idx = Dictionary_find(me, key);
    int n = me->entries;
    char *dst;
    if (idx >= 0) { /* the key found? */
        Q_ASSERT((idx <= n) || (n == 0));
        dst = me->sto[idx].name;
        STRNCPY_S(dst, name, sizeof(me->sto[idx].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
    }
    else if (n < me->capacity - 1) {
        me->sto[n].key = key;
        dst = me->sto[n].name;
        STRNCPY_S(dst, name, sizeof(me->sto[n].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
        ++me->entries;
        /* keep the entries sorted by the key */
        qsort(me->sto, (size_t)me->entries, sizeof(me->sto[0]),
              &Dictionary_comp);
    }
}
/*..........................................................................*/
static char const *Dictionary_get(Dictionary * const me,
                                  KeyType key, char *buf)
{
    int idx;
    if (key == 0) {
        return "NULL";
    }
    idx = Dictionary_find(me, key);
    if (idx >= 0) { /* key found? */
        return me->sto[idx].name;
    }
    else { /* key not found */
        if (buf == 0) { /* extra buffer not provided? */
            buf = me->notFound.name; /* use the internal location */
        }
        /* otherwise use the provided buffer... */
        if (me->keySize <= 4) {
            SNPRINTF_S(buf, MAX_NAME_LEN, "0x%08X", (unsigned)key);
        }
        else {
            SNPRINTF_S(buf, MAX_NAME_LEN, "0x%016"PRIX64"", key);
        }
        //Dictionary_put(me, key, buf); /* put into the dictionary */
        return buf;
    }
}
/*..........................................................................*/
static int Dictionary_find(Dictionary * const me, KeyType key) {
    /* binary search algorithm... */
    int mid;
    int first = 0;
    int last = me->entries;
    while (first <= last) {
        mid = (first + last) / 2;
        if (me->sto[mid].key == key) {
            return mid;
        }
        if (me->sto[mid].key > key) {
            last = mid - 1;
        }
        else {
            first = mid + 1;
        }
    }

    return -1; /* entry not found */
}
/*..........................................................................*/
static KeyType Dictionary_findKey(Dictionary * const me, char const *name) {
    /* brute-force search algorithm... */
    int i;
    for (i = 0; i < me->entries; ++i) {
        if (strncmp(me->sto[i].name, name, sizeof(me->sto[i].name)) == 0) {
            return me->sto[i].key;
        }
    }
    return (KeyType)0; /* not found */
}
/*..........................................................................*/
static void Dictionary_reset(Dictionary * const me) {
    int i;
    for (i = 0; i < me->capacity; ++i) {
        me->sto[i].key = (KeyType)0;
    }
    me->entries = 0;
}
/*..........................................................................*/
static void Dictionary_write(Dictionary const * const me, FILE *stream) {
    int i;

    fprintf(stream, "%d %d\n", me->entries, me->keySize);
    for (i = 0; i < me->entries; ++i) {
        DictEntry const *e = &me->sto[i];
        if (me->keySize <= 4) {
            fprintf(stream, "0x%08X %s\n", (unsigned)e->key, e->name);
        }
        else {
            fprintf(stream, "0x%016"PRIX64" %s\n", e->key, e->name);
        }
        QSPY_onPrintLn();

    }
}
/*..........................................................................*/
static bool Dictionary_read(Dictionary * const me, FILE *stream) {
    char dictLine[80];
    int i;

    if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
        goto error;
    }
    if (SSCANF_S(dictLine, "%d %d\n", &me->entries, &me->keySize) != 2) {
        goto error;
    }
    if ((me->entries > me->capacity) || (me->keySize > 8)) {
        goto error;
    }
    for (i = 0; i < me->entries; ++i) {
        DictEntry *e = &me->sto[i];
        if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
            goto error;
        }
        if (me->keySize <= 4) {
            unsigned key;
            if (SSCANF_S(dictLine, "0x%08X %s\n", &key, e->name) != 2) {
                goto error;
            }
            e->key = key;
        }
        else {
            if (SSCANF_S(dictLine, "0x%016"PRIX64" %s\n",
                &e->key, e->name) != 2)
            {
                goto error;
            }
        }
    }
    return true;

error:
    Dictionary_reset(me);
    return false;
}

/*--------------------------------------------------------------------------*/
static int SigDictionary_comp(void const *arg1, void const *arg2) {
    SigType sig1 = ((SigDictEntry const *)arg1)->sig;
    SigType sig2 = ((SigDictEntry const *)arg2)->sig;
    if (sig1 > sig2) {
        return 1;
    }
    else if (sig1 < sig2) {
        return -1;
    }
    else { /* sig1 == sig2 */
        return 0;
    }
}
/*..........................................................................*/
static void SigDictionary_ctor(SigDictionary * const me,
                        SigDictEntry *sto, uint32_t capacity)
{
    me->sto      = sto;
    me->capacity = capacity;
    me->entries  = 0;
    me->ptrSize  = 4;
}
/*..........................................................................*/
static void SigDictionary_config(SigDictionary * const me, int ptrSize) {
    me->ptrSize = ptrSize;
}
/*..........................................................................*/
static void SigDictionary_put(SigDictionary * const me,
                              SigType sig, ObjType obj, char const *name)
{
    int idx = SigDictionary_find(me, sig, obj);
    int n = me->entries;
    char *dst;
    if (idx >= 0) { /* the key found? */
        Q_ASSERT((idx <= n) || (n == 0));
        me->sto[idx].obj = obj;
        dst = me->sto[idx].name;
        STRNCPY_S(dst, name, sizeof(me->sto[idx].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
    }
    else if (n < me->capacity - 1) {
        me->sto[n].sig = sig;
        me->sto[n].obj = obj;
        dst = me->sto[n].name;
        STRNCPY_S(dst, name, sizeof(me->sto[n].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
        ++me->entries;
        /* keep the entries sorted by the sig */
        qsort(me->sto, (size_t)me->entries, sizeof(me->sto[0]),
              &SigDictionary_comp);
    }
}
/*..........................................................................*/
static char const *SigDictionary_get(SigDictionary * const me,
                                     SigType sig, ObjType obj, char *buf)
{
    int idx;
    if (sig == 0) {
        return "NULL";
    }
    idx = SigDictionary_find(me, sig, obj);
    if (idx >= 0) {
        return me->sto[idx].name;
    }
    else { /* key not found */
        if (buf == 0) { /* extra buffer not provided? */
            buf = me->notFound.name; /* use the internal location */
        }
        /* otherwise use the provided buffer... */
        if (me->ptrSize <= 4) {
            SNPRINTF_S(buf, MAX_NAME_LEN, "%08d,Obj=0x%08X",
                      (int)sig, (int)obj);
        }
        else {
            SNPRINTF_S(buf, MAX_NAME_LEN, "%08d,Obj=0x%016"PRIX64"",
                       (int)sig, obj);
        }
        //SigDictionary_put(me, sig, obj, buf); /* put into the dictionary */
        return buf;
    }
}
/*..........................................................................*/
static int SigDictionary_find(SigDictionary * const me,
                              SigType sig, ObjType obj)
{
    /* binary search algorithm ... */
    int mid;
    int first = 0;
    int last = me->entries;
    while (first <= last) {
        mid = (first + last) / 2;
        if (me->sto[mid].sig == sig) {
            if (obj == 0) { /* global/generic signal? */
                return mid;
            }
            else {
                first = mid;
                do {
                    if ((me->sto[first].obj == 0)
                        || (me->sto[first].obj == obj))
                    {
                        return first;
                    }
                    --first;
                } while ((first >= 0) && (me->sto[first].sig == sig));
                last = mid + 1;
                while ((last < me->entries) && (me->sto[last].sig == sig)) {
                    if ((me->sto[last].obj == 0)
                        || (me->sto[last].obj == obj))
                    {
                        return last;
                    }
                    ++last;
                }
                return -1;  /* entry not found */
            }
        }
        if (me->sto[mid].sig > sig) {
            last = mid - 1;
        }
        else {
            first = mid + 1;
        }
    }

    return -1; /* entry not found */
}
/*..........................................................................*/
static SigType SigDictionary_findSig(SigDictionary * const me,
                                     char const *name, ObjType obj)
{
    /* brute-force search algorithm... */
    int i;
    for (i = 0; i < me->entries; ++i) {
        if (((me->sto[i].obj == obj) || (me->sto[i].obj == (ObjType)0))
            && (strncmp(me->sto[i].name, name, sizeof(me->sto[i].name)) == 0))
        {
            return me->sto[i].sig;
        }
    }
    return (SigType)0; /* not found */
}
/*..........................................................................*/
static void SigDictionary_reset(SigDictionary * const me) {
    int i;
    for (i = 0; i < me->capacity; ++i) {
        me->sto[i].sig = (SigType)0;
    }
    me->entries = 0;
}
/*..........................................................................*/
static void SigDictionary_write(SigDictionary const * const me,
                                FILE *stream)
{
    int i;

    fprintf(stream, "%d %d\n", me->entries, me->ptrSize);
    for (i = 0; i < me->entries; ++i) {
        SigDictEntry const *e = &me->sto[i];
        if (me->ptrSize <= 4) {
            fprintf(stream, "%08d 0x%08X %s\n",
                    e->sig, (unsigned)e->obj, e->name);
        }
        else {
            fprintf(stream, "%08d 0x%016"PRIX64" %s\n",
                    e->sig, e->obj, e->name);
        }
    }
}
/*..........................................................................*/
static bool SigDictionary_read(SigDictionary * const me, FILE *stream) {
    char dictLine[80];
    int i;

    if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
        goto error;
    }
    if (SSCANF_S(dictLine, "%d %d\n", &me->entries, &me->ptrSize) != 2) {
        goto error;
    }
    if ((me->entries > me->capacity) || (me->ptrSize > 8)) {
        goto error;
    }
    for (i = 0; i < me->entries; ++i) {
        SigDictEntry *e = &me->sto[i];
        if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
            goto error;
        }
        if (me->ptrSize <= 4) {
            unsigned obj;
            if (SSCANF_S(dictLine, "%08d 0x%08X %s\n",
                &e->sig, &obj, e->name) != 3)
            {
                goto error;
            }
            e->obj = obj;
        }
        else {
            if (SSCANF_S(dictLine, "%08X %016"PRIX64" %s\n",
                &e->sig, &e->obj, e->name) != 3)
            {
                goto error;
            }
        }
    }
    return true;

error:
    SigDictionary_reset(me);
    return false;
}
