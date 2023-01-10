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
* @date Last updated on: 2023-01-19
* @version Last updated for version: 7.2.1
*
* @file
* @brief QSPY host utility: main parser
*/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#include "safe_std.h"   /* "safe" <stdio.h> and <string.h> facilities */
#include "qspy.h"       /* QSPY data parser */
#include "pal.h"        /* Platform Abstraction Layer */

#define Q_SPY   1       /* this is QP implementation */
#define QP_IMPL 1       /* this is QP implementation */
#include "qpc_qs.h"     /* QS target-resident interface */
#include "qpc_qs_pkg.h" /* QS package-scope interface */

/* global objects ..........................................................*/
QSPY_LastOutput QSPY_output;
char const * const QSPY_line = &QSPY_output.buf[QS_LINE_OFFSET];

QSpyConfig    QSPY_conf;
Dictionary    QSPY_funDict;
Dictionary    QSPY_objDict;
Dictionary    QSPY_usrDict;
SigDictionary QSPY_sigDict;
Dictionary    QSPY_enumDict[8];

/*==========================================================================*/
enum {
    OLD_QS_USER = 70,  /* old QS_USER used before QS 6.6.0 */
};

/*..........................................................................*/
static DictEntry     l_funSto[512];
static DictEntry     l_objSto[256];
static DictEntry     l_usrSto[128 + 1 - OLD_QS_USER];
static SigDictEntry  l_sigSto[512];
static DictEntry     l_enumSto[8][256];

/*..........................................................................*/
static FILE         *l_matFile;
static uint32_t      l_userRec;
static QSPY_CustParseFun l_custParseFun;
static QSPY_resetFun     l_txResetFun;

/* QS record names... NOTE: keep in synch with qspy_qs.h */
QSpyRecRender const QSPY_rec[QS_USER] = {
    { "QS_EMPTY",                         GRP_INF },

    /* [1] QEP records */
    { "QS_QEP_STATE_ENTRY",               GRP_SM },
    { "QS_QEP_STATE_EXIT",                GRP_SM },
    { "QS_QEP_STATE_INIT",                GRP_SM },
    { "QS_QEP_INIT_TRAN",                 GRP_SM },
    { "QS_QEP_INTERN_TRAN",               GRP_SM },
    { "QS_QEP_TRAN",                      GRP_SM },
    { "QS_QEP_IGNORED",                   GRP_SM },
    { "QS_QEP_DISPATCH",                  GRP_SM },
    { "QS_QEP_UNHANDLED",                 GRP_SM },

    /* [10] QF (AP) records */
    { "QS_QF_ACTIVE_DEFER",               GRP_AO },
    { "QS_QF_ACTIVE_RECALL",              GRP_AO },
    { "QS_QF_ACTIVE_SUBSCRIBE",           GRP_AO },
    { "QS_QF_ACTIVE_UNSUBSCRIBE",         GRP_AO },
    { "QS_QF_ACTIVE_POST",                GRP_AO },
    { "QS_QF_ACTIVE_POST_LIFO",           GRP_AO },
    { "QS_QF_ACTIVE_GET",                 GRP_AO },
    { "QS_QF_ACTIVE_GET_LAST",            GRP_AO },
    { "QS_QF_ACTIVE_RECALL_ATTEMPT",      GRP_AO },

    /* [19] QF (EQ) records */
    { "QS_QF_EQUEUE_POST",                GRP_EQ },
    { "QS_QF_EQUEUE_POST_LIFO",           GRP_EQ },
    { "QS_QF_EQUEUE_GET",                 GRP_EQ },
    { "QS_QF_EQUEUE_GET_LAST",            GRP_EQ },

    /* [23] QF records */
    { "QS_QF_NEW_ATTEMPT",                GRP_QF },

    /* [24] Memory Pool (MP) records */
    { "QS_QF_MPOOL_GET",                  GRP_MP },
    { "QS_QF_MPOOL_PUT",                  GRP_MP },

    /* [26] Additional Framework (QF) records */
    { "QS_QF_PUBLISH",                    GRP_QF },
    { "QS_QF_NEW_REF",                    GRP_QF },
    { "QS_QF_NEW",                        GRP_QF },
    { "QS_QF_GC_ATTEMPT",                 GRP_QF },
    { "QS_QF_GC",                         GRP_QF },
    { "QS_QF_TICK",                       GRP_QF },

    /* [32] Time Event (TE) records */
    { "QS_QF_TIMEEVT_ARM",                GRP_TE },
    { "QS_QF_TIMEEVT_AUTO_DISARM",        GRP_TE },
    { "QS_QF_TIMEEVT_DISARM_ATTEMPT",     GRP_TE },
    { "QS_QF_TIMEEVT_DISARM",             GRP_TE },
    { "QS_QF_TIMEEVT_REARM",              GRP_TE },
    { "QS_QF_TIMEEVT_POST",               GRP_TE },

    /* [38] Additional Framework (QF) records */
    { "QS_QF_DELETE_REF",                 GRP_QF },
    { "QS_QF_CRIT_ENTRY",                 GRP_QF },
    { "QS_QF_CRIT_EXIT",                  GRP_QF },
    { "QS_QF_ISR_ENTRY",                  GRP_QF },
    { "QS_QF_ISR_EXIT",                   GRP_QF },
    { "QS_QF_INT_DISABLE",                GRP_QF },
    { "QS_QF_INT_ENABLE",                 GRP_QF },

    /* [45] Additional Active Object (AO) records */
    { "QS_QF_ACTIVE_POST_ATTEMPT",        GRP_AO },

    /* [46] Additional Event Queue (EQ) records */
    { "QS_QF_EQUEUE_POST_ATTEMPT",        GRP_EQ },

    /* [47] Additional Memory Pool (MP) records */
    { "QS_QF_MPOOL_GET_ATTEMPT",          GRP_MP },

    /* [48] Scheduler (SC) records */
    { "QS_SCHED_PREEMPT",                 GRP_SC },
    { "QS_SCHED_RESTORE",                 GRP_SC },
    { "QS_SCHED_LOCK",                    GRP_SC },
    { "QS_SCHED_UNLOCK",                  GRP_SC },
    { "QS_SCHED_NEXT",                    GRP_SC },
    { "QS_SCHED_IDLE",                    GRP_SC },
    { "QS_ENUM_DICT",                     GRP_DIC },

    /* [55] Additional QEP records */
    { "QS_QEP_TRAN_HIST",                 GRP_SM },
    { "QS_QEP_TRAN_EP",                   GRP_SM },
    { "QS_QEP_TRAN_XP",                   GRP_SM },

    /* [58] Miscellaneous QS records (not maskable) */
    { "QS_TEST_PAUSED",                   GRP_TST },
    { "QS_TEST_PROBE_GET",                GRP_TST },
    { "QS_SIG_DICT",                      GRP_DIC },
    { "QS_OBJ_DICT",                      GRP_DIC },
    { "QS_FUN_DICT",                      GRP_DIC },
    { "QS_USR_DICT",                      GRP_DIC },
    { "QS_TARGET_INFO",                   GRP_INF },
    { "QS_TARGET_DONE",                   GRP_TST },
    { "QS_RX_STATUS",                     GRP_TST },
    { "QS_QUERY_DATA",                    GRP_TST },
    { "QS_PEEK_DATA",                     GRP_TST },
    { "QS_ASSERT_FAIL",                   GRP_ERR },
    { "QS_QF_RUN",                        GRP_INF },

    /* [71] Semaphore (SEM) records */
    { "QS_SEM_TAKE",                      GRP_SEM },
    { "QS_SEM_BLOCK",                     GRP_SEM },
    { "QS_SEM_SIGNAL",                    GRP_SEM },
    { "QS_SEM_BLOCK_ATTEMPT",             GRP_SEM },

    /* [75] Mutex (MTX) records */
    { "QS_MTX_LOCK",                      GRP_MTX },
    { "QS_MTX_BLOCK",                     GRP_MTX },
    { "QS_MTX_UNLOCK",                    GRP_MTX },
    { "QS_MTX_LOCK_ATTEMPT",              GRP_MTX },
    { "QS_MTX_BLOCK_ATTEMPT",             GRP_MTX },
    { "QS_MTX_UNLOCK_ATTEMPT",            GRP_MTX },

    /* [81] reserved and unused */
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
    { "QS_RESERVED",                      GRP_ERR },
};

/* QS object kinds... NOTE: keep in synch with qspy_qs.h */
static char const *  l_qs_obj[] = {
    "SM",
    "AO",
    "MP",
    "EQ",
    "TE",
    "AP",
    "SM_AO"
};

/* QS-RX record names... NOTE: keep in synch with qspy_qs_pkg.h */
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
    "QS_RX_QUERY_CURR",
    "QS_RX_EVENT"
};

/* facilities for QSPY host application only (but not for QSPY parser) */
#ifdef QSPY_APP

#define FPRINF_MATFILE(format_, ...)                \
    if (l_matFile != (FILE *)0) {                   \
        FPRINTF_S(l_matFile, format_, __VA_ARGS__); \
    } else (void)0

#else

#define FPRINF_MATFILE(format_, ...)   ((void)0)

#endif /* QSPY_APP */

/*==========================================================================*/
void QSPY_config(QSpyConfig const *config,
                 QSPY_CustParseFun custParseFun)
{
    MEMMOVE_S(&QSPY_conf, sizeof(QSPY_conf), config, sizeof(*config));
    l_custParseFun = custParseFun;

    Dictionary_ctor(&QSPY_funDict, l_funSto,
                    sizeof(l_funSto)/sizeof(l_funSto[0]));
    Dictionary_config(&QSPY_funDict, QSPY_conf.funPtrSize);

    Dictionary_ctor(&QSPY_objDict, l_objSto,
                    sizeof(l_objSto)/sizeof(l_objSto[0]));
    Dictionary_config(&QSPY_objDict, QSPY_conf.objPtrSize);

    Dictionary_ctor(&QSPY_usrDict, l_usrSto,
                    sizeof(l_usrSto)/sizeof(l_usrSto[0]));
    Dictionary_config(&QSPY_usrDict, 1);

    SigDictionary_ctor(&QSPY_sigDict, l_sigSto,
                       sizeof(l_sigSto)/sizeof(l_sigSto[0]));
    SigDictionary_config(&QSPY_sigDict, QSPY_conf.objPtrSize);

    for (unsigned i = 0U;
         i < sizeof(QSPY_enumDict)/sizeof(QSPY_enumDict[0]);
         ++i)
    {
        Dictionary_ctor(&QSPY_enumDict[i], l_enumSto[i],
                        sizeof(l_enumSto[i])/sizeof(l_enumSto[i][0]));
        Dictionary_config(&QSPY_enumDict[i], 1);
    }

    QSPY_conf.tstamp[5] = 0U; /* invalidate the year-part of the timestamp */
    l_userRec = ((QSPY_conf.version < 660U) ? OLD_QS_USER : QS_USER);
}
/*..........................................................................*/
void QSPY_cleanup(void) {
    QSPY_configMatFile((void*)0);
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
void QSpyRecord_init(QSpyRecord * const me,
                     uint8_t const *start, uint32_t tot_len)
{
    me->start   = start;
    me->tot_len = (uint32_t)tot_len;
    me->len     = (uint32_t)(tot_len - 3U);
    me->pos     = start + 2;
    me->rec     = start[1];

    /* set the current QS record-ID for any subsequent output */
    QSPY_output.rec  = me->rec;
    QSPY_output.rx_status = -1;
}
/*..........................................................................*/
QSpyStatus QSpyRecord_OK(QSpyRecord * const me) {
    if (me->len != 0) {
        SNPRINTF_LINE("   <COMMS> %s", "ERROR    ");
        if (me->len > 0) {
            SNPRINTF_APPEND("%d bytes unused in ", me->len);
        }
        else {
            SNPRINTF_APPEND("%d bytes needed in ", (-me->len));
        }

        /* is this a pre-defined QS record? */
        if (me->rec < l_userRec) {
            SNPRINTF_APPEND("Rec=%s", QSPY_rec[me->rec].name);
        }
        else { /* application-specific (user) record */
            SNPRINTF_APPEND("Rec=USER+%3d", (int)(me->rec - l_userRec));
        }
        QSPY_onPrintLn();
        return QSPY_ERROR;
    }
    return QSPY_SUCCESS;
}
/*..........................................................................*/
uint32_t QSpyRecord_getUint32(QSpyRecord * const me, uint8_t size) {
    uint32_t ret = 0U;

    if (me->len >= size) {
        if (size == 1U) {
            ret = (uint32_t)me->pos[0];
        }
        else if (size == 2U) {
            ret = (((uint32_t)me->pos[1] << 8) | (uint32_t)me->pos[0]);
        }
        else if (size == 4U) {
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
        if (size == 1U) {
            ret = (uint32_t)me->pos[0];
            ret <<= 24;
            ret >>= 24; /* sign-extend */
        }
        else if (size == 2U) {
            ret = ((uint32_t)me->pos[1] << 8)
                        | (uint32_t)me->pos[0];
            ret <<= 16;
            ret >>= 16; /* sign-extend */
        }
        else if (size == 4U) {
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
    uint64_t ret = 0U;

    if (me->len >= size) {
        if (size == 1U) {
            ret = (uint64_t)me->pos[0];
        }
        else if (size == 2U) {
            ret = (((uint64_t)me->pos[1] << 8)
                       | (uint32_t)me->pos[0]);
        }
        else if (size == 4U) {
            ret = ((((((uint64_t)me->pos[3] << 8)
                        | (uint64_t)me->pos[2]) << 8)
                          | (uint64_t)me->pos[1]) << 8)
                            | (uint64_t)me->pos[0];
        }
        else if (size == 8U) {
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
        if (size == 1U) {
            ret = (uint64_t)me->pos[0];
            ret <<= 56;
            ret >>= 56; /* sign-extend */
        }
        else if (size == 2U) {
            ret = (((uint64_t)me->pos[1] << 8)
                       | (uint32_t)me->pos[0]);
            ret <<= 48;
            ret >>= 48; /* sign-extend */
        }
        else if (size == 4U) {
            ret = ((((((uint64_t)me->pos[3] << 8)
                        | (uint64_t)me->pos[2]) << 8)
                          | (uint64_t)me->pos[1]) << 8)
                            | (uint64_t)me->pos[0];
            ret <<= 32;
            ret >>= 32; /* sign-extend */
        }
        else if (size == 8U) {
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

    /* is the string empty? */
    if (*me->pos == 0U) {
        /* adjust the stream for the next token */
        --me->len;
        ++me->pos;

        /* return explicit empty string as two single-quotes '' */
        return "''";
    }

    for (l = me->len, p = me->pos; l > 0; --l, ++p) {
        if (*p == 0U) { /* zero-terminated end of the string? */
            char const *s = (char const *)me->pos;

            /* adjust the stream for the next token */
            me->len = l - 1;
            me->pos = p + 1;
            return s; /* normal return */
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
    *pNum = 0U;
    QSPY_onPrintLn();

    return (uint8_t *)0;
}

/*==========================================================================*/
/* application-specific (user) QS records... */
static void QSpyRecord_processUser(QSpyRecord * const me) {
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
        "%18"PRIi64, "%"PRIi64, "%22"PRIi64, "%24"PRIi64,
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

    u32 = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    i32 = Dictionary_find(&QSPY_usrDict, me->rec);
    if (i32 >= 0) {
        SNPRINTF_LINE("%010u %s", u32, Dictionary_at(&QSPY_usrDict, i32));
    }
    else {
        SNPRINTF_LINE("%010u USER+%03d", u32, (int)(me->rec - l_userRec));
    }

    FPRINF_MATFILE("%d %u", (int)me->rec, u32);

    while (me->len > 0) {
        /* get the format byte */
        uint32_t fmt = QSpyRecord_getUint32(me, 1);
        uint32_t width = (fmt >> 4U) & 0x0FU;
        bool is_hex = (width == (uint32_t)QS_HEX_FMT);
        fmt &= 0x0FU;

        SNPRINTF_APPEND("%c", ' ');
        FPRINF_MATFILE("%c", ' ');

        char const *s;
        switch (fmt) {
            case QS_I8_ENUM_T: {
                if ((width & 0x8U) == 0U) { /* QS_I8() data element */
                    i32 = QSpyRecord_getInt32(me, 1);
                    SNPRINTF_APPEND(ifmt[width], (long)i32);
                    FPRINF_MATFILE(ifmt[width], (long)i32);
                }
                else { /* QS_ENUM() data element */
                    u32 = QSpyRecord_getUint32(me, 1);
                    SNPRINTF_APPEND("%s",
                        Dictionary_get(&QSPY_enumDict[width & 0x7U],
                                       u32, (char *)0));
                    FPRINF_MATFILE(ufmt[1], (unsigned long)u32);
                }
                break;
            }
            case QS_U8_T: {
                u32 = QSpyRecord_getUint32(me, 1);
                SNPRINTF_APPEND(is_hex ? uhfmt[2] : ufmt[width],
                                (unsigned long)u32);
                FPRINF_MATFILE(ufmt[width], (unsigned long)u32);
                break;
            }
            case QS_I16_T: {
                i32 = QSpyRecord_getInt32(me, 2);
                SNPRINTF_APPEND(ifmt[width], (long)i32);
                FPRINF_MATFILE(ifmt[width], (long)i32);
                break;
            }
            case QS_U16_T: {
                u32 = QSpyRecord_getUint32(me, 2);
                SNPRINTF_APPEND(is_hex ? uhfmt[4] : ufmt[width],
                                (unsigned long)u32);
                FPRINF_MATFILE(ufmt[width], (unsigned long)u32);
                break;
            }
            case QS_I32_T: {
                i32 = QSpyRecord_getInt32(me, 4);
                SNPRINTF_APPEND(ifmt[width], (long)i32);
                FPRINF_MATFILE(ifmt[width], (long)i32);
                break;
            }
            case QS_U32_T: {
                u32 = QSpyRecord_getUint32(me, 4);
                SNPRINTF_APPEND(is_hex ? uhfmt[8] : ufmt[width],
                                (unsigned long)u32);
                FPRINF_MATFILE(ufmt[width], (unsigned long)u32);
                break;
            }
            case QS_F32_T: {
                union {
                   uint32_t u;
                   float    f;
                } x;
                x.u = QSpyRecord_getUint32(me, 4);
                SNPRINTF_APPEND(efmt[width], (double)x.f);
                FPRINF_MATFILE(efmt[width], (double)x.f);
                break;
            }
            case QS_F64_T: {
                union F64Rep {
                    uint64_t u;
                    double   d;
                } data;
                data.u = QSpyRecord_getUint64(me, 8);
                SNPRINTF_APPEND(efmt[width], data.d);
                FPRINF_MATFILE(efmt[width], data.d);
                break;
            }
            case QS_STR_T: {
                s = QSpyRecord_getStr(me);
                SNPRINTF_APPEND("%s", s);
                FPRINF_MATFILE("%s", s);
                break;
            }
            case QS_MEM_T: {
                uint8_t const *mem = QSpyRecord_getMem(me, 1, &u32);
                for (; u32 > 0U; --u32, ++mem) {
                    SNPRINTF_APPEND(" %02X", (unsigned int)*mem);
                    FPRINF_MATFILE(" %03d", (unsigned int)*mem);
                }
                break;
            }
            case QS_SIG_T: {
                u32 = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
                u64 = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                if (u64 != 0U) {
                    SNPRINTF_APPEND("%s,Obj=%s",
                        SigDictionary_get(&QSPY_sigDict, u32, u64, (char *)0),
                        Dictionary_get(&QSPY_objDict, u64, (char *)0));
                }
                else {
                    SNPRINTF_APPEND("%s",
                        SigDictionary_get(&QSPY_sigDict, u32, u64, (char *)0));
                }
                FPRINF_MATFILE("%u %"PRId64, u32, u64);
                break;
            }
            case QS_OBJ_T: {
                u64 = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                SNPRINTF_APPEND("%s",
                    Dictionary_get(&QSPY_objDict, u64, (char *)0));
                FPRINF_MATFILE("%"PRId64, u64);
                break;
            }
            case QS_FUN_T: {
                u64 = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
                SNPRINTF_APPEND("%s",
                    Dictionary_get(&QSPY_funDict, u64, (char *)0));
                FPRINF_MATFILE("%"PRId64, u64);
                break;
            }
            case QS_I64_T: {
                i64 = QSpyRecord_getInt64(me, 8);
                SNPRINTF_APPEND(ilfmt[width], i64);
                FPRINF_MATFILE(ilfmt[width], i64);
                break;
            }
            case QS_U64_T: {
                u64 = QSpyRecord_getUint64(me, 8);
                SNPRINTF_APPEND(is_hex ? "0x%16"PRIX64 : ulfmt[width], u64);
                FPRINF_MATFILE(ulfmt[width], u64);
                break;
            }
            case 0x0FU: { /* former QS_U32_HEX_T */
                u32 = QSpyRecord_getUint32(me, 4);
                SNPRINTF_APPEND(uhfmt[width], (unsigned long)u32);
                FPRINF_MATFILE(uhfmt[width], (unsigned long)u32);
                break;
            }
            default: {
                SNPRINTF_APPEND("%s", "Unknown format");
                me->len = -1;
                break;
            }
        }
    }
    QSPY_onPrintLn();
    FPRINF_MATFILE("%c", '\n');
}

/*==========================================================================*/
/* predefined QS records... */
static void QSpyRecord_process(QSpyRecord * const me) {
    uint32_t t, a, b, c, d, e;
    uint64_t p, q, r;
    char buf[QS_FNAME_LEN_MAX];
    char const *s = 0;
    char const *w = 0;

    switch (me->rec) {
        /* Session start ...................................................*/
        case QS_EMPTY: {
            if (QSPY_conf.version >= 550U) {
                /* silently ignore */
            }
            else {
                if (QSpyRecord_OK(me)) {
                    SNPRINTF_LINE("           Trg-RST  %u",
                                 (unsigned)QSPY_conf.version);
                    QSPY_onPrintLn();

                    QSPY_resetAllDictionaries();
                }
            }
            break;
        }

        /* QEP records .....................................................*/
        case QS_QEP_STATE_ENTRY:
            s = "St-Entry";
            /* fall through */
        case QS_QEP_STATE_EXIT: {
            if (s == 0) s = "St-Exit ";
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("===RTC===> %s Obj=%s,State=%s",
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       Dictionary_get(&QSPY_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %"PRId64" %"PRId64"\n",
                            (int)me->rec, p, q);
            }
            break;
        }
        case QS_QEP_STATE_INIT:
            s = "St-Init ";
            /* fall through */
        case QS_QEP_TRAN_HIST:
            if (s == 0) s = "St-Hist ";
            /* fall through */
        case QS_QEP_TRAN_EP:
            if (s == 0) s = "St-EP   ";
            /* fall through */
        case QS_QEP_TRAN_XP: {
            if (s == 0) s = "St-XP   ";
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            r = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("===RTC===> %s Obj=%s,State=%s->%s",
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       Dictionary_get(&QSPY_funDict, q, (char *)0),
                       Dictionary_get(&QSPY_funDict, r, buf));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %"PRId64" %"PRId64" %"PRId64"\n",
                               (int)me->rec, p, q, r);
            }
            break;
        }
        case QS_QEP_INIT_TRAN: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Init===> Obj=%s,State=%s",
                       t,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       Dictionary_get(&QSPY_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %"PRId64"\n",
                               (int)me->rec, t, p, q);
            }
            break;
        }
        case QS_QEP_INTERN_TRAN: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u =>Intern Obj=%s,Sig=%s,State=%s",
                       t,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       SigDictionary_get(&QSPY_sigDict, a, p, (char *)0),
                       Dictionary_get(&QSPY_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %"PRId64
                               " %"PRId64"\n",
                               (int)me->rec, t, a, p, q);
            }
            break;
        }
        case QS_QEP_TRAN: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            r = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            if (QSpyRecord_OK(me)) {
                w = Dictionary_get(&QSPY_funDict, r, buf);
                SNPRINTF_LINE("%010u ===>Tran "
                       "Obj=%s,Sig=%s,State=%s->%s",
                       t,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       SigDictionary_get(&QSPY_sigDict, a, p, (char *)0),
                       Dictionary_get(&QSPY_funDict, q, (char *)0),
                       w);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %"PRId64" %"PRId64" %"PRId64"\n",
                               (int)me->rec, t, a, p, q, r);
#ifdef QSPY_APP
                if (QSEQ_isActive()) {
                    int obj = QSEQ_find(p);
                    if (obj >= 0) {
                        QSEQ_genTran(t, obj, w);
                    }
                }
#endif
            }
            break;
        }
        case QS_QEP_IGNORED: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u =>Ignore Obj=%s,Sig=%s,State=%s",
                       t,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       SigDictionary_get(&QSPY_sigDict, a, p, (char *)0),
                       Dictionary_get(&QSPY_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %"PRId64" %"PRId64"\n",
                               (int)me->rec, t, a, p, q);
            }
            break;
        }
        case QS_QEP_DISPATCH: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Disp===> Obj=%s,Sig=%s,State=%s",
                       t,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       SigDictionary_get(&QSPY_sigDict, a, p, (char *)0),
                       Dictionary_get(&QSPY_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %"PRId64" %"PRId64"\n",
                               (int)me->rec, t, a, p, q);
            }
            break;
        }
        case QS_QEP_UNHANDLED: {
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("===RTC===> St-Unhnd Obj=%s,Sig=%s,State=%s",
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       SigDictionary_get(&QSPY_sigDict, a, p, (char *)0),
                       Dictionary_get(&QSPY_funDict, q, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %"PRId64"\n",
                               (int)me->rec, a, p, q);
            }
            break;
        }

        /* QF records ......................................................*/
        case QS_QF_ACTIVE_DEFER:
            if (QSPY_conf.version >= 620U) {
                s = "Defer";
            }
            else { /* former QS_QF_ACTIVE_ADD */
                s = "Add  ";
            }
            /* fall through */
        case QS_QF_ACTIVE_RECALL: {
            if (QSPY_conf.version >= 620U) {
                if (s == 0) s = "RCall";
                t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
                p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
                if (QSpyRecord_OK(me)) {
                    SNPRINTF_LINE("%010u AO-%s Obj=%s,Que=%s,"
                                  "Evt<Sig=%s,Pool=%u,Ref=%u>",
                           t,
                           s,
                           Dictionary_get(&QSPY_objDict, p, (char *)0),
                           Dictionary_get(&QSPY_objDict, q, (char *)0),
                           SigDictionary_get(&QSPY_sigDict, a, p, (char *)0),
                           b, c);
                    QSPY_onPrintLn();
                    FPRINF_MATFILE("%d %u %"PRId64" %"PRId64" %u %u %u\n",
                                   (int)me->rec, t, p, q, a, b, c);
#ifdef QSPY_APP
                    if (QSEQ_isActive()) {
                        int obj = QSEQ_find(p);
                        if (obj >= 0) {
                            QSEQ_genAnnotation(t, obj, s);
                        }
                    }
#endif
                }
            }
            else if (me->rec == QS_QF_ACTIVE_RECALL) { /* former... */
                                          /*... QS_QF_ACTIVE_REMOVE */
                if (s == 0) s = "Remov";
                t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
                p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                a = QSpyRecord_getUint32(me, 1);
                if (QSpyRecord_OK(me)) {
                    SNPRINTF_LINE("%010u AO-%s Obj=%s,Pri=%u",
                           t,
                           s,
                           Dictionary_get(&QSPY_objDict, p, (char *)0),
                           a);
                    QSPY_onPrintLn();
                    FPRINF_MATFILE("%d %u %"PRId64" %u\n",
                                   (int)me->rec, t, p, a);
                }
            }
            break;
        }
        case QS_QF_ACTIVE_RECALL_ATTEMPT: {
            if (QSPY_conf.version >= 620U) {
                t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
                p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                if (QSpyRecord_OK(me)) {
                    SNPRINTF_LINE("%010u AO-RCllA Obj=%s,Que=%s",
                           t,
                           Dictionary_get(&QSPY_objDict, p, (char *)0),
                           Dictionary_get(&QSPY_objDict, q, (char *)0));
                    QSPY_onPrintLn();
                    FPRINF_MATFILE("%d %u %"PRId64" %"PRId64"\n",
                                   (int)me->rec, t, p, q);
#ifdef QSPY_APP
                    if (QSEQ_isActive()) {
                        int obj = QSEQ_find(p);
                        if (obj >= 0) {
                            QSEQ_genAnnotation(t, obj, "RCallA");
                        }
                    }
#endif
                }
            }
            else { /* former QS_QF_EQUEUE_INIT */
                p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                b = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
                if (QSpyRecord_OK(me)) {
                    SNPRINTF_LINE("           EQ-Init  Obj=%s,Len=%u",
                           Dictionary_get(&QSPY_objDict, p, (char *)0),
                           b);
                    QSPY_onPrintLn();
                    FPRINF_MATFILE("%d %"PRId64" %u\n",
                                   (int)me->rec, p, b);
                }
            }
            break;
        }

        case QS_QF_ACTIVE_SUBSCRIBE:
            s = "Subsc";
            /* fall through */
        case QS_QF_ACTIVE_UNSUBSCRIBE: {
            if (s == 0) s = "Unsub";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u AO-%s Obj=%s,Sig=%s",
                       t,
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       SigDictionary_get(&QSPY_sigDict, a, p, (char *)0));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %"PRId64"\n",
                               (int)me->rec, t, a, p);
            }
            break;
        }
        case QS_QF_ACTIVE_POST:
            s = "Post ";
            /* fall through */
        case QS_QF_ACTIVE_POST_ATTEMPT: {
            if (s == 0) s = "PostA";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            if (QSPY_conf.version >= 420U) {
                q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            }
            else {
                q = 0U;
            }
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            if (QSPY_conf.version >= 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            d = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
            e = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                w = SigDictionary_get(&QSPY_sigDict, a, p, (char *)0);
                SNPRINTF_LINE("%010u AO-%s Sdr=%s,Obj=%s,"
                       "Evt<Sig=%s,Pool=%u,Ref=%u>,"
                       "Que<Free=%u,%s=%u>",
                       t,
                       s,
                       Dictionary_get(&QSPY_objDict, q, (char *)0),
                       Dictionary_get(&QSPY_objDict, p, buf),
                       w,
                       b, c, d,
                       (me->rec == QS_QF_ACTIVE_POST ? "Min" : "Mar"),
                       e);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %u %"PRId64" %u %u %u %u\n",
                               (int)me->rec, t, q, a, p, b, c, d, e);
#ifdef QSPY_APP
                if (QSEQ_isActive()) {
                    int src = QSEQ_find(q);
                    int dst = QSEQ_find(p);
                    QSEQ_genPost(t, src, dst, w,
                               (me->rec == QS_QF_ACTIVE_POST_ATTEMPT));
                }
#endif
            }
            break;
        }
        case QS_QF_ACTIVE_POST_LIFO: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            if (QSPY_conf.version >= 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            d = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
            e = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                w = SigDictionary_get(&QSPY_sigDict, a, p, (char *)0);
                SNPRINTF_LINE("%010u AO-LIFO  Obj=%s,"
                       "Evt<Sig=%s,Pool=%u,Ref=%u>,"
                       "Que<Free=%u,Min=%u>",
                       t,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       w,
                       b, c, d, e);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %"PRId64" %u %u %u %u\n",
                               (int)me->rec, t, a, p, b, c, d, e);
#ifdef QSPY_APP
                if (QSEQ_isActive()) {
                    int src = QSEQ_find(p);
                    if (src >= 0) {
                        QSEQ_genPostLIFO(t, src, w);
                    }
                }
#endif
            }
            break;
        }
        case QS_QF_ACTIVE_GET:
            s = "AO-Get  ";
            /* fall through */
        case QS_QF_EQUEUE_GET: {
            if (s == 0) s = "EQ-Get  ";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            if (QSPY_conf.version >= 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            d = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Obj=%s,Evt<Sig=%s,Pool=%u,Ref=%u>,"
                       "Que<Free=%u>",
                       t,
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       SigDictionary_get(&QSPY_sigDict, a, p, (char *)0),
                       b, c, d);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %"PRId64" %u %u %u\n",
                               (int)me->rec, t, a, p, b, c, d);
            }
            break;
        }
        case QS_QF_ACTIVE_GET_LAST:
            s = "AO-GetL ";
            /* fall through */
        case QS_QF_EQUEUE_GET_LAST: {
            if (s == 0) s = "EQ-GetL ";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            if (QSPY_conf.version >= 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Obj=%s,Evt<Sig=%s,Pool=%u,Ref=%u>",
                       t,
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       SigDictionary_get(&QSPY_sigDict, a, p, (char *)0),
                       b, c);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %"PRId64" %u %u\n",
                               (int)me->rec, t, a, p, b, c);
            }
            break;
        }

        case QS_QF_EQUEUE_POST:
            s = "Post ";
            w = "Min";
            /* fall through */
        case QS_QF_EQUEUE_POST_ATTEMPT:
            if (s == 0) s = "PostA";
            if (w == 0) w = "Mar";
            /* fall through */
        case QS_QF_EQUEUE_POST_LIFO: {
            if (s == 0) s = "LIFO";
            if (w == 0) w = "Min";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            if (QSPY_conf.version >= 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            d = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
            e = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u EQ-%s Obj=%s,"
                       "Evt<Sig=%s,Pool=%u,Ref=%u>,"
                       "Que<Free=%u,%s=%u>",
                       t,
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       SigDictionary_get(&QSPY_sigDict, a, p, (char *)0),
                       b, c, d,
                       w,
                       e);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %"PRId64" %u %u %u %u\n",
                               (int)me->rec, t, a, p,
                               b, c, d, e);
            }
            break;
        }

        case QS_QF_MPOOL_GET:
            s = "Get  ";
            w = "Min";
            /* fall through */
        case QS_QF_MPOOL_GET_ATTEMPT: {
            if (s == 0) s = "GetA ";
            if (w == 0) w = "Mar";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            b = QSpyRecord_getUint32(me, QSPY_conf.poolCtrSize);
            c = QSpyRecord_getUint32(me, QSPY_conf.poolCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u MP-%s Obj=%s,Free=%u,%s=%u",
                       t,
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       b,
                       w,
                       c);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %u %u\n",
                               (int)me->rec, t, p, b, c);
            }
            break;
        }
        case QS_QF_MPOOL_PUT: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            b = QSpyRecord_getUint32(me, QSPY_conf.poolCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u MP-Put   Obj=%s,Free=%u",
                       t,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %u\n",
                               (int)me->rec, t, p, b);
            }
            break;
        }

        /* QF */
        case QS_QF_NEW_ATTEMPT:
            s = "QF-NewA ";
            /* fall through */
        case QS_QF_NEW: {
            if (s == 0) s = "QF-New  ";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.evtSize);
            c = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Sig=%s,Size=%u",
                       t, s,
                       SigDictionary_get(&QSPY_sigDict, c, 0, (char *)0),
                       a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %u\n",
                               (int)me->rec, t, a, c);
            }
            break;
        }

        case QS_QF_PUBLISH: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            if (QSPY_conf.version >= 420U) {
                p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            else {
                p = 0U;
                a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            if (QSpyRecord_OK(me)) {
                w = SigDictionary_get(&QSPY_sigDict, a, 0, buf);
                SNPRINTF_LINE("%010u QF-Pub   Sdr=%s,"
                       "Evt<Sig=%s,Pool=%u,Ref=%u>",
                       t,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       w,
                       b, c);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %u %u\n",
                               (int)me->rec, t, p, a, b);
#ifdef QSPY_APP
                if (QSEQ_isActive()) {
                    int obj = QSEQ_find(p);
                    QSEQ_genPublish(t, obj, w);
                }
#endif
            }
            break;
        }

        case QS_QF_NEW_REF: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            b = QSpyRecord_getUint32(me, 1);
            c = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u QF-NewRf Evt<Sig=%s,Pool=%u,Ref=%u>",
                       t,
                       SigDictionary_get(&QSPY_sigDict, a, 0, (char *)0),
                       b, c);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %u %u\n",
                               (int)me->rec, t, a, b, c);
            }
            break;
        }

        case QS_QF_DELETE_REF: {
            if (QSPY_conf.version >= 620U) {
                t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
                a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
                if (QSpyRecord_OK(me)) {
                    SNPRINTF_LINE("%010u QF-DelRf Evt<Sig=%s,Pool=%u,Ref=%u>",
                           t,
                           SigDictionary_get(&QSPY_sigDict, a, 0, (char *)0),
                           b, c);
                    QSPY_onPrintLn();
                    FPRINF_MATFILE("%d %u %u %u %u\n",
                                   (int)me->rec, t, a, b, c);
                }
            }
            else { /* former QS_QF_TIMEEVT_CTR */
                t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
                p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                c = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
                d = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
                if (QSPY_conf.version >= 500U) {
                    b = QSpyRecord_getUint32(me, 1);
                }
                else {
                    b = 0U;
                }
                if (QSpyRecord_OK(me)) {
                    SNPRINTF_LINE("%010u TE%1u-Ctr  Obj=%s,AO=%s,"
                           "Tim=%u,Int=%u",
                           t,
                           b,
                           Dictionary_get(&QSPY_objDict, p, (char *)0),
                           Dictionary_get(&QSPY_objDict, q, buf),
                           c, d);
                    QSPY_onPrintLn();
                    FPRINF_MATFILE("%d %u %"PRId64" %"PRId64" %u %u\n",
                                   (int)me->rec, t, p, q, c, d);
                }
            }
            break;
        }

        case QS_QF_GC_ATTEMPT:
            s = "QF-gcA  ";
            /* fall through */
        case QS_QF_GC: {
            if (s == 0) s = "QF-gc   ";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            if (QSPY_conf.version >= 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Evt<Sig=%s,Pool=%d,Ref=%d>",
                       t,
                       s,
                       SigDictionary_get(&QSPY_sigDict, a, 0, (char *)0),
                       b, c);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %u %u\n",
                               (int)me->rec, t, a, b, c);
            }
            break;
        }
        case QS_QF_TICK: {
            a = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
            if (QSPY_conf.version >= 500U) {
                b = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = 0U;
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("           Tick<%1u>  Ctr=%010u",
                        b,
                        a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u\n", (int)me->rec, a);
#ifdef QSPY_APP
                if (QSEQ_isActive()) {
                    QSEQ_genTick(b, a);
                }
#endif
            }
            break;
        }
        case QS_QF_TIMEEVT_ARM:
            s = "Arm ";
            /* fall through */
        case QS_QF_TIMEEVT_DISARM: {
            if (s == 0) s = "Dis ";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            c = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
            d = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
            if (QSPY_conf.version >= 500U) {
                b = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = 0U;
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TE%1u-%s Obj=%s,AO=%s,Tim=%u,Int=%u",
                       t,
                       b,
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       Dictionary_get(&QSPY_objDict, q, buf),
                       c, d);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %"PRId64" %u %u\n",
                               (int)me->rec, t, p, q, c, d);
            }
            break;
        }
        case QS_QF_TIMEEVT_AUTO_DISARM: {
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            if (QSPY_conf.version >= 500U) {
                b = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = 0U;
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("           TE%1u-ADis Obj=%s,AO=%s",
                       b,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       Dictionary_get(&QSPY_objDict, q, buf));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %"PRId64" %"PRId64"\n",
                               (int)me->rec, p, q);
           }
            break;
        }
        case QS_QF_TIMEEVT_DISARM_ATTEMPT: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            if (QSPY_conf.version >= 500U) {
                b = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = 0U;
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TE%1u-DisA Obj=%s,AO=%s",
                       t,
                       b,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       Dictionary_get(&QSPY_objDict, q, buf));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %"PRId64"\n",
                               (int)me->rec, t, p, q);
            }
            break;
        }
        case QS_QF_TIMEEVT_REARM: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            c = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
            d = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
            e = QSpyRecord_getUint32(me, 1);
            if (QSPY_conf.version >= 500U) {
                b = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = 0U;
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TE%1u-Rarm Obj=%s,AO=%s,"
                       "Tim=%u,Int=%u,Was=%1u",
                       t,
                       b,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       Dictionary_get(&QSPY_objDict, q, buf),
                       c, d, e);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %"PRId64" %u %u %u\n",
                               (int)me->rec, t, p, q, c, d, e);
            }
            break;
        }
        case QS_QF_TIMEEVT_POST: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            if (QSPY_conf.version >= 500U) {
                b = QSpyRecord_getUint32(me, 1);
            }
            else {
                b = 0U;
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TE%1u-Post Obj=%s,Sig=%s,AO=%s",
                       t,
                       b,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       SigDictionary_get(&QSPY_sigDict, a, q, (char *)0),
                       Dictionary_get(&QSPY_objDict, q, buf));
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %u %"PRId64"\n",
                               (int)me->rec, t, p, a, q);
            }
            break;
        }
        case QS_QF_CRIT_ENTRY:
            s = "QF-CritE";
            /* fall through */
        case QS_QF_CRIT_EXIT: {
            if (s == 0) s = "QF-CritX";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Nest=%d",
                       t,
                       s,
                       a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u\n",
                               (int)me->rec, t, a);
           }
            break;
        }
        case QS_QF_ISR_ENTRY:
            s = "QF-IsrE";
            /* fall through */
        case QS_QF_ISR_EXIT: {
            if (s == 0) s = "QF-IsrX";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s  Nest=%u,Pri=%u",
                       t,
                       s,
                       a, b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %u\n",
                               (int)me->rec, t, a, b);
            }
            break;
        }

        /* scheduler records ...............................................*/
        case QS_SCHED_PREEMPT:
            if (QSPY_conf.version < 710U) {
                /* old QS_MUTEX_LOCK */
                if (s == 0) s = "Mtx-Lock";
            }
            else {
                if (s == 0) s = "Sch-Pre ";
            }
            /* fall through */
        case QS_SCHED_RESTORE: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                if (QSPY_conf.version < 710U) {
                    /* old QS_MUTEX_UNLOCK */
                    if (s == 0) s = "Mtx-Unlk";
                    SNPRINTF_LINE("%010u %s Pro=%u,Ceil=%u",
                           t, s, a, b);
                    QSPY_onPrintLn();
                    FPRINF_MATFILE("%d %u %u %u\n",
                                   (int)me->rec, t, a, b);
                }
                else {
                    if (s == 0) s = "Sch-Rest";
                    SNPRINTF_LINE("%010u %s Pri=%u->%u",
                           t, s, b, a);
                    QSPY_onPrintLn();
                    FPRINF_MATFILE("%d %u %u %u\n",
                                   (int)me->rec, t, b, a);
                }
            }
            break;
        }

        case QS_SCHED_LOCK:
            if (s == 0) s = "Sch-Lock";
            /* fall through */
        case QS_SCHED_UNLOCK: {
            if (s == 0) s = "Sch-Unlk";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s Ceil=%u->%u",
                       t,
                       s,
                       a, b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %u\n",
                               (int)me->rec, t, a, b);
            }
            break;
        }
        case QS_SCHED_NEXT: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Sch-Next Pri=%u->%u",
                       t, b, a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %u\n",
                               (int)me->rec, t, a, b);
            }
            break;
        }
        case QS_SCHED_IDLE: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Sch-Idle Pri=%u->0",
                       t, a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u\n",
                               (int)me->rec, t, a);
            }
            break;
        }

        /* enum dictionary .................................................*/
        case QS_ENUM_DICT: {
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1) & 0x7U;
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&QSPY_enumDict[b], a, s);
                SNPRINTF_LINE("           Enum-Dic %03d,Grp=%1d->%s",
                              a, b, s);
                QSPY_onPrintLn();
            }
            break;
        }

        /* Miscellaneous built-in QS records ...............................*/
        case QS_TEST_PAUSED: {
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("           %s", "TstPause");
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_TEST_PROBE_GET: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            a = QSpyRecord_getUint32(me, 4U);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u TstProbe Fun=%s,Data=%d",
                              t, Dictionary_get(&QSPY_funDict,
                              q, (char *)0), a);
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_SIG_DICT: {
            a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                SigDictionary_put(&QSPY_sigDict, (SigType)a, p, s);
                if (QSPY_conf.objPtrSize <= 4) {
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
                FPRINF_MATFILE("%d %s=[%u %"PRId64"];\n",
                               (int)me->rec, QSPY_getMatDict(s), a, p);
            }
            break;
        }

        case QS_OBJ_DICT: {
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            s = QSpyRecord_getStr(me);

            /* for backward compatibility replace the '['/']' with '<'/'>' */
            if (QSPY_conf.version < 690U) {
                char *ps;
                for (ps = (char *)s; *ps != '\0'; ++ps) {
                    if (*ps == '[') {
                        *ps = '<';
                    }
                    else if (*ps == ']') {
                        *ps = '>';
                    }
                }
            }
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&QSPY_objDict, p, s);
                if (QSPY_conf.objPtrSize <= 4) {
                    SNPRINTF_LINE("           Obj-Dict 0x%08X->%s",
                                  (unsigned)p, s);
                }
                else {
                    SNPRINTF_LINE("           Obj-Dict 0x%016"PRIX64"->%s",
                                  p, s);
                }
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %s=%"PRId64";\n",
                               (int)me->rec, QSPY_getMatDict(s), p);
#ifdef QSPY_APP
                /* if needed, update the object in the Sequence dictionary */
                QSEQ_updateDictionary(s, p);
#endif
            }
            break;
        }

        case QS_FUN_DICT: {
            p = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&QSPY_funDict, p, s);
                if (QSPY_conf.funPtrSize <= 4) {
                    SNPRINTF_LINE("           Fun-Dict 0x%08X->%s",
                                  (unsigned)p, s);
                }
                else {
                    SNPRINTF_LINE("           Fun-Dict 0x%016"PRIX64"->%s",
                                  p, s);
                }
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %s=%"PRId64";\n",
                            (int)me->rec, QSPY_getMatDict(s), p);
            }
            break;
        }

        case QS_USR_DICT: {
            a = QSpyRecord_getUint32(me, 1);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&QSPY_usrDict, a, s);
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
                s = (a != 0U) ? "Trg-RST " : "Trg-Info";

                /* save the year-part of the timestamp
                * NOTE: (year-part == 0) means that we don't have target info
                */
                c = QSPY_conf.tstamp[5];

                /* apply the target info...
                * find differences from the current config and store in 'd'
                */
                d = 0U; /* assume no difference in the target info */
                CONFIG_UPDATE(version ,    (uint16_t)(b & 0x7FFFU), d);
                CONFIG_UPDATE(endianness,  (uint8_t)((b >> 15) & 0x01U), d);
                CONFIG_UPDATE(objPtrSize,  (uint8_t)(buf[3] & 0xFU), d);
                CONFIG_UPDATE(funPtrSize,  (uint8_t)((buf[3] >> 4) & 0xFU),d);
                CONFIG_UPDATE(tstampSize,  (uint8_t)(buf[4] & 0xFU), d);
                CONFIG_UPDATE(sigSize,     (uint8_t)(buf[0] & 0xFU), d);
                CONFIG_UPDATE(evtSize,     (uint8_t)((buf[0] >> 4) & 0xFU),d);
                CONFIG_UPDATE(queueCtrSize,(uint8_t)(buf[1] & 0x0FU), d);
                CONFIG_UPDATE(poolCtrSize, (uint8_t)((buf[2] >> 4) & 0xFU),d);
                CONFIG_UPDATE(poolBlkSize, (uint8_t)(buf[2] & 0xFU), d);
                CONFIG_UPDATE(tevtCtrSize, (uint8_t)((buf[1] >> 4) & 0xFU),d);

                /* update the user record offset */
                l_userRec = ((QSPY_conf.version < 660U)
                             ? OLD_QS_USER : QS_USER);

                for (e = 0U; e < sizeof(QSPY_conf.tstamp); ++e) {
                    CONFIG_UPDATE(tstamp[e], (uint8_t)buf[7U + e], d);
                }

                SNPRINTF_LINE("           %s QP-Ver=%u,"
                       "Build=%02u%02u%02u_%02u%02u%02u",
                       s,
                       b,
                       (unsigned)QSPY_conf.tstamp[5],
                       (unsigned)QSPY_conf.tstamp[4],
                       (unsigned)QSPY_conf.tstamp[3],
                       (unsigned)QSPY_conf.tstamp[2],
                       (unsigned)QSPY_conf.tstamp[1],
                       (unsigned)QSPY_conf.tstamp[0]);
                QSPY_onPrintLn();

                /* any difference in configuration found
                * and this is not the first target info?
                */
                if ((d != 0U) && (c != 0U)) {
                    SNPRINTF_LINE("   <QSPY-> %s",
                                  "Target info mismatch "
                                  "(dictionaries discarded)");
                    QSPY_printInfo();
                    QSPY_resetAllDictionaries();
                }

                if (a != 0U) {  /* is this also Target RESET? */

                    /* always reset dictionaries upon target reset */
                    QSPY_resetAllDictionaries();

                    /* reset the QSPY-Tx channel, if available */
                    if (l_txResetFun != (QSPY_resetFun)0) {
                        (*l_txResetFun)();
                    }
                    /*TBD: close and re-open MATLAB, Sequence file, etc. */
                }

#ifdef QSPY_APP
                /* should external dictionaries be used (-d option)? */
                if (QDIC_isActive()) {
                    QSPY_readDict();
                }
#endif
            }
            break;
        }

        case QS_TARGET_DONE: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
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
            if (QSPY_conf.version >= 580U) {
            }
            else {
                t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            }
            a = QSpyRecord_getUint32(me, 1U);
            QSPY_output.rx_status = (int)a;
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

        case QS_QUERY_DATA: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, 1U);
            b = 0;
            c = 0;
            d = 0;
            e = 0;
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = 0;
            switch (a) {
                case SM_OBJ:
                    q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
                    break;
                case MP_OBJ:
                    b = QSpyRecord_getUint32(me, QSPY_conf.poolCtrSize);
                    c = QSpyRecord_getUint32(me, QSPY_conf.poolCtrSize);
                    break;
                case EQ_OBJ:
                    b = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
                    c = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
                    break;
                case TE_OBJ:
                    q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                    b = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
                    c = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
                    d = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
                    e = QSpyRecord_getUint32(me, 1);
                    break;
                case AP_OBJ:
                    break;
                default:
                    break;
            }
            if (QSPY_conf.version < 690U) {
                switch (a) {
                    case AO_OBJ:
                        b = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
                        c = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
                        break;
                }
            }
            else {
                switch (a) {
                    case AO_OBJ:
                        q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
                        break;
                }
            }
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Query-%s Obj=%s",
                       t,
                       l_qs_obj[a],
                       Dictionary_get(&QSPY_objDict, p, (char *)0));
                switch (a) {
                    case SM_OBJ:
                        SNPRINTF_APPEND(",State=%s",
                            Dictionary_get(&QSPY_funDict, q, (char *)0));
                        break;
                    case MP_OBJ:
                        SNPRINTF_APPEND(",Free=%u,Min=%u",
                            b, c);
                        break;
                    case EQ_OBJ:
                        SNPRINTF_APPEND(",Que<Free=%u,Min=%u>",
                            b, c);
                        break;
                    case TE_OBJ:
                        SNPRINTF_APPEND(
                            ",Rate=%u,Sig=%s,Tim=%u,Int=%u,Flags=0x%02X",
                            (e & 0x0FU),
                            SigDictionary_get(&QSPY_sigDict, d, q, (char *)0),
                            b, c,
                            (e & 0xF0U));
                        break;
                    case AP_OBJ:
                        break;
                    default:
                        break;
                }
                if (QSPY_conf.version < 690U) {
                    switch (a) {
                        case AO_OBJ:
                            SNPRINTF_APPEND(",Que<Free=%u,Min=%u>",
                                b, c);
                            break;
                    }
                }
                else {
                    switch (a) {
                        case AO_OBJ:
                            SNPRINTF_APPEND(",State=%s",
                                Dictionary_get(&QSPY_funDict, q, (char *)0));
                            break;
                    }
                }
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_PEEK_DATA: {
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, 2);  /* offset */
            b = QSpyRecord_getUint32(me, 1);  /* data size */
            w = (char const *)QSpyRecord_getMem(me, (uint8_t)b, &c);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u Trg-Peek Offs=%d,Size=%d,Num=%d,Data=<",
                              t, a, b, c);
                for (; c > 1U; --c, w += b) {
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
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            a = QSpyRecord_getUint32(me, 2);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u =ASSERT= Mod=%s,Loc=%u",
                       t, s, a);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %u %s\n",
                            (int)me->rec, (unsigned)t, (unsigned)a, s);
            }
            break;
        }

        case QS_QF_RUN: {
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("           %s", "QF_RUN");
                QSPY_onPrintLn();
#ifdef QSPY_APP
                if (QDIC_isActive()) {
                    QSPY_writeDict();
                }
#endif
            }
            break;
        }

        /* Semaphore records ...............................................*/
        case QS_SEM_TAKE:
            if (s == 0) s = "Sem-Take";
            /* fall through */
        case QS_SEM_BLOCK:
            if (s == 0) s = "Sem-Blk ";
            /* fall through */
        case QS_SEM_SIGNAL:
            if (s == 0) s = "Sem-Sgnl";
            /* fall through */
        case QS_SEM_BLOCK_ATTEMPT: {
            if (s == 0) s = "Sem-BlkA";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s %s,Thr=%u,Cnt=%u",
                       t,
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       a, b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %u %u\n",
                               (int)me->rec, (unsigned)t, p, a, b);
            }
            break;
        }

        /* Mutex records ...............................................*/
        case QS_MTX_LOCK:
            if (s == 0) s = "Mtx-Lock";
            /* fall through */
        case QS_MTX_UNLOCK:
            if (s == 0) s = "Mtx-Unlk";
            /* fall through */
        case QS_MTX_LOCK_ATTEMPT:
            if (s == 0) s = "Mtx-LckA";
            /* fall through */
        case QS_MTX_UNLOCK_ATTEMPT: {
            if (s == 0) s = "Mtx-UlkA";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s %s,Hldr=%u,Nest=%u",
                       t,
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       a, b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %u %u\n",
                               (int)me->rec, (unsigned)t, p, a, b);
            }
            break;
        }
        case QS_MTX_BLOCK:
            if (s == 0) s = "Mtx-Blk ";
            /* fall through */
        case QS_MTX_BLOCK_ATTEMPT: {
            if (s == 0) s = "Mtx-BlkA";
            t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINTF_LINE("%010u %s %s,Hldr=%u,Thr=%u",
                       t,
                       s,
                       Dictionary_get(&QSPY_objDict, p, (char *)0),
                       a, b);
                QSPY_onPrintLn();
                FPRINF_MATFILE("%d %u %"PRId64" %u %u\n",
                               (int)me->rec, (unsigned)t, p, a, b);
            }
            break;
        }


        /* Unknown records .................................................*/
        /* NOTE: the Application-Specific records have been already
         * sifted out in QSPY_parse()
         */
        default: {
            SNPRINTF_LINE("           Unknown Rec=%d,Len=%d",
                   (int)me->rec, (int)me->len);
            QSPY_onPrintLn();
            break;
        }
    }
}
/*..........................................................................*/
void QSPY_printInfo(void) {
    QSPY_output.type = INF_OUT; /* this is an internal info message */
    QSPY_onPrintLn();
}
/*..........................................................................*/
void QSPY_printError(void) {
    QSPY_output.type = ERR_OUT; /* this is an error message */
    QSPY_onPrintLn();
}

/*==========================================================================*/
static uint8_t l_record[QS_RECORD_SIZE_MAX];
static uint8_t *l_pos   = l_record; /* position within the record */
static uint8_t l_chksum = 0U;
static uint8_t l_esc    = 0U;
static uint8_t l_seq    = 0U;

/*..........................................................................*/
void QSPY_reset(void) {
    l_pos    = l_record; /* position within the record */
    l_chksum = 0U;
    l_esc    = 0U;
    l_seq    = 0U;
}
/*..........................................................................*/
void QSPY_parse(uint8_t const *buf, uint32_t nBytes) {
    static bool isJustStarted = true;

    for (; nBytes != 0U; --nBytes) {
        uint8_t b = *buf++;

        if (l_esc) { /* escaped byte arrived? */
            l_esc = 0U;
            b ^= QS_ESC_XOR;

            l_chksum = (uint8_t)(l_chksum + b);
            if (l_pos < &l_record[sizeof(l_record)]) {
                *l_pos++ = b;
            }
            else {
                SNPRINTF_LINE("   <COMMS> ERROR    Record too long at "
                           "Seq=%u(?),", (unsigned)l_seq);
                /* is it a standard QS record? */
                if (l_record[1] < l_userRec) {
                    SNPRINTF_APPEND("Rec=%s(?)",
                                    QSPY_rec[l_record[1]].name);
                }
                else { /* this is a USER-specific record */
                    SNPRINTF_APPEND("Rec=USER+%u(?)",
                               (unsigned)(l_record[1] - l_userRec));
                }
                QSPY_printError();
                l_chksum = 0U;
                l_pos = l_record;
                l_esc = 0U;
            }
        }
        else if (b == QS_ESC) {   /* transparent byte? */
            l_esc = 1U;
        }
        else if (b == QS_FRAME) { /* frame byte? */
            if (l_chksum != QS_GOOD_CHKSUM) { /* bad checksum? */
                if (!isJustStarted) {
                    SNPRINTF_LINE("   <COMMS> ERROR    %s",
                                  "Bad checksum in ");
                    if (l_record[1] < l_userRec) {
                        SNPRINTF_APPEND("Rec=%s(?),",
                            QSPY_rec[l_record[1]].name);
                    }
                    else {
                        SNPRINTF_APPEND("Rec=USER+%u(?),",
                            (unsigned)(l_record[1] - l_userRec));
                    }
                    SNPRINTF_APPEND("Seq=%u", (unsigned)l_seq);
                    QSPY_printError();
                }
            }
            else if (l_pos < &l_record[3]) { /* record too short? */
                SNPRINTF_LINE("   <COMMS> ERROR    Record too short at "
                           "Seq=%u(?),",
                           (unsigned)l_seq);
                if (l_record[1] < l_userRec) {
                    SNPRINTF_APPEND("Rec=%s", QSPY_rec[l_record[1]].name);
                }
                else {
                    SNPRINTF_APPEND("Rec=USER+%u(?)",
                               (unsigned)(l_record[1] - l_userRec));
                }
                QSPY_printError();
            }
            else { /* a healty record received */
                QSpyRecord qrec;
                int parse = 1;
                ++l_seq; /* increment with natural wrap-around */

                if (!isJustStarted) {
                    /* data discountinuity found?
                    * but not for the QS_EMPTY record?
                    */
                    if ((l_seq != l_record[0])
                         && (l_record[1] != QS_EMPTY))
                    {
                        SNPRINTF_LINE("   <COMMS> ERROR    Discontinuity "
                            "Seq=%u->%u",
                            (unsigned)(l_seq - 1), (unsigned)l_record[0]);
                        QSPY_printError();
                    }
                }
                else {
                    isJustStarted = false;
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
                    if (qrec.rec < l_userRec) {
                        QSpyRecord_process(&qrec);
                    }
                    else {
                        QSpyRecord_processUser(&qrec);
                    }
                }
            }

            /* get ready for the next record ... */
            l_chksum = 0U;
            l_pos = l_record;
            l_esc = 0U;
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
                if (l_record[1] < l_userRec) {
                    SNPRINTF_APPEND("Rec=%s", QSPY_rec[l_record[1]].name);
                }
                else {
                    SNPRINTF_APPEND("Rec=USER+%3u",
                               (unsigned)(l_record[1] - l_userRec));
                }
                QSPY_printError();
                l_chksum = 0U;
                l_pos = l_record;
                l_esc = 0U;
            }
        }
    }
}

/*..........................................................................*/
void QSPY_resetAllDictionaries(void) {
    Dictionary_reset(&QSPY_funDict);
    Dictionary_reset(&QSPY_objDict);
    Dictionary_reset(&QSPY_usrDict);
    SigDictionary_reset(&QSPY_sigDict);

#ifdef QSPY_APP
    QSEQ_dictionaryReset();
    /* find out if NULL needs to be added to the Sequence dictionary... */
    QSEQ_updateDictionary("NULL", 0);
#endif

    /* pre-fill known user entries */
    Dictionary_put(&QSPY_usrDict, 124, "QUTEST_ON_POST");
}
/*..........................................................................*/
SigType QSPY_findSig(char const* name, ObjType obj) {
    return SigDictionary_findSig(&QSPY_sigDict, name, obj);
}
/*..........................................................................*/
KeyType QSPY_findObj(char const* name) {
    return Dictionary_findKey(&QSPY_objDict, name);
}
/*..........................................................................*/
KeyType QSPY_findFun(char const* name) {
    return Dictionary_findKey(&QSPY_funDict, name);
}
/*..........................................................................*/
KeyType QSPY_findUsr(char const* name) {
    return Dictionary_findKey(&QSPY_usrDict, name);
}
/*..........................................................................*/
KeyType QSPY_findEnum(char const *name, uint8_t group) {
    Q_ASSERT(group < sizeof(QSPY_enumDict)/sizeof(QSPY_enumDict[0]));
    return Dictionary_findKey(&QSPY_enumDict[group], name);
}

/* Dictionary class ========================================================*/
int Dictionary_comp(void const *arg1, void const *arg2) {
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
void Dictionary_ctor(Dictionary * const me,
                            DictEntry *sto, uint32_t capacity)
{
    me->sto      = sto;
    me->capacity = capacity;
    me->entries  = 0;
    me->keySize  = 4;
}
/*..........................................................................*/
 void Dictionary_config(Dictionary * const me, int keySize) {
    me->keySize = keySize;
}
/*..........................................................................*/
char const *Dictionary_at(Dictionary * const me, unsigned idx) {
    if (idx < (unsigned)me->entries) {
        return me->sto[idx].name;
    }
    else {
        return "";
    }
}
/*..........................................................................*/
void Dictionary_put(Dictionary * const me,
                           KeyType key, char const *name)
{
    int idx = Dictionary_find(me, key);
    int n = me->entries;
    char *dst;
    if (idx >= 0) { /* the key found? */
        Q_ASSERT((idx <= n) || (n == 0));
        dst = me->sto[idx].name;
        STRNCPY_S(dst, sizeof(me->sto[idx].name), name);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
    }
    else if (n < me->capacity - 1) {
        me->sto[n].key = key;
        dst = me->sto[n].name;
        STRNCPY_S(dst, sizeof(me->sto[n].name), name);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
        ++me->entries;
        /* keep the entries sorted by the key */
        qsort(me->sto, (uint32_t)me->entries, sizeof(me->sto[0]),
              &Dictionary_comp);
    }
}
/*..........................................................................*/
char const *Dictionary_get(Dictionary * const me, KeyType key, char *buf) {
    int idx;
    if ((key == 0U) && (me->keySize >= 4)) {
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
        if (me->keySize <= 1) {
            SNPRINTF_S(buf, QS_DNAME_LEN_MAX, "%03d", (unsigned)key);
        }
        else if (me->keySize <= 4) {
            SNPRINTF_S(buf, QS_DNAME_LEN_MAX, "0x%08X", (unsigned)key);
        }
        else {
            SNPRINTF_S(buf, QS_DNAME_LEN_MAX, "0x%016"PRIX64"", key);
        }
        //Dictionary_put(me, key, buf); /* put into the dictionary */
        return buf;
    }
}
/*..........................................................................*/
int Dictionary_find(Dictionary * const me, KeyType key) {
    /* binary search algorithm... */
    int first = 0;
    int last = me->entries;
    if (last > 0) { /* not empty? */
        while (first <= last) {
            int mid = (first + last) / 2;
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
    }
    return -1; /* entry not found */
}
/*..........................................................................*/
KeyType Dictionary_findKey(Dictionary * const me, char const *name) {
    /* brute-force search algorithm... */
    for (int i = 0; i < me->entries; ++i) {
        if (strncmp(me->sto[i].name, name, sizeof(me->sto[i].name)) == 0) {
            return me->sto[i].key;
        }
    }
    return KEY_NOT_FOUND;
}
/*..........................................................................*/
void Dictionary_reset(Dictionary * const me) {
    for (int i = 0; i < me->capacity; ++i) {
        me->sto[i].key = (KeyType)0;
    }
    me->entries = 0;
}

/* SigDictionary class =====================================================*/
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
void SigDictionary_ctor(SigDictionary * const me,
                        SigDictEntry *sto, uint32_t capacity)
{
    me->sto      = sto;
    me->capacity = capacity;
    me->entries  = 0;
    me->ptrSize  = 4;
}
/*..........................................................................*/
void SigDictionary_config(SigDictionary * const me, int ptrSize) {
    me->ptrSize = ptrSize;
}
/*..........................................................................*/
void SigDictionary_put(SigDictionary * const me,
                       SigType sig, ObjType obj, char const *name)
{
    int idx = SigDictionary_find(me, sig, obj);
    int n = me->entries;
    char *dst;
    if (idx >= 0) { /* the key found? */
        Q_ASSERT((idx <= n) || (n == 0));
        me->sto[idx].obj = obj;
        dst = me->sto[idx].name;
        STRNCPY_S(dst, sizeof(me->sto[idx].name), name);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
    }
    else if (n < me->capacity - 1) {
        me->sto[n].sig = sig;
        me->sto[n].obj = obj;
        dst = me->sto[n].name;
        STRNCPY_S(dst, sizeof(me->sto[n].name), name);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
        ++me->entries;
        /* keep the entries sorted by the sig */
        qsort(me->sto, (uint32_t)me->entries, sizeof(me->sto[0]),
              &SigDictionary_comp);
    }
}
/*..........................................................................*/
char const *SigDictionary_get(SigDictionary * const me,
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
            SNPRINTF_S(buf, QS_DNAME_LEN_MAX, "%08d,Obj=0x%08X",
                      (int)sig, (int)obj);
        }
        else {
            SNPRINTF_S(buf, QS_DNAME_LEN_MAX, "%08d,Obj=0x%016"PRIX64"",
                       (int)sig, obj);
        }
        //SigDictionary_put(me, sig, obj, buf); /* put into the dictionary */
        return buf;
    }
}
/*..........................................................................*/
int SigDictionary_find(SigDictionary * const me,
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
SigType SigDictionary_findSig(SigDictionary * const me,
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
void SigDictionary_reset(SigDictionary * const me) {
    int i;
    for (i = 0; i < me->capacity; ++i) {
        me->sto[i].sig = (SigType)0;
    }
    me->entries = 0;
}
