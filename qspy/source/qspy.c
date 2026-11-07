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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#define Q_SPY   1       // this is QS implementation
#define QP_IMPL 1       // this is QP implementation
typedef int      int_t;   // dummy definition for including "qpc_qs.h"
typedef int      enum_t;  // dummy definition for including "qpc_qs.h"
typedef uint16_t QSignal; // dummy definition for including "qpc_qs.h"
typedef uint32_t QSFun;   // dummy definition for including "qpc_qs.h"
typedef uint32_t QSObj;   // dummy definition for including "qpc_qs.h"
typedef uint32_t QEvt;    // dummy definition for including "qpc_qs.h"
typedef uint32_t QActive; // dummy definition for including "qpc_qs.h"
typedef uint32_t QPSet;   // dummy definition for including "qpc_qs.h"
#include "qpc_qs.h"       // QS target-resident interface
#include "qpc_qs_pkg.h"   // QS package-scope interface

#include "safe_std.h"   // "safe" <stdio.h> and <string.h> facilities
#include "qspy.h"       // QSPY data parser
#include "pal.h"        // Platform Abstraction Layer

// global objects ............................................................
QSPY_LastOutput QSPY_output;
char const * const QSPY_line = &QSPY_output.buf[QS_LINE_OFFSET];

QSpyConfig    QSPY_conf;
Dictionary    QSPY_funDict;
Dictionary    QSPY_objDict;
Dictionary    QSPY_usrDict;
SigDictionary QSPY_sigDict;
Dictionary    QSPY_enumDict[8];

//============================================================================
static DictEntry     l_funSto[8192];
static DictEntry     l_objSto[2048];
static DictEntry     l_usrSto[128 + 1 - QS_USER];
static SigDictEntry  l_sigSto[8192];
static DictEntry     l_enumSto[8][256];

//............................................................................
static FILE         *l_matFile;
static QSPY_CustParseFun l_custParseFun;
static QSPY_resetFun     l_txResetFun;

// descriptors of the pre-defined QS records ...............................
typedef struct QSpyRecDescTag QSpyRecDesc;

//! handler (decoder) of a pre-defined QS record
typedef void (*QSpyRecHandler)(QSpyRecord * const me,
                               QSpyRecDesc const * const desc);

struct QSpyRecDescTag {
    char const    *name;    //!< name of the record, e.g. "QS_QF_PUBLISH"
    int            group;   //!< group of the record (rendering/coloring)
    QSpyRecHandler handler; //!< decoder of the record (NULL: unknown)
    char const    *label;   //!< label for the output (NULL: none)
    char const    *aux;     //!< 2nd label, record-specific (NULL: none)
};

// decoders of the pre-defined records (defined below)
static void rec_empty(QSpyRecord * const me,
                      QSpyRecDesc const * const desc);
static void rec_qepState(QSpyRecord * const me,
                         QSpyRecDesc const * const desc);
static void rec_qepStateTran(QSpyRecord * const me,
                             QSpyRecDesc const * const desc);
static void rec_qepInitTran(QSpyRecord * const me,
                            QSpyRecDesc const * const desc);
static void rec_qepInternTran(QSpyRecord * const me,
                              QSpyRecDesc const * const desc);
static void rec_qepTran(QSpyRecord * const me,
                        QSpyRecDesc const * const desc);
static void rec_qepIgnored(QSpyRecord * const me,
                           QSpyRecDesc const * const desc);
static void rec_qepDispatch(QSpyRecord * const me,
                            QSpyRecDesc const * const desc);
static void rec_qepUnhandled(QSpyRecord * const me,
                             QSpyRecDesc const * const desc);
static void rec_qfActiveDefer(QSpyRecord * const me,
                              QSpyRecDesc const * const desc);
static void rec_qfActiveRecallAttempt(QSpyRecord * const me,
                                      QSpyRecDesc const * const desc);
static void rec_qfActiveSubscr(QSpyRecord * const me,
                               QSpyRecDesc const * const desc);
static void rec_qfActivePost(QSpyRecord * const me,
                             QSpyRecDesc const * const desc);
static void rec_qfActivePostLifo(QSpyRecord * const me,
                                 QSpyRecDesc const * const desc);
static void rec_qfGet(QSpyRecord * const me,
                      QSpyRecDesc const * const desc);
static void rec_qfGetLast(QSpyRecord * const me,
                          QSpyRecDesc const * const desc);
static void rec_qfEqueuePost(QSpyRecord * const me,
                             QSpyRecDesc const * const desc);
static void rec_qfMpoolGet(QSpyRecord * const me,
                           QSpyRecDesc const * const desc);
static void rec_qfMpoolPut(QSpyRecord * const me,
                           QSpyRecDesc const * const desc);
static void rec_qfNew(QSpyRecord * const me,
                      QSpyRecDesc const * const desc);
static void rec_qfPublish(QSpyRecord * const me,
                          QSpyRecDesc const * const desc);
static void rec_qfNewRef(QSpyRecord * const me,
                         QSpyRecDesc const * const desc);
static void rec_qfDeleteRef(QSpyRecord * const me,
                            QSpyRecDesc const * const desc);
static void rec_qfGc(QSpyRecord * const me,
                     QSpyRecDesc const * const desc);
static void rec_qfTick(QSpyRecord * const me,
                       QSpyRecDesc const * const desc);
static void rec_qfTimeEvtArm(QSpyRecord * const me,
                             QSpyRecDesc const * const desc);
static void rec_qfTimeEvtAutoDisarm(QSpyRecord * const me,
                                    QSpyRecDesc const * const desc);
static void rec_qfTimeEvtDisarmAttempt(QSpyRecord * const me,
                                       QSpyRecDesc const * const desc);
static void rec_qfTimeEvtRearm(QSpyRecord * const me,
                               QSpyRecDesc const * const desc);
static void rec_qfTimeEvtPost(QSpyRecord * const me,
                              QSpyRecDesc const * const desc);
static void rec_qfCrit(QSpyRecord * const me,
                       QSpyRecDesc const * const desc);
static void rec_qfIsr(QSpyRecord * const me,
                      QSpyRecDesc const * const desc);
static void rec_schedPreempt(QSpyRecord * const me,
                             QSpyRecDesc const * const desc);
static void rec_schedLock(QSpyRecord * const me,
                          QSpyRecDesc const * const desc);
static void rec_schedNext(QSpyRecord * const me,
                          QSpyRecDesc const * const desc);
static void rec_schedIdle(QSpyRecord * const me,
                          QSpyRecDesc const * const desc);
static void rec_enumDict(QSpyRecord * const me,
                         QSpyRecDesc const * const desc);
static void rec_testPaused(QSpyRecord * const me,
                           QSpyRecDesc const * const desc);
static void rec_testProbeGet(QSpyRecord * const me,
                             QSpyRecDesc const * const desc);
static void rec_sigDict(QSpyRecord * const me,
                        QSpyRecDesc const * const desc);
static void rec_objDict(QSpyRecord * const me,
                        QSpyRecDesc const * const desc);
static void rec_funDict(QSpyRecord * const me,
                        QSpyRecDesc const * const desc);
static void rec_usrDict(QSpyRecord * const me,
                        QSpyRecDesc const * const desc);
static void rec_targetInfo(QSpyRecord * const me,
                           QSpyRecDesc const * const desc);
static void rec_targetDone(QSpyRecord * const me,
                           QSpyRecDesc const * const desc);
static void rec_rxStatus(QSpyRecord * const me,
                         QSpyRecDesc const * const desc);
static void rec_queryData(QSpyRecord * const me,
                          QSpyRecDesc const * const desc);
static void rec_peekData(QSpyRecord * const me,
                         QSpyRecDesc const * const desc);
static void rec_assertFail(QSpyRecord * const me,
                           QSpyRecDesc const * const desc);
static void rec_qfRun(QSpyRecord * const me,
                      QSpyRecDesc const * const desc);
static void rec_sem(QSpyRecord * const me,
                    QSpyRecDesc const * const desc);
static void rec_mtxLock(QSpyRecord * const me,
                        QSpyRecDesc const * const desc);
static void rec_mtxBlock(QSpyRecord * const me,
                         QSpyRecDesc const * const desc);
static void rec_unknown(QSpyRecord * const me,
                        QSpyRecDesc const * const desc);

//! descriptors of all pre-defined QS records, indexed by the record ID
//! NOTE: keep in synch with qpc_qs.h; an entry without a handler is
//! decoded as an unknown record
#define NO_LBL ((char const *)0)
#define REC_(id_, grp_, fun_) \
    [id_] = { #id_, (grp_), &(fun_), NO_LBL, NO_LBL }
#define REC_L_(id_, grp_, fun_, lbl_) \
    [id_] = { #id_, (grp_), &(fun_), (lbl_), NO_LBL }
#define REC_LA_(id_, grp_, fun_, lbl_, aux_) \
    [id_] = { #id_, (grp_), &(fun_), (lbl_), (aux_) }
#define REC_NO_DECODER_(id_, grp_) \
    [id_] = { #id_, (grp_), (QSpyRecHandler)0, NO_LBL, NO_LBL }
#define REC_RESERVED_(id_) \
    [id_] = { "QS_RESERVED", QSPY_GRP_ERR, (QSpyRecHandler)0, NO_LBL, NO_LBL }

static QSpyRecDesc const l_recDesc[QS_USER] = {
    REC_(QS_EMPTY, QSPY_GRP_INF, rec_empty),

    // [1] QEP records
    REC_L_(QS_QEP_STATE_ENTRY, QS_GRP_SM, rec_qepState, "St-Entry"),
    REC_L_(QS_QEP_STATE_EXIT, QS_GRP_SM, rec_qepState, "St-Exit "),
    REC_L_(QS_QEP_STATE_INIT, QS_GRP_SM, rec_qepStateTran, "St-Init "),
    REC_(QS_QEP_INIT_TRAN, QS_GRP_SM, rec_qepInitTran),
    REC_(QS_QEP_INTERN_TRAN, QS_GRP_SM, rec_qepInternTran),
    REC_(QS_QEP_TRAN, QS_GRP_SM, rec_qepTran),
    REC_(QS_QEP_IGNORED, QS_GRP_SM, rec_qepIgnored),
    REC_(QS_QEP_DISPATCH, QS_GRP_SM, rec_qepDispatch),
    REC_(QS_QEP_UNHANDLED, QS_GRP_SM, rec_qepUnhandled),

    // [10] QF (AP) records
    REC_L_(QS_QF_ACTIVE_DEFER, QS_GRP_AO, rec_qfActiveDefer, "Defer"),
    REC_L_(QS_QF_ACTIVE_RECALL, QS_GRP_AO, rec_qfActiveDefer, "RCall"),
    REC_L_(QS_QF_ACTIVE_SUBSCRIBE, QS_GRP_AO, rec_qfActiveSubscr, "Subsc"),
    REC_L_(QS_QF_ACTIVE_UNSUBSCRIBE, QS_GRP_AO, rec_qfActiveSubscr, "Unsub"),
    REC_L_(QS_QF_ACTIVE_POST, QS_GRP_AO, rec_qfActivePost, "Post "),
    REC_(QS_QF_ACTIVE_POST_LIFO, QS_GRP_AO, rec_qfActivePostLifo),
    REC_L_(QS_QF_ACTIVE_GET, QS_GRP_AO, rec_qfGet, "AO-Get  "),
    REC_L_(QS_QF_ACTIVE_GET_LAST, QS_GRP_AO, rec_qfGetLast, "AO-GetL "),
    REC_(QS_QF_ACTIVE_RECALL_ATTEMPT, QS_GRP_AO, rec_qfActiveRecallAttempt),

    // [19] QF (EQ) records
    REC_LA_(QS_QF_EQUEUE_POST, QS_GRP_EQ, rec_qfEqueuePost, "Post ", "Min"),
    REC_LA_(QS_QF_EQUEUE_POST_LIFO, QS_GRP_EQ, rec_qfEqueuePost, "LIFO", "Min"),
    REC_L_(QS_QF_EQUEUE_GET, QS_GRP_EQ, rec_qfGet, "EQ-Get  "),
    REC_L_(QS_QF_EQUEUE_GET_LAST, QS_GRP_EQ, rec_qfGetLast, "EQ-GetL "),

    // [23] QF records
    REC_L_(QS_QF_NEW_ATTEMPT, QS_GRP_QF, rec_qfNew, "QF-NewA "),

    // [24] Memory Pool (MP) records
    REC_LA_(QS_QF_MPOOL_GET, QS_GRP_MP, rec_qfMpoolGet, "Get  ", "Min"),
    REC_(QS_QF_MPOOL_PUT, QS_GRP_MP, rec_qfMpoolPut),

    // [26] Additional Framework (QF) records
    REC_(QS_QF_PUBLISH, QS_GRP_QF, rec_qfPublish),
    REC_(QS_QF_NEW_REF, QS_GRP_QF, rec_qfNewRef),
    REC_L_(QS_QF_NEW, QS_GRP_QF, rec_qfNew, "QF-New  "),
    REC_L_(QS_QF_GC_ATTEMPT, QS_GRP_QF, rec_qfGc, "QF-gcA  "),
    REC_L_(QS_QF_GC, QS_GRP_QF, rec_qfGc, "QF-gc   "),
    REC_(QS_QF_TICK, QS_GRP_QF, rec_qfTick),

    // [32] Time Event (TE) records
    REC_L_(QS_QF_TIMEEVT_ARM, QS_GRP_TE, rec_qfTimeEvtArm, "Arm "),
    REC_(QS_QF_TIMEEVT_AUTO_DISARM, QS_GRP_TE, rec_qfTimeEvtAutoDisarm),
    REC_(QS_QF_TIMEEVT_DISARM_ATTEMPT, QS_GRP_TE, rec_qfTimeEvtDisarmAttempt),
    REC_L_(QS_QF_TIMEEVT_DISARM, QS_GRP_TE, rec_qfTimeEvtArm, "Dis "),
    REC_(QS_QF_TIMEEVT_REARM, QS_GRP_TE, rec_qfTimeEvtRearm),
    REC_(QS_QF_TIMEEVT_POST, QS_GRP_TE, rec_qfTimeEvtPost),

    // [38] Additional Framework (QF) records
    REC_(QS_QF_DELETE_REF, QS_GRP_QF, rec_qfDeleteRef),
    REC_L_(QS_QF_CRIT_ENTRY, QS_GRP_QF, rec_qfCrit, "QF-CritE"),
    REC_L_(QS_QF_CRIT_EXIT, QS_GRP_QF, rec_qfCrit, "QF-CritX"),
    REC_L_(QS_QF_ISR_ENTRY, QS_GRP_QF, rec_qfIsr, "QF-IsrE"),
    REC_L_(QS_QF_ISR_EXIT, QS_GRP_QF, rec_qfIsr, "QF-IsrX"),
    REC_NO_DECODER_(QS_QF_INT_DISABLE, QS_GRP_QF),
    REC_NO_DECODER_(QS_QF_INT_ENABLE, QS_GRP_QF),

    // [45] Additional Active Object (AO) records
    REC_L_(QS_QF_ACTIVE_POST_ATTEMPT, QS_GRP_AO, rec_qfActivePost, "PostA"),

    // [46] Additional Event Queue (EQ) records
    REC_LA_(QS_QF_EQUEUE_POST_ATTEMPT, QS_GRP_EQ,
            rec_qfEqueuePost, "PostA", "Mar"),

    // [47] Additional Memory Pool (MP) records
    REC_LA_(QS_QF_MPOOL_GET_ATTEMPT, QS_GRP_MP, rec_qfMpoolGet, "GetA ", "Mar"),

    // [48] Scheduler (SC) records
    REC_LA_(QS_SCHED_PREEMPT, QS_GRP_SC,
            rec_schedPreempt, "Sch-Pre ", "Mtx-Lock"),
    REC_LA_(QS_SCHED_RESTORE, QS_GRP_SC,
            rec_schedPreempt, "Sch-Rest", "Mtx-Unlk"),
    REC_L_(QS_SCHED_LOCK, QS_GRP_SC, rec_schedLock, "Sch-Lock"),
    REC_L_(QS_SCHED_UNLOCK, QS_GRP_SC, rec_schedLock, "Sch-Unlk"),
    REC_(QS_SCHED_NEXT, QS_GRP_SC, rec_schedNext),
    REC_(QS_SCHED_IDLE, QS_GRP_SC, rec_schedIdle),
    REC_(QS_ENUM_DICT, QSPY_GRP_DIC, rec_enumDict),

    // [55] Additional QEP records
    REC_L_(QS_QEP_TRAN_HIST, QS_GRP_SM, rec_qepStateTran, "St-Hist "),
    [QS_RESERVED_56] = { "QS_QEP_TRAN_EP", QS_GRP_SM,
        &rec_qepStateTran, "St-EP   ", NO_LBL },
    [QS_RESERVED_57] = { "QS_QEP_TRAN_XP", QS_GRP_SM,
        &rec_qepStateTran, "St-XP   ", NO_LBL },

    // [58] Miscellaneous QS records (not maskable)
    REC_(QS_TEST_PAUSED, QSPY_GRP_TST, rec_testPaused),
    REC_(QS_TEST_PROBE_GET, QSPY_GRP_TST, rec_testProbeGet),
    REC_(QS_SIG_DICT, QSPY_GRP_DIC, rec_sigDict),
    REC_(QS_OBJ_DICT, QSPY_GRP_DIC, rec_objDict),
    REC_(QS_FUN_DICT, QSPY_GRP_DIC, rec_funDict),
    REC_(QS_USR_DICT, QSPY_GRP_DIC, rec_usrDict),
    REC_(QS_TARGET_INFO, QSPY_GRP_INF, rec_targetInfo),
    REC_(QS_TARGET_DONE, QSPY_GRP_TST, rec_targetDone),
    REC_(QS_RX_STATUS, QSPY_GRP_TST, rec_rxStatus),
    REC_(QS_QUERY_DATA, QSPY_GRP_TST, rec_queryData),
    REC_(QS_PEEK_DATA, QSPY_GRP_TST, rec_peekData),
    REC_(QS_ASSERT_FAIL, QSPY_GRP_ERR, rec_assertFail),
    REC_(QS_QF_RUN, QSPY_GRP_INF, rec_qfRun),

    // [71] Semaphore (SEM) records
    REC_L_(QS_SEM_TAKE, QS_GRP_SEM, rec_sem, "Sem-Take"),
    REC_L_(QS_SEM_BLOCK, QS_GRP_SEM, rec_sem, "Sem-Blk "),
    REC_L_(QS_SEM_SIGNAL, QS_GRP_SEM, rec_sem, "Sem-Sgnl"),
    REC_L_(QS_SEM_BLOCK_ATTEMPT, QS_GRP_SEM, rec_sem, "Sem-BlkA"),

    // [75] Mutex (MTX) records
    REC_L_(QS_MTX_LOCK, QS_GRP_MTX, rec_mtxLock, "Mtx-Lock"),
    REC_L_(QS_MTX_BLOCK, QS_GRP_MTX, rec_mtxBlock, "Mtx-Blk "),
    REC_L_(QS_MTX_UNLOCK, QS_GRP_MTX, rec_mtxLock, "Mtx-Unlk"),
    REC_L_(QS_MTX_LOCK_ATTEMPT, QS_GRP_MTX, rec_mtxLock, "Mtx-LckA"),
    REC_L_(QS_MTX_BLOCK_ATTEMPT, QS_GRP_MTX, rec_mtxBlock, "Mtx-BlkA"),
    REC_L_(QS_MTX_UNLOCK_ATTEMPT, QS_GRP_MTX, rec_mtxLock, "Mtx-UlkA"),

    // [81] Additional QF (AO) records
    REC_L_(QS_QF_ACTIVE_DEFER_ATTEMPT, QS_GRP_AO, rec_qfActiveDefer, "DefrA"),

    // [82] Additional QF (AO) records
    REC_RESERVED_(82),
    REC_RESERVED_(83),
    REC_RESERVED_(84),
    REC_RESERVED_(85),
    REC_RESERVED_(86),
    REC_RESERVED_(87),
    REC_RESERVED_(88),
    REC_RESERVED_(89),
    REC_RESERVED_(90),
    REC_RESERVED_(91),
    REC_RESERVED_(92),
    REC_RESERVED_(93),
    REC_RESERVED_(94),
    REC_RESERVED_(95),
    REC_RESERVED_(96),
    REC_RESERVED_(97),
    REC_RESERVED_(98),
    REC_RESERVED_(99),
};
// compile-time checks of the table: the reserved entries above start at
// QS_PRE_MAX (a new record ID in qpc_qs.h must also be added to the table,
// otherwise its REC_RESERVED_() entry would silently override it)
typedef char l_recDescCheck1[(QS_PRE_MAX == 82) ? 1 : -1];
typedef char l_recDescCheck2[(QS_PRE_MAX <= QS_USER) ? 1 : -1];

// QS object kinds... NOTE: keep in synch with QS_ObjKind in qpc_qs.h
static char const *  l_qs_obj[] = {
    "SM",
    "AO",
    "MP",
    "EQ",
    "TE",
    "AP",
    "SM_AO",
    "EP",
};

// QS-RX record names... NOTE: keep in synch with QS_RxRecords in qpc_qs_pkg.h
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

// facilities for QSPY host application only (but not for QSPY parser)
#ifdef QSPY_APP

#define FPRINF_MATFILE(format_, ...)                \
    if (l_matFile != (FILE *)0) {                   \
        FPRINTF_S(l_matFile, format_, __VA_ARGS__); \
    } else (void)0

#else

#define FPRINF_MATFILE(format_, ...)   ((void)0)

#endif // QSPY_APP

//============================================================================
void QSPY_config(QSpyConfig const *config,
                 QSPY_CustParseFun custParseFun)
{
    QSPY_conf = *config; // copy over

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

    QSPY_conf.qpDate = 0U; // invalidate the date to indicate "no-target-info"
}
//............................................................................
// checks that all data sizes in the configuration can be decoded;
// returns NULL when OK or a description of the first invalid size
char const *QSPY_checkConfig(QSpyConfig const * const cfg) {
    static char desc[48];
    struct { uint8_t val; uint8_t maxSize; char const *name; } const chk[] = {
        { cfg->objPtrSize,   8U, "object-pointer size"   },
        { cfg->funPtrSize,   8U, "function-pointer size" },
        { cfg->tstampSize,   4U, "timestamp size"        },
        { cfg->sigSize,      4U, "signal size"           },
        { cfg->evtSize,      4U, "event-size size"       },
        { cfg->queueCtrSize, 4U, "queue-counter size"    },
        { cfg->poolCtrSize,  4U, "pool-counter size"     },
        { cfg->poolBlkSize,  4U, "pool-block-size size"  },
        { cfg->tevtCtrSize,  4U, "time-event-counter size" },
    };
    for (unsigned i = 0U; i < sizeof(chk)/sizeof(chk[0]); ++i) {
        uint8_t v = chk[i].val;
        if (((v != 1U) && (v != 2U) && (v != 4U) && (v != 8U))
            || (v > chk[i].maxSize))
        {
            (void)SNPRINTF_S(desc, sizeof(desc), "%s=%u",
                             chk[i].name, (unsigned)v);
            return desc;
        }
    }
    return (char const *)0;
}
//............................................................................
// configure the key sizes of the dictionaries from the current QSPY_conf
void QSPY_configDictionaries(void) {
    Dictionary_config(&QSPY_funDict, QSPY_conf.funPtrSize);
    Dictionary_config(&QSPY_objDict, QSPY_conf.objPtrSize);
    SigDictionary_config(&QSPY_sigDict, QSPY_conf.objPtrSize);
}
//............................................................................
void QSPY_configTxReset(QSPY_resetFun txResetFun) {
    l_txResetFun = txResetFun;
}
//............................................................................
void QSPY_configMatFile(void *matFile) {
    // NOTE: the file is opened/closed by the caller (see qspy_main.c)
    l_matFile = (FILE *)matFile;
}

//............................................................................
void QSpyRecord_init(QSpyRecord * const me,
                     uint8_t const *start, uint32_t tot_len)
{
    me->start   = start;
    me->tot_len = (uint32_t)tot_len;
    me->len     = (int32_t)(tot_len - 3U);
    me->pos     = start + 2;
    me->rec     = start[1];
    me->err     = (char const *)0;

    // set the current QS record-ID for any subsequent output
    QSPY_output.rec  = me->rec;
    QSPY_output.rx_status = -1;
}
//............................................................................
// name of the record for error messages
static char const *QSpyRecord_name(uint8_t rec, char *buf, size_t size) {
    if (rec < QS_USER) { // pre-defined QS record?
        return l_recDesc[rec].name;
    }
    (void)SNPRINTF_S(buf, size, "USER+%03d", (int)(rec - QS_USER));
    return buf;
}
//............................................................................
// NOTE: the QSpyRecord_get*() functions report NO errors by themselves.
// The first error (data missing or invalid data size) is recorded in the
// record ('len' < 0, 'err' describes the missing data item) and all
// subsequent get operations on the record return 0 (sticky error).
// The error is reported ONCE by QSpyRecord_OK(), which MUST be called
// before any output generated from the record.
QSpyStatus QSpyRecord_OK(QSpyRecord * const me) {
    if (me->len != 0) {
        char rbuf[16];
        char const *rname = QSpyRecord_name(me->rec, rbuf, sizeof(rbuf));
        if (me->len > 0) {
            SNPRINTF_LINE("   <COMMS> ERROR    %d bytes unused in Rec=%s",
                          (int)me->len, rname);
        }
        else if (me->len == QS_REC_BAD_DATA) {
            SNPRINTF_LINE("   <COMMS> ERROR    invalid %s in Rec=%s",
                          me->err, rname);
        }
        else if (me->len == QS_REC_BAD_SIZE) {
            SNPRINTF_LINE("   <COMMS> ERROR    invalid size of %s in Rec=%s",
                          me->err, rname);
        }
        else {
            SNPRINTF_LINE("   <COMMS> ERROR    %d bytes needed for %s "
                          "in Rec=%s", -(int)me->len, me->err, rname);
        }
        QSPY_printError();
        return QSPY_ERROR;
    }
    return QSPY_SUCCESS;
}
//............................................................................
// record the first error in the record (sticky)
static void QSpyRecord_fail(QSpyRecord * const me,
                            int32_t len, char const *what)
{
    if (me->len >= 0) { // no error recorded yet?
        me->len = len;
        me->err = what;
    }
}
//............................................................................
// take 'size' bytes (1, 2, 4, or 8) little-endian from the record
static bool QSpyRecord_take(QSpyRecord * const me, uint8_t size,
                            char const *what, uint64_t *pVal)
{
    *pVal = 0U;
    if (me->len < 0) { // error already recorded?
        return false;
    }
    if ((size != 1U) && (size != 2U) && (size != 4U) && (size != 8U)) {
        QSpyRecord_fail(me, QS_REC_BAD_SIZE, what);
        return false;
    }
    if (me->len < (int32_t)size) {
        QSpyRecord_fail(me, -((int32_t)size - me->len), what);
        return false;
    }
    uint64_t ret = 0U;
    for (int i = (int)size - 1; i >= 0; --i) {
        ret = (ret << 8) | (uint64_t)me->pos[i];
    }
    me->pos += size;
    me->len -= (int32_t)size;
    *pVal = ret;
    return true;
}
//............................................................................
uint32_t QSpyRecord_getUint32(QSpyRecord * const me, uint8_t size) {
    uint64_t ret;
    if (size > 4U) {
        QSpyRecord_fail(me, QS_REC_BAD_SIZE, "uint32_t");
        return 0U;
    }
    (void)QSpyRecord_take(me, size, "uint32_t", &ret);
    return (uint32_t)ret;
}
//............................................................................
int32_t QSpyRecord_getInt32(QSpyRecord * const me, uint8_t size) {
    uint64_t ret;
    if (size > 4U) {
        QSpyRecord_fail(me, QS_REC_BAD_SIZE, "int32_t");
        return 0;
    }
    if (!QSpyRecord_take(me, size, "int32_t", &ret)) {
        return 0;
    }
    if (size < 4U) { // sign-extend
        uint64_t const sign = (uint64_t)1U << (size*8U - 1U);
        ret = (ret ^ sign) - sign;
    }
    return (int32_t)(uint32_t)ret;
}
//............................................................................
uint64_t QSpyRecord_getUint64(QSpyRecord * const me, uint8_t size) {
    uint64_t ret;
    (void)QSpyRecord_take(me, size, "uint64_t", &ret);
    return ret;
}
//............................................................................
int64_t QSpyRecord_getInt64(QSpyRecord * const me, uint8_t size) {
    uint64_t ret;
    if (!QSpyRecord_take(me, size, "int64_t", &ret)) {
        return 0;
    }
    if (size < 8U) { // sign-extend
        uint64_t const sign = (uint64_t)1U << (size*8U - 1U);
        ret = (ret ^ sign) - sign;
    }
    return (int64_t)ret;
}
//............................................................................
char const *QSpyRecord_getStr(QSpyRecord * const me) {
    if (me->len < 0) { // error already recorded?
        return "";
    }
    if (me->len == 0) { // no data at all?
        QSpyRecord_fail(me, -1, "string");
        return "";
    }

    // is the string empty?
    if (*me->pos == 0U) {
        // adjust the stream for the next token
        --me->len;
        ++me->pos;

        // return explicit empty string as two single-quotes ''
        return "''";
    }

    uint8_t const *p = me->pos;
    for (int32_t l = me->len; l > 0; --l, ++p) {
        if (*p == 0U) { // zero-terminated end of the string?
            char const *s = (char const *)me->pos;

            // adjust the stream for the next token
            me->len = l - 1;
            me->pos = p + 1;
            return s; // normal return
        }
    }

    // zero-terminator missing within the record
    QSpyRecord_fail(me, -1, "string");
    return "";
}
//............................................................................
uint8_t const *QSpyRecord_getMem(QSpyRecord * const me,
                                 uint8_t size,
                                 uint32_t *pNum)
{
    *pNum = 0U;
    if (me->len < 0) { // error already recorded?
        return (uint8_t const *)0;
    }
    if (me->len < 1) { // the count byte missing?
        QSpyRecord_fail(me, -1, "memory-dump");
        return (uint8_t const *)0;
    }
    uint32_t const num = (uint32_t)me->pos[0];
    int32_t const nBytes = 1 + (int32_t)(num * size);
    if (me->len < nBytes) {
        QSpyRecord_fail(me, -(nBytes - me->len), "memory-dump");
        return (uint8_t const *)0;
    }
    uint8_t const *mem = me->pos + 1;
    *pNum = num;
    me->len -= nBytes;
    me->pos += nBytes;
    return mem;
}

//============================================================================
// check pass over an application-specific (user) QS record: consumes all
// data elements without generating any output, so that a malformed record
// is detected (by QSpyRecord_OK()) BEFORE any output is produced.
static void QSpyRecord_skipUser(QSpyRecord * const me) {
    uint32_t num;
    (void)QSpyRecord_getUint64(me, QSPY_conf.tstampSize);
    while (me->len > 0) {
        uint32_t fmt = QSpyRecord_getUint32(me, 1) & 0x0FU;
        switch (fmt) {
            case QS_I8_ENUM_FMT: (void)QSpyRecord_getInt32(me, 1); break;
            case QS_U8_FMT:  (void)QSpyRecord_getUint32(me, 1); break;
            case QS_I16_FMT: (void)QSpyRecord_getInt32(me, 2); break;
            case QS_U16_FMT: (void)QSpyRecord_getUint32(me, 2); break;
            case QS_I32_FMT: (void)QSpyRecord_getInt32(me, 4); break;
            case QS_U32_FMT: // intentionally fall through
            case QS_F32_FMT: // intentionally fall through
            case QS_HEX_FMT: (void)QSpyRecord_getUint32(me, 4); break;
            case QS_I64_FMT: (void)QSpyRecord_getInt64(me, 8); break;
            case QS_U64_FMT: // intentionally fall through
            case QS_F64_FMT: (void)QSpyRecord_getUint64(me, 8); break;
            case QS_STR_FMT: (void)QSpyRecord_getStr(me); break;
            case QS_MEM_FMT: (void)QSpyRecord_getMem(me, 1, &num); break;
            case QS_SIG_FMT:
                (void)QSpyRecord_getUint32(me, QSPY_conf.sigSize);
                (void)QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                break;
            case QS_OBJ_FMT:
                (void)QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                break;
            case QS_FUN_FMT:
                (void)QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
                break;
            default: // all 16 formats handled above; defensive only
                QSpyRecord_fail(me, QS_REC_BAD_DATA, "data format");
                break;
        }
    }
}
//============================================================================
// application-specific (user) QS records...
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

    // check the whole record first (no output for a malformed record)
    {
        QSpyRecord chk = *me;
        QSpyRecord_skipUser(&chk);
        if (QSpyRecord_OK(&chk) != QSPY_SUCCESS) {
            return; // error already reported
        }
    }

    u32 = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    i32 = Dictionary_find(&QSPY_usrDict, me->rec);
    if (i32 >= 0) {
        SNPRINTF_LINE("%010u %s", u32, Dictionary_at(&QSPY_usrDict, i32));
    }
    else {
        SNPRINTF_LINE("%010u USER+%03d", u32, (int)(me->rec - QS_USER));
    }

    FPRINF_MATFILE("%d %u", (int)me->rec, u32);

    while (me->len > 0) {
        // get the format byte
        uint32_t fmt = QSpyRecord_getUint32(me, 1);
        uint32_t width = (fmt >> 4U) & 0x0FU;
        bool is_hex = (width == (uint32_t)QS_HEX_FMT);
        fmt &= 0x0FU;

        SNPRINTF_APPEND("%c", ' ');
        FPRINF_MATFILE("%c", ' ');

        char const *s;
        switch (fmt) {
            case QS_I8_ENUM_FMT: {
                if ((width & 0x8U) == 0U) { // QS_I8() data element
                    i32 = QSpyRecord_getInt32(me, 1);
                    SNPRINTF_APPEND(ifmt[width], (long)i32);
                    FPRINF_MATFILE(ifmt[width], (long)i32);
                }
                else { // QS_ENUM() data element
                    u32 = QSpyRecord_getUint32(me, 1);
                    SNPRINTF_APPEND("%s",
                        Dictionary_get(&QSPY_enumDict[width & 0x7U],
                                       u32, (char *)0));
                    FPRINF_MATFILE(ufmt[1], (unsigned long)u32);
                }
                break;
            }
            case QS_U8_FMT: {
                u32 = QSpyRecord_getUint32(me, 1);
                SNPRINTF_APPEND(is_hex ? uhfmt[2] : ufmt[width],
                                (unsigned long)u32);
                FPRINF_MATFILE(ufmt[width], (unsigned long)u32);
                break;
            }
            case QS_I16_FMT: {
                i32 = QSpyRecord_getInt32(me, 2);
                SNPRINTF_APPEND(ifmt[width], (long)i32);
                FPRINF_MATFILE(ifmt[width], (long)i32);
                break;
            }
            case QS_U16_FMT: {
                u32 = QSpyRecord_getUint32(me, 2);
                SNPRINTF_APPEND(is_hex ? uhfmt[4] : ufmt[width],
                                (unsigned long)u32);
                FPRINF_MATFILE(ufmt[width], (unsigned long)u32);
                break;
            }
            case QS_I32_FMT: {
                i32 = QSpyRecord_getInt32(me, 4);
                SNPRINTF_APPEND(ifmt[width], (long)i32);
                FPRINF_MATFILE(ifmt[width], (long)i32);
                break;
            }
            case QS_U32_FMT: {
                u32 = QSpyRecord_getUint32(me, 4);
                SNPRINTF_APPEND(is_hex ? uhfmt[8] : ufmt[width],
                                (unsigned long)u32);
                FPRINF_MATFILE(ufmt[width], (unsigned long)u32);
                break;
            }
            case QS_F32_FMT: {
                union {
                   uint32_t u;
                   float    f;
                } x;
                x.u = QSpyRecord_getUint32(me, 4);
                SNPRINTF_APPEND(efmt[width], (double)x.f);
                FPRINF_MATFILE(efmt[width], (double)x.f);
                break;
            }
            case QS_F64_FMT: {
                union F64Rep {
                    uint64_t u;
                    double   d;
                } data;
                data.u = QSpyRecord_getUint64(me, 8);
                SNPRINTF_APPEND(efmt[width], data.d);
                FPRINF_MATFILE(efmt[width], data.d);
                break;
            }
            case QS_STR_FMT: {
                s = QSpyRecord_getStr(me);
                SNPRINTF_APPEND("%s", s);
                FPRINF_MATFILE("%s", s);
                break;
            }
            case QS_MEM_FMT: {
                uint8_t const *mem = QSpyRecord_getMem(me, 1, &u32);
                if (mem) {
                    for (; u32 > 0U; --u32, ++mem) {
                        SNPRINTF_APPEND(" %02X", (unsigned int)*mem);
                        FPRINF_MATFILE(" %03d", (unsigned int)*mem);
                    }
                }
                break;
            }
            case QS_SIG_FMT: {
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
            case QS_OBJ_FMT: {
                u64 = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
                SNPRINTF_APPEND("%s",
                    Dictionary_get(&QSPY_objDict, u64, (char *)0));
                FPRINF_MATFILE("%"PRId64, u64);
                break;
            }
            case QS_FUN_FMT: {
                u64 = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
                SNPRINTF_APPEND("%s",
                    Dictionary_get(&QSPY_funDict, u64, (char *)0));
                FPRINF_MATFILE("%"PRId64, u64);
                break;
            }
            case QS_I64_FMT: {
                i64 = QSpyRecord_getInt64(me, 8);
                SNPRINTF_APPEND(ilfmt[width], i64);
                FPRINF_MATFILE(ilfmt[width], i64);
                break;
            }
            case QS_U64_FMT: {
                u64 = QSpyRecord_getUint64(me, 8);
                SNPRINTF_APPEND(is_hex ? "0x%016"PRIX64 : ulfmt[width], u64);
                FPRINF_MATFILE(ulfmt[width], u64);
                break;
            }
            case QS_HEX_FMT: {
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

//============================================================================
// predefined QS records...
static void QSpyRecord_processPre(QSpyRecord * const me) {
    // NOTE: the Application-Specific records have been already
    // sifted out in QSPY_rxByte()
    if ((me->rec < QS_USER)
        && (l_recDesc[me->rec].handler != (QSpyRecHandler)0))
    {
        (*l_recDesc[me->rec].handler)(me, &l_recDesc[me->rec]);
    }
    else {
        rec_unknown(me, (QSpyRecDesc const *)0);
    }
}
//............................................................................
// Session start
// QS_EMPTY
static void rec_empty(QSpyRecord * const me,
                      QSpyRecDesc const * const desc)
{
    (void)desc; // unused
    (void)me; // unused
    // silently ignore
}
//............................................................................
// QEP records
// QS_QEP_STATE_ENTRY, QS_QEP_STATE_EXIT
static void rec_qepState(QSpyRecord * const me,
                         QSpyRecDesc const * const desc)
{
    uint64_t p, q;
    char const *s = desc->label;

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
}
//............................................................................
// QS_QEP_STATE_INIT, QS_QEP_TRAN_HIST, QS_RESERVED_56, QS_RESERVED_57
static void rec_qepStateTran(QSpyRecord * const me,
                             QSpyRecDesc const * const desc)
{
    uint64_t p, q, r;
    char buf[QS_FNAME_LEN_MAX];
    char const *s = desc->label;

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
}
//............................................................................
// QS_QEP_INIT_TRAN
static void rec_qepInitTran(QSpyRecord * const me,
                            QSpyRecDesc const * const desc)
{
    uint32_t t;
    uint64_t p, q;

    (void)desc; // unused
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
}
//............................................................................
// QS_QEP_INTERN_TRAN
static void rec_qepInternTran(QSpyRecord * const me,
                              QSpyRecDesc const * const desc)
{
    uint32_t t, a;
    uint64_t p, q;

    (void)desc; // unused
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
}
//............................................................................
// QS_QEP_TRAN
static void rec_qepTran(QSpyRecord * const me,
                        QSpyRecDesc const * const desc)
{
    uint32_t t, a;
    uint64_t p, q, r;
    char buf[QS_FNAME_LEN_MAX];
    char const *w = (char const *)0;

    (void)desc; // unused
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
}
//............................................................................
// QS_QEP_IGNORED
static void rec_qepIgnored(QSpyRecord * const me,
                           QSpyRecDesc const * const desc)
{
    uint32_t t, a;
    uint64_t p, q;

    (void)desc; // unused
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
}
//............................................................................
// QS_QEP_DISPATCH
static void rec_qepDispatch(QSpyRecord * const me,
                            QSpyRecDesc const * const desc)
{
    uint32_t t, a;
    uint64_t p, q;

    (void)desc; // unused
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
}
//............................................................................
// QS_QEP_UNHANDLED
static void rec_qepUnhandled(QSpyRecord * const me,
                             QSpyRecDesc const * const desc)
{
    uint32_t a;
    uint64_t p, q;

    (void)desc; // unused
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
}
//............................................................................
// QF records
// QS_QF_ACTIVE_DEFER, QS_QF_ACTIVE_DEFER_ATTEMPT, QS_QF_ACTIVE_RECALL
static void rec_qfActiveDefer(QSpyRecord * const me,
                              QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c;
    uint64_t p, q;
    char buf[QS_FNAME_LEN_MAX];
    char const *s = desc->label;

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
                Dictionary_get(&QSPY_objDict, q, buf),
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
//............................................................................
// QS_QF_ACTIVE_RECALL_ATTEMPT
static void rec_qfActiveRecallAttempt(QSpyRecord * const me,
                                      QSpyRecDesc const * const desc)
{
    uint32_t t;
    uint64_t p, q;
    char buf[QS_FNAME_LEN_MAX];

    (void)desc; // unused
    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    if (QSpyRecord_OK(me)) {
        SNPRINTF_LINE("%010u AO-RCllA Obj=%s,Que=%s",
                t,
                Dictionary_get(&QSPY_objDict, p, (char *)0),
                Dictionary_get(&QSPY_objDict, q, buf));
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
//............................................................................
// QS_QF_ACTIVE_SUBSCRIBE, QS_QF_ACTIVE_UNSUBSCRIBE
static void rec_qfActiveSubscr(QSpyRecord * const me,
                               QSpyRecDesc const * const desc)
{
    uint32_t t, a;
    uint64_t p;
    char const *s = desc->label;

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
}
//............................................................................
// QS_QF_ACTIVE_POST, QS_QF_ACTIVE_POST_ATTEMPT
static void rec_qfActivePost(QSpyRecord * const me,
                             QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c, d, e;
    uint64_t p, q;
    char buf[QS_FNAME_LEN_MAX];
    char const *s = desc->label;
    char const *w = (char const *)0;

    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    b = QSpyRecord_getUint32(me, 1);
    c = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_ACTIVE_POST_LIFO
static void rec_qfActivePostLifo(QSpyRecord * const me,
                                 QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c, d, e;
    uint64_t p;
    char const *w = (char const *)0;

    (void)desc; // unused
    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    b = QSpyRecord_getUint32(me, 1);
    c = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_ACTIVE_GET, QS_QF_EQUEUE_GET
static void rec_qfGet(QSpyRecord * const me,
                      QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c, d;
    uint64_t p;
    char const *s = desc->label;

    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    b = QSpyRecord_getUint32(me, 1);
    c = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_ACTIVE_GET_LAST, QS_QF_EQUEUE_GET_LAST
static void rec_qfGetLast(QSpyRecord * const me,
                          QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c;
    uint64_t p;
    char const *s = desc->label;

    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    b = QSpyRecord_getUint32(me, 1);
    c = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_EQUEUE_POST, QS_QF_EQUEUE_POST_ATTEMPT, QS_QF_EQUEUE_POST_LIFO
static void rec_qfEqueuePost(QSpyRecord * const me,
                             QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c, d, e;
    uint64_t p;
    char const *s = desc->label;
    char const *w = desc->aux;

    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    b = QSpyRecord_getUint32(me, 1);
    c = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_MPOOL_GET, QS_QF_MPOOL_GET_ATTEMPT
static void rec_qfMpoolGet(QSpyRecord * const me,
                           QSpyRecDesc const * const desc)
{
    uint32_t t, b, c;
    uint64_t p;
    char const *s = desc->label;
    char const *w = desc->aux;

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
}
//............................................................................
// QS_QF_MPOOL_PUT
static void rec_qfMpoolPut(QSpyRecord * const me,
                           QSpyRecDesc const * const desc)
{
    uint32_t t, b;
    uint64_t p;

    (void)desc; // unused
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
}
//............................................................................
// QF
// QS_QF_NEW_ATTEMPT, QS_QF_NEW
static void rec_qfNew(QSpyRecord * const me,
                      QSpyRecDesc const * const desc)
{
    uint32_t t, a, c;
    char const *s = desc->label;

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
}
//............................................................................
// QS_QF_PUBLISH
static void rec_qfPublish(QSpyRecord * const me,
                          QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c;
    uint64_t p;
    char buf[QS_FNAME_LEN_MAX];
    char const *w = (char const *)0;

    (void)desc; // unused
    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
    b = QSpyRecord_getUint32(me, 1);
    c = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_NEW_REF
static void rec_qfNewRef(QSpyRecord * const me,
                         QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c;

    (void)desc; // unused
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
}
//............................................................................
// QS_QF_DELETE_REF
static void rec_qfDeleteRef(QSpyRecord * const me,
                            QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c;

    (void)desc; // unused
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
//............................................................................
// QS_QF_GC_ATTEMPT, QS_QF_GC
static void rec_qfGc(QSpyRecord * const me,
                     QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c;
    char const *s = desc->label;

    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
    b = QSpyRecord_getUint32(me, 1);
    c = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_TICK
static void rec_qfTick(QSpyRecord * const me,
                       QSpyRecDesc const * const desc)
{
    uint32_t a, b;

    (void)desc; // unused
    a = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
    b = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_TIMEEVT_ARM, QS_QF_TIMEEVT_DISARM
static void rec_qfTimeEvtArm(QSpyRecord * const me,
                             QSpyRecDesc const * const desc)
{
    uint32_t t, b, c, d;
    uint64_t p, q;
    char buf[QS_FNAME_LEN_MAX];
    char const *s = desc->label;

    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    c = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
    d = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
    b = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_TIMEEVT_AUTO_DISARM
static void rec_qfTimeEvtAutoDisarm(QSpyRecord * const me,
                                    QSpyRecDesc const * const desc)
{
    uint32_t b;
    uint64_t p, q;
    char buf[QS_FNAME_LEN_MAX];

    (void)desc; // unused
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    b = QSpyRecord_getUint32(me, 1);
    if (QSpyRecord_OK(me)) {
        SNPRINTF_LINE("           TE%1u-ADis Obj=%s,AO=%s",
               b,
               Dictionary_get(&QSPY_objDict, p, (char *)0),
               Dictionary_get(&QSPY_objDict, q, buf));
        QSPY_onPrintLn();
        FPRINF_MATFILE("%d %"PRId64" %"PRId64"\n",
                       (int)me->rec, p, q);
   }
}
//............................................................................
// QS_QF_TIMEEVT_DISARM_ATTEMPT
static void rec_qfTimeEvtDisarmAttempt(QSpyRecord * const me,
                                       QSpyRecDesc const * const desc)
{
    uint32_t t, b;
    uint64_t p, q;
    char buf[QS_FNAME_LEN_MAX];

    (void)desc; // unused
    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    b = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_TIMEEVT_REARM
static void rec_qfTimeEvtRearm(QSpyRecord * const me,
                               QSpyRecDesc const * const desc)
{
    uint32_t t, b, c, d, e;
    uint64_t p, q;
    char buf[QS_FNAME_LEN_MAX];

    (void)desc; // unused
    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    c = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
    d = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
    b = QSpyRecord_getUint32(me, 1);
    e = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_TIMEEVT_POST
static void rec_qfTimeEvtPost(QSpyRecord * const me,
                              QSpyRecDesc const * const desc)
{
    uint32_t t, a, b;
    uint64_t p, q;
    char buf[QS_FNAME_LEN_MAX];

    (void)desc; // unused
    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    a = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
    q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    b = QSpyRecord_getUint32(me, 1);
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
}
//............................................................................
// QS_QF_CRIT_ENTRY, QS_QF_CRIT_EXIT
static void rec_qfCrit(QSpyRecord * const me,
                       QSpyRecDesc const * const desc)
{
    uint32_t t, a;
    char const *s = desc->label;

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
}
//............................................................................
// QS_QF_ISR_ENTRY, QS_QF_ISR_EXIT
static void rec_qfIsr(QSpyRecord * const me,
                      QSpyRecDesc const * const desc)
{
    uint32_t t, a, b;
    char const *s = desc->label;

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
}
//............................................................................
// scheduler records
// QS_SCHED_PREEMPT, QS_SCHED_RESTORE
static void rec_schedPreempt(QSpyRecord * const me,
                             QSpyRecDesc const * const desc)
{
    uint32_t t, a, b;
    // before QP 7.1.0: the old mutex records (label in aux)
    char const *s = (QSPY_conf.qpVersion < 710U)
                    ? desc->aux : desc->label;

    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    a = QSpyRecord_getUint32(me, 1);
    b = QSpyRecord_getUint32(me, 1);
    if (QSpyRecord_OK(me)) {
        if (QSPY_conf.qpVersion < 710U) {
            // old QS_MUTEX_UNLOCK
            SNPRINTF_LINE("%010u %s Pro=%u,Ceil=%u",
                   t, s, a, b);
            QSPY_onPrintLn();
            FPRINF_MATFILE("%d %u %u %u\n",
                           (int)me->rec, t, a, b);
        }
        else {
            SNPRINTF_LINE("%010u %s Pri=%u->%u",
                   t, s, b, a);
            QSPY_onPrintLn();
            FPRINF_MATFILE("%d %u %u %u\n",
                           (int)me->rec, t, b, a);
        }
    }
}
//............................................................................
// QS_SCHED_LOCK, QS_SCHED_UNLOCK
static void rec_schedLock(QSpyRecord * const me,
                          QSpyRecDesc const * const desc)
{
    uint32_t t, a, b;
    char const *s = desc->label;

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
}
//............................................................................
// QS_SCHED_NEXT
static void rec_schedNext(QSpyRecord * const me,
                          QSpyRecDesc const * const desc)
{
    uint32_t t, a, b;

    (void)desc; // unused
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
}
//............................................................................
// QS_SCHED_IDLE
static void rec_schedIdle(QSpyRecord * const me,
                          QSpyRecDesc const * const desc)
{
    uint32_t t, a;

    (void)desc; // unused
    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    a = QSpyRecord_getUint32(me, 1);
    if (QSpyRecord_OK(me)) {
        SNPRINTF_LINE("%010u Sch-Idle Pri=%u->0",
               t, a);
        QSPY_onPrintLn();
        FPRINF_MATFILE("%d %u %u\n",
                       (int)me->rec, t, a);
    }
}
//............................................................................
// enum dictionary
// QS_ENUM_DICT
static void rec_enumDict(QSpyRecord * const me,
                         QSpyRecDesc const * const desc)
{
    uint32_t a, b;
    char const *s = (char const *)0;

    (void)desc; // unused
    a = QSpyRecord_getUint32(me, 1);
    b = QSpyRecord_getUint32(me, 1) & 0x7U;
    s = QSpyRecord_getStr(me);
    if (QSpyRecord_OK(me)) {
        Dictionary_put(&QSPY_enumDict[b], a, s);
        SNPRINTF_LINE("           Enum-Dic %03d,Grp=%1d->%s",
                      a, b, s);
        QSPY_onPrintLn();
    }
}
//............................................................................
// Miscellaneous built-in QS records
// QS_TEST_PAUSED
static void rec_testPaused(QSpyRecord * const me,
                           QSpyRecDesc const * const desc)
{
    (void)desc; // unused
    if (QSpyRecord_OK(me)) {
        SNPRINTF_LINE("           %s", "TstPause");
        QSPY_onPrintLn();
    }
}
//............................................................................
// QS_TEST_PROBE_GET
static void rec_testProbeGet(QSpyRecord * const me,
                             QSpyRecDesc const * const desc)
{
    uint32_t t, a;
    uint64_t q;

    (void)desc; // unused
    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
    a = QSpyRecord_getUint32(me, 4U);
    if (QSpyRecord_OK(me)) {
        SNPRINTF_LINE("%010u TstProbe Fun=%s,Data=%d",
                      t, Dictionary_get(&QSPY_funDict,
                      q, (char *)0), a);
        QSPY_onPrintLn();
    }
}
//............................................................................
// QS_SIG_DICT
static void rec_sigDict(QSpyRecord * const me,
                        QSpyRecDesc const * const desc)
{
    uint32_t a;
    uint64_t p;
    char const *s = (char const *)0;

    (void)desc; // unused
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
}
//............................................................................
// QS_OBJ_DICT
static void rec_objDict(QSpyRecord * const me,
                        QSpyRecDesc const * const desc)
{
    uint64_t p;
    char const *s = (char const *)0;

    (void)desc; // unused
    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    s = QSpyRecord_getStr(me);

    // for backward compatibility replace the '['/']' with '<'/'>'
    if (QSPY_conf.qpVersion < 690U) {
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
        // if needed, update the object in the Sequence dictionary
        QSEQ_updateDictionary(s, p);
#endif
    }
}
//............................................................................
// QS_FUN_DICT
static void rec_funDict(QSpyRecord * const me,
                        QSpyRecDesc const * const desc)
{
    uint64_t p;
    char const *s = (char const *)0;

    (void)desc; // unused
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
}
//............................................................................
// QS_USR_DICT
static void rec_usrDict(QSpyRecord * const me,
                        QSpyRecDesc const * const desc)
{
    uint32_t a;
    char const *s = (char const *)0;

    (void)desc; // unused
    a = QSpyRecord_getUint32(me, 1);
    s = QSpyRecord_getStr(me);
    if (QSpyRecord_OK(me)) {
        Dictionary_put(&QSPY_usrDict, a, s);
        SNPRINTF_LINE("           Usr-Dict %08d->%s",
                a, s);
        QSPY_onPrintLn();
    }
}
//............................................................................
// the configuration changed (Target reset or new Target info):
// reload the external dictionaries (-d option) and notify the Back-End
static void targetInfo_reload(void) {
#ifdef QSPY_APP
    if (QDIC_isActive()) {
        QSPY_readDict();
    }
    QSPY_configChanged();
#endif
}
//............................................................................
// QS_TARGET_INFO
static void rec_targetInfo(QSpyRecord * const me,
                           QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c, d, e;
    char buf[QS_FNAME_LEN_MAX];
    char const *s = (char const *)0;

    (void)desc; // unused
    a = QSpyRecord_getUint32(me, 1);
    if ((a & 0x03U) == 0x02U) { // is this the new format?
        t = ~QSpyRecord_getUint32(me, 4); // QP-version/QP-date
        b = (a >> 7U) & 0x01U;   // endianness
        c = ((a >> 2U) & 0x03U); // QP framework type
        a = ((a >> 6U) & 0x01U); // target reset
    }
    else { // old format
        t = QSpyRecord_getUint32(me, 2);
        b = (t >> 15) & 0x01U;   // endianness
        t = (t & 0x7FFFU);       // QP-version/NO-QP-date
        c = 0x0U;                // QP framework type (unknown)
        a = (a & 0x01U);         // target reset
    }
    // extract the data bytes...
    for (e = 0U; e < 13U; ++e) {
        buf[e] = (char)QSpyRecord_getUint32(me, 1);
    }

    if (QSpyRecord_OK(me)) {
        s = (a != 0U) ? "Trg-RST " : "Trg-Info";

        // validate the sizes BEFORE applying anything...
        QSpyConfig cand = QSPY_conf;
        cand.objPtrSize   = (uint8_t)(buf[3] & 0xFU);
        cand.funPtrSize   = (uint8_t)((buf[3] >> 4) & 0xFU);
        cand.tstampSize   = (uint8_t)(buf[4] & 0xFU);
        cand.sigSize      = (uint8_t)(buf[0] & 0xFU);
        cand.evtSize      = (uint8_t)((buf[0] >> 4) & 0xFU);
        cand.queueCtrSize = (uint8_t)(buf[1] & 0x0FU);
        cand.poolCtrSize  = (uint8_t)((buf[2] >> 4) & 0xFU);
        cand.poolBlkSize  = (uint8_t)(buf[2] & 0xFU);
        cand.tevtCtrSize  = (uint8_t)((buf[1] >> 4) & 0xFU);
        char const *bad = QSPY_checkConfig(&cand);
        if (bad != (char const *)0) {
            SNPRINTF_LINE("   <COMMS> ERROR    %s Rejected: "
                "invalid %s (Target info NOT applied)", s, bad);
            QSPY_printError();
            return;
        }

        // apply the target info...
        // find differences from the current config and store in 'd'
        d = 0U; // assume no difference in the target info
        CONFIG_UPDATE(qpVersion,   (uint16_t)(t % 10000U), d);
        CONFIG_UPDATE(qpDate,      (uint32_t)(t / 10000U), d);
        CONFIG_UPDATE(qpType,      (uint8_t)c, d);
        CONFIG_UPDATE(endianness,  (uint8_t)b, d);
        CONFIG_UPDATE(objPtrSize,  (uint8_t)(buf[3] & 0xFU), d);
        CONFIG_UPDATE(funPtrSize,  (uint8_t)((buf[3] >> 4) & 0xFU),d);
        CONFIG_UPDATE(tstampSize,  (uint8_t)(buf[4] & 0xFU), d);
        CONFIG_UPDATE(sigSize,     (uint8_t)(buf[0] & 0xFU), d);
        CONFIG_UPDATE(evtSize,     (uint8_t)((buf[0] >> 4) & 0xFU),d);
        CONFIG_UPDATE(queueCtrSize,(uint8_t)(buf[1] & 0x0FU), d);
        CONFIG_UPDATE(poolCtrSize, (uint8_t)((buf[2] >> 4) & 0xFU),d);
        CONFIG_UPDATE(poolBlkSize, (uint8_t)(buf[2] & 0xFU), d);
        CONFIG_UPDATE(tevtCtrSize, (uint8_t)((buf[1] >> 4) & 0xFU),d);

        // unpack the target build timestamp
        for (e = 0U; e < sizeof(QSPY_conf.tbuild); ++e) {
            CONFIG_UPDATE(tbuild[e], (uint8_t)buf[7U + e], d);
        }

        // the dictionaries must follow the Target pointer sizes
        QSPY_configDictionaries();

        SNPRINTF_LINE("           %s QP-Ver=%u,"
               "Build=%02u%02u%02u_%02u%02u%02u",
               s,
               (unsigned)QSPY_conf.qpVersion,
               (unsigned)QSPY_conf.tbuild[5],
               (unsigned)QSPY_conf.tbuild[4],
               (unsigned)QSPY_conf.tbuild[3],
               (unsigned)QSPY_conf.tbuild[2],
               (unsigned)QSPY_conf.tbuild[1],
               (unsigned)QSPY_conf.tbuild[0]);
        QSPY_onPrintLn();

        if (a != 0U) {  // is this Target RESET?
            // always reset dictionaries upon target reset
            QSPY_resetAllDictionaries();
            targetInfo_reload();
            //TBD: close and re-open MATLAB, Sequence file, etc.

            // reset the QSPY-Tx channel, if available
            if (l_txResetFun != (QSPY_resetFun)0) {
                (*l_txResetFun)();
            }
        }
        // the configuration changed WITHOUT a Target reset? (the dictionaries
        // collected so far belong to the previous configuration)
        else if (d != 0U) {
            // reset dictionaries upon config change
            QSPY_resetAllDictionaries();
            SNPRINTF_LINE("   <QSPY-> %s",
                "Target info changed (dictionaries discarded)");
            QSPY_printInfo();
            targetInfo_reload();
        }
    }
}
//............................................................................
// QS_TARGET_DONE
static void rec_targetDone(QSpyRecord * const me,
                           QSpyRecDesc const * const desc)
{
    uint32_t t, a;

    (void)desc; // unused
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
}
//............................................................................
// QS_RX_STATUS
static void rec_rxStatus(QSpyRecord * const me,
                         QSpyRecDesc const * const desc)
{
    uint32_t a;

    (void)desc; // unused
    a = QSpyRecord_getUint32(me, 1U);
    QSPY_output.rx_status = (int)a;
    if (QSpyRecord_OK(me)) {
        if (a < 128U) { // Ack?
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
}
//............................................................................
// QS_QUERY_DATA before QP 8.1.0
static void queryData_pre810(QSpyRecord * const me, uint32_t t, uint32_t a) {
    uint32_t b = 0U;
    uint32_t c = 0U;
    uint32_t d = 0U;
    uint32_t e = 0U;
    uint64_t p = 0U;
    uint64_t q = 0U;

    p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
    switch (a) {
        case QS_OBJ_SM: //lint -fallthrough
        case QS_OBJ_AO:
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            break;
        case QS_OBJ_MP:
            b = QSpyRecord_getUint32(me, QSPY_conf.poolCtrSize);
            c = QSpyRecord_getUint32(me, QSPY_conf.poolCtrSize);
            break;
        case QS_OBJ_EQ:
            b = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
            c = QSpyRecord_getUint32(me, QSPY_conf.queueCtrSize);
            break;
        case QS_OBJ_TE:
            q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            b = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
            c = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
            d = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            e = QSpyRecord_getUint32(me, 1);
            break;
        case QS_OBJ_AP:
            break;
        default:
            break;
    }
    if (a >= (uint32_t)(sizeof(l_qs_obj)/sizeof(l_qs_obj[0]))) {
        QSpyRecord_fail(me, QS_REC_BAD_DATA, "object kind");
    }
    if (QSpyRecord_OK(me)) {
        SNPRINTF_LINE("%010u Query-%s Obj=%s",
               t,
               l_qs_obj[a],
               Dictionary_get(&QSPY_objDict, p, (char *)0));
        switch (a) {
            case QS_OBJ_SM: //lint -fallthrough
            case QS_OBJ_AO:
                SNPRINTF_APPEND(",State=%s",
                    Dictionary_get(&QSPY_funDict, q, (char *)0));
                break;
            case QS_OBJ_MP:
                SNPRINTF_APPEND(",Free=%u,Min=%u",
                    b, c);
                break;
            case QS_OBJ_EQ:
                SNPRINTF_APPEND(",Que<Free=%u,Min=%u>",
                    b, c);
                break;
            case QS_OBJ_TE:
                SNPRINTF_APPEND(
                    ",Rate=%u,Sig=%s,Tim=%u,Int=%u,Flags=0x%02X",
                    (e & 0x0FU),
                    SigDictionary_get(&QSPY_sigDict, d, q, (char *)0),
                    b, c,
                    (e & 0xF0U));
                break;
            case QS_OBJ_AP:
                break;
            default:
                break;
        }
        QSPY_onPrintLn();
    }
}
//............................................................................
// QS_QUERY_DATA since QP 8.1.0
static void queryData_810(QSpyRecord * const me, uint32_t t, uint32_t a) {
    uint32_t b = 0U;
    uint32_t c = 0U;
    uint32_t d = 0U;
    uint32_t e = 0U;
    uint32_t f = 0U;
    uint64_t p = 0U;
    uint64_t q = 0U;
    char const *s;

    switch (a) {
        case QS_OBJ_SM:
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.funPtrSize);
            break;
        case QS_OBJ_AO:
            b = QSpyRecord_getUint32(me, 1U);
            c = QSpyRecord_getUint32(me, 2U);
            d = QSpyRecord_getUint32(me, 2U);
            e = QSpyRecord_getUint32(me, 2U);
            break;
        case QS_OBJ_MP:
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            b = QSpyRecord_getUint32(me, 2U);
            c = QSpyRecord_getUint32(me, 2U);
            d = QSpyRecord_getUint32(me, 2U);
            e = QSpyRecord_getUint32(me, 2U);
            break;
        case QS_OBJ_EQ:
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            b = QSpyRecord_getUint32(me, 2U);
            c = QSpyRecord_getUint32(me, 2U);
            d = QSpyRecord_getUint32(me, 2U);
            break;
        case QS_OBJ_TE:
            p = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            q = QSpyRecord_getUint64(me, QSPY_conf.objPtrSize);
            b = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
            c = QSpyRecord_getUint32(me, QSPY_conf.tevtCtrSize);
            d = QSpyRecord_getUint32(me, QSPY_conf.sigSize);
            e = QSpyRecord_getUint32(me, 1U);
            break;
        case QS_OBJ_EP:
            b = QSpyRecord_getUint32(me, 1U);
            c = QSpyRecord_getUint32(me, 2U);
            d = QSpyRecord_getUint32(me, 2U);
            e = QSpyRecord_getUint32(me, 2U);
            f = QSpyRecord_getUint32(me, 2U);
            break;
        case QS_OBJ_AP:
            break;
        default:
            break;
    }
    if (a >= (uint32_t)(sizeof(l_qs_obj)/sizeof(l_qs_obj[0]))) {
        QSpyRecord_fail(me, QS_REC_BAD_DATA, "object kind");
    }
    if (QSpyRecord_OK(me)) {
        SNPRINTF_LINE("%010u Query-%s",
               t,
               l_qs_obj[a]);
        s = Dictionary_get(&QSPY_objDict, p, (char *)0);
        switch (a) {
            case QS_OBJ_SM:
                SNPRINTF_APPEND(" Obj=%s,State=%s",
                    s, Dictionary_get(&QSPY_funDict, q, (char*)0));
                break;
            case QS_OBJ_AO:
                SNPRINTF_APPEND(" Pri=%u,Que<Use=%u,Free=%u,Min=%u>",
                    b, c, d, e);
                break;
            case QS_OBJ_MP:
                SNPRINTF_APPEND(" Obj=%s,Use=%u,Free=%u,Min=%u,Size=%u",
                    s, b, c, d, e);
                break;
            case QS_OBJ_EQ:
                SNPRINTF_APPEND(" Obj=%s,Use=%u,Free=%u,Min=%u",
                    s, b, c, d);
                break;
            case QS_OBJ_TE:
                SNPRINTF_APPEND(
                    " Obj=%s,Rate=%u,Sig=%s,Tim=%u,Int=%u,Flags=0x%02X",
                    s, (e & 0x0FU),
                    SigDictionary_get(&QSPY_sigDict, d, q, (char *)0),
                    b, c,
                    (e & 0xF0U));
                break;
            case QS_OBJ_EP:
                SNPRINTF_APPEND(" Id=%u,Use=%u,Free=%u,Min=%u,Size=%u",
                    b, c, d, e, f);
                break;
            case QS_OBJ_AP:
                break;
            default:
                break;
        }
        QSPY_onPrintLn();
    }
}
//............................................................................
// QS_QUERY_DATA
static void rec_queryData(QSpyRecord * const me,
                          QSpyRecDesc const * const desc)
{
    uint32_t const t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    uint32_t const a = QSpyRecord_getUint32(me, 1U); // object kind

    (void)desc; // unused
    if (QSPY_conf.qpVersion < 810U) {
        queryData_pre810(me, t, a);
    }
    else {
        queryData_810(me, t, a);
    }
}
//............................................................................
// QS_PEEK_DATA
static void rec_peekData(QSpyRecord * const me,
                         QSpyRecDesc const * const desc)
{
    uint32_t t, a, b, c;
    char const *w = (char const *)0;

    (void)desc; // unused
    t = QSpyRecord_getUint32(me, QSPY_conf.tstampSize);
    a = QSpyRecord_getUint32(me, 2);  // offset
    b = QSpyRecord_getUint32(me, 1);  // data size
    w = (char const *)0;
    c = 0U;
    if ((b == 1U) || (b == 2U) || (b == 4U)) {
        w = (char const *)QSpyRecord_getMem(me, (uint8_t)b, &c);
    }
    else if (me->len >= 0) { // no error so far?
        QSpyRecord_fail(me, QS_REC_BAD_DATA, "peek data size");
    }
    if (QSpyRecord_OK(me)) {
        SNPRINTF_LINE("%010u Trg-Peek Offs=%d,Size=%d,Num=%d,Data=<",
                      t, a, b, c);
        for (uint32_t i = 0U; i < c; ++i, w += b) {
            uint8_t const *u = (uint8_t const *)w;
            uint32_t v = 0U;
            for (int k = (int)b - 1; k >= 0; --k) { // little-endian
                v = (v << 8) | (uint32_t)u[k];
            }
            SNPRINTF_APPEND((b == 1U) ? "%02X%c"
                            : (b == 2U) ? "%04X%c" : "%08X%c",
                            (unsigned)v,
                            (i + 1U < c) ? ',' : '>');
        }
        if (c == 0U) {
            SNPRINTF_APPEND("%c", '>');
        }
        QSPY_onPrintLn();
    }
}
//............................................................................
// QS_ASSERT_FAIL
static void rec_assertFail(QSpyRecord * const me,
                           QSpyRecDesc const * const desc)
{
    uint32_t t, a;
    char const *s = (char const *)0;

    (void)desc; // unused
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
}
//............................................................................
// QS_QF_RUN
static void rec_qfRun(QSpyRecord * const me,
                      QSpyRecDesc const * const desc)
{
    (void)desc; // unused
    if (QSpyRecord_OK(me)) {
        SNPRINTF_LINE("           %s", "QF_RUN");
        QSPY_onPrintLn();
#ifdef QSPY_APP
        if (QDIC_isActive()) {
            QSPY_writeDict();
        }
#endif
    }
}
//............................................................................
// Semaphore records
// QS_SEM_TAKE, QS_SEM_BLOCK, QS_SEM_SIGNAL, QS_SEM_BLOCK_ATTEMPT
static void rec_sem(QSpyRecord * const me,
                    QSpyRecDesc const * const desc)
{
    uint32_t t, a, b;
    uint64_t p;
    char const *s = desc->label;

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
}
//............................................................................
// Mutex records
// QS_MTX_LOCK, QS_MTX_UNLOCK, QS_MTX_LOCK_ATTEMPT, QS_MTX_UNLOCK_ATTEMPT
static void rec_mtxLock(QSpyRecord * const me,
                        QSpyRecDesc const * const desc)
{
    uint32_t t, a, b;
    uint64_t p;
    char const *s = desc->label;

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
}
//............................................................................
// QS_MTX_BLOCK, QS_MTX_BLOCK_ATTEMPT
static void rec_mtxBlock(QSpyRecord * const me,
                         QSpyRecDesc const * const desc)
{
    uint32_t t, a, b;
    uint64_t p;
    char const *s = desc->label;

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
}
//............................................................................
// Unknown records
// NOTE: the Application-Specific records have been already
// sifted out in QSPY_parse()
// default
static void rec_unknown(QSpyRecord * const me,
                        QSpyRecDesc const * const desc)
{
    (void)desc; // unused
    SNPRINTF_LINE("           Unknown Rec=%d,Len=%d",
           (int)me->rec, (int)me->len);
    QSPY_onPrintLn();
}
//............................................................................
void QSPY_printInfo(void) {
    QSPY_output.type = INF_OUT; // this is an internal info message
    QSPY_onPrintLn();
}
//............................................................................
void QSPY_printError(void) {
    QSPY_output.type = ERR_OUT; // this is an error message
    QSPY_onPrintLn();
}

//============================================================================
static uint8_t l_record[QS_RECORD_SIZE_MAX];
static uint8_t *l_pos   = l_record; // position within the record
static uint8_t l_chksum = 0U;
static uint8_t l_esc    = 0U;
static uint8_t l_seq    = 0U;
static bool    l_discard     = false; // discarding a too-long record
static bool    l_justStarted = true;  // no frame byte seen since reset
static bool    l_seqValid    = false; // healthy record seen since reset

//............................................................................
void QSPY_reset(void) {
    l_pos    = l_record; // position within the record
    l_chksum = 0U;
    l_esc    = 0U;
    l_seq    = 0U;
    l_discard     = false;
    l_justStarted = true;  // new link: no checksum check of the 1st frame
    l_seqValid    = false; // new link: sequence not known yet
}
//............................................................................
// name of the record currently being received (for error messages)
static char const *QSPY_rxRecName(char *buf, size_t size) {
    if (l_pos < &l_record[2]) { // record-ID not received?
        return "?";
    }
    return QSpyRecord_name(l_record[1], buf, size);
}
//............................................................................
// store one (unescaped) byte of the record being received
static void QSPY_rxByte(uint8_t b) {
    l_chksum = (uint8_t)(l_chksum + b);
    if (l_pos < &l_record[sizeof(l_record)]) {
        *l_pos++ = b;
    }
    else { // record too long
        char rbuf[16];
        SNPRINTF_LINE("   <COMMS> ERROR    Record too long at "
                      "Seq=%u(?),Rec=%s(?) (discarded)",
                      (unsigned)l_seq, QSPY_rxRecName(rbuf, sizeof(rbuf)));
        QSPY_printError();
        // discard everything until the next frame byte, so that the tail
        // of the too-long record is never mistaken for a valid record
        l_discard = true;
    }
}
//............................................................................
void QSPY_parse(uint8_t const *buf, uint32_t nBytes) {
    for (; nBytes != 0U; --nBytes) {
        uint8_t b = *buf++;

        if (b == QS_FRAME) { // frame byte?
            if (l_discard) { // discarding the rest of a too-long record?
                l_discard = false;
            }
            else if (l_esc) { // frame byte right after the escape byte?
                if (!l_justStarted) {
                    char rbuf[16];
                    SNPRINTF_LINE("   <COMMS> ERROR    Bad escape sequence "
                        "in Rec=%s(?) (discarded)",
                        QSPY_rxRecName(rbuf, sizeof(rbuf)));
                    QSPY_printError();
                }
            }
            else if (l_chksum != QS_GOOD_CHKSUM) { // bad checksum?
                if (!l_justStarted) {
                    char rbuf[16];
                    char sbuf[8];
                    if (l_pos > &l_record[0]) { // sequence number received?
                        (void)SNPRINTF_S(sbuf, sizeof(sbuf), "%u",
                                         (unsigned)l_record[0]);
                    }
                    else {
                        sbuf[0] = '?';
                        sbuf[1] = '\0';
                    }
                    // NOTE: the sequence number of the corrupted record
                    // itself is not reliable, hence "(?)"
                    SNPRINTF_LINE("   <COMMS> ERROR    Bad checksum in "
                        "Rec=%s(?),Seq=%s(?)",
                        QSPY_rxRecName(rbuf, sizeof(rbuf)), sbuf);
                    QSPY_printError();
                }
            }
            else if (l_pos < &l_record[3]) { // record too short?
                char rbuf[16];
                SNPRINTF_LINE("   <COMMS> ERROR    Record too short at "
                              "Seq=%u(?),Rec=%s(?)",
                              (unsigned)l_seq,
                              QSPY_rxRecName(rbuf, sizeof(rbuf)));
                QSPY_printError();
            }
            else { // a healthy record received
                QSpyRecord qrec;
                int parse = 1;
                ++l_seq; // increment with natural wrap-around

                if (l_seqValid) {
                    // data discontinuity found?
                    // but not for the QS_EMPTY record?

                    if ((l_seq != l_record[0])
                         && (l_record[1] != QS_EMPTY))
                    {
                        SNPRINTF_LINE("   <COMMS> ERROR    Discontinuity "
                            "Seq=%u->%u",
                            (unsigned)(l_seq - 1), (unsigned)l_record[0]);
                        QSPY_printError();
                    }
                }
                l_seq = l_record[0];
                l_seqValid = true; // sequence known from now on

                QSpyRecord_init(&qrec, l_record, (uint32_t)(l_pos - l_record));

                if (l_custParseFun != (QSPY_CustParseFun)0) {
                    parse = (*l_custParseFun)(&qrec);
                    if (parse) {
                        // re-initialize the record for parsing again
                        QSpyRecord_init(&qrec,
                            l_record, (uint32_t)(l_pos - l_record));
                    }
                }
                if (parse) {
                    if (qrec.rec < QS_USER) { // pre-defined record?
                        QSpyRecord_processPre(&qrec);
                    }
                    else { // user (app-specific) record
                        QSpyRecord_processUser(&qrec);
                    }
                }
            }

            // get ready for the next record ...
            l_justStarted = false; // the first frame byte seen
            l_chksum = 0U;
            l_pos = l_record;
            l_esc = 0U;
        }
        else if (l_discard) { // discarding the rest of a too-long record?
            // ignore the byte
        }
        else if (l_esc) { // escaped byte arrived?
            l_esc = 0U;
            QSPY_rxByte((uint8_t)(b ^ QS_ESC_XOR));
        }
        else if (b == QS_ESC) {   // transparent byte?
            l_esc = 1U;
        }
        else {  // a regular un-escaped byte
            QSPY_rxByte(b);
        }
    }
}

//............................................................................
void QSPY_resetAllDictionaries(void) {
    Dictionary_reset(&QSPY_funDict);
    Dictionary_reset(&QSPY_objDict);
    Dictionary_reset(&QSPY_usrDict);
    SigDictionary_reset(&QSPY_sigDict);
    for (unsigned i = 0U;
         i < sizeof(QSPY_enumDict)/sizeof(QSPY_enumDict[0]);
         ++i)
    {
        Dictionary_reset(&QSPY_enumDict[i]);
    }

#ifdef QSPY_APP
    QSEQ_dictionaryReset();
    // find out if NULL needs to be added to the Sequence dictionary...
    QSEQ_updateDictionary("NULL", 0);
#endif

    // pre-fill known user entries
    Dictionary_put(&QSPY_usrDict, 124, "QUTEST_ON_POST");
}
//............................................................................
SigType QSPY_findSig(char const* name, ObjType obj) {
    return SigDictionary_findSig(&QSPY_sigDict, name, obj);
}
//............................................................................
KeyType QSPY_findObj(char const* name) {
    return Dictionary_findKey(&QSPY_objDict, name);
}
//............................................................................
KeyType QSPY_findFun(char const* name) {
    return Dictionary_findKey(&QSPY_funDict, name);
}
//............................................................................
KeyType QSPY_findUsr(char const* name) {
    return Dictionary_findKey(&QSPY_usrDict, name);
}
//............................................................................
KeyType QSPY_findEnum(char const *name, uint8_t group) {
    Q_ASSERT(group < sizeof(QSPY_enumDict)/sizeof(QSPY_enumDict[0]));
    return Dictionary_findKey(&QSPY_enumDict[group], name);
}
//............................................................................
int QSPY_getGroup(int recId) {
    return (recId < QS_USER) // is it a Predefined record?
        ? l_recDesc[recId].group
        : QS_GRP_UA;
}

// Dictionary class ========================================================*/
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

//............................................................................
void Dictionary_ctor(Dictionary * const me,
                            DictEntry *sto, uint32_t capacity)
{
    me->sto      = sto;
    me->capacity = (int)capacity;
    me->entries  = 0;
    me->keySize  = 4;
    me->overflow = false;
}
//............................................................................
 void Dictionary_config(Dictionary * const me, int keySize) {
    me->keySize = keySize;
}
//............................................................................
char const *Dictionary_at(Dictionary * const me, unsigned idx) {
    if (idx < (unsigned)me->entries) {
        return me->sto[idx].name;
    }
    else {
        return "";
    }
}
//............................................................................
bool Dictionary_put(Dictionary * const me,
                    KeyType key, char const *name)
{
    int idx = Dictionary_find(me, key);
    if (idx >= 0) { // the key found?
        string_copy(me->sto[idx].name, sizeof(me->sto[idx].name), name);
        return true;
    }
    int n = me->entries;
    if (n < me->capacity) {
        me->sto[n].key = key;
        string_copy(me->sto[n].name, sizeof(me->sto[n].name), name);
        ++me->entries;
        // keep the entries sorted by the key
        qsort(me->sto, (uint32_t)me->entries, sizeof(me->sto[0]),
              &Dictionary_comp);
        return true;
    }
    if (!me->overflow) { // overflow not reported yet?
        me->overflow = true;
        SNPRINTF_LINE("   <QSPY-> ERROR    Dictionary full (capacity=%d), "
                      "entry NOT stored: %s", me->capacity, name);
        QSPY_printError();
    }
    return false;
}
//............................................................................
char const *Dictionary_get(Dictionary * const me, KeyType key, char *buf) {
    int idx;
    if ((key == 0U) && (me->keySize >= 4)) {
        return "NULL";
    }
    idx = Dictionary_find(me, key);
    if (idx >= 0) { // key found?
        return me->sto[idx].name;
    }
    else { // key not found
        if (buf == 0) { // extra buffer not provided?
            buf = me->notFound.name; // use the internal location
        }
        // otherwise use the provided buffer...
        if (me->keySize <= 1) {
            SNPRINTF_S(buf, QS_DNAME_LEN_MAX, "%03d", (unsigned)key);
        }
        else if ((me->keySize <= 4) && (key <= 0xFFFFFFFFU)) {
            SNPRINTF_S(buf, QS_DNAME_LEN_MAX, "0x%08X", (unsigned)key);
        }
        else {
            SNPRINTF_S(buf, QS_DNAME_LEN_MAX, "0x%016"PRIX64"", key);
        }
        //Dictionary_put(me, key, buf); // put into the dictionary
        return buf;
    }
}
//............................................................................
int Dictionary_find(Dictionary * const me, KeyType key) {
    // binary search algorithm (over the entries sorted by the key)...
    int first = 0;
    int last  = me->entries - 1;
    while (first <= last) {
        int mid = first + ((last - first) / 2);
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
    return -1; // entry not found
}
//............................................................................
KeyType Dictionary_findKey(Dictionary * const me, char const *name) {
    // brute-force search algorithm...
    for (int i = 0; i < me->entries; ++i) {
        if (strncmp(me->sto[i].name, name, sizeof(me->sto[i].name)) == 0) {
            return me->sto[i].key;
        }
    }
    return KEY_NOT_FOUND;
}
//............................................................................
void Dictionary_reset(Dictionary * const me) {
    for (int i = 0; i < me->capacity; ++i) {
        me->sto[i].key = (KeyType)0;
        me->sto[i].name[0] = '\0';
    }
    me->entries  = 0;
    me->overflow = false;
}
// SigDictionary class =====================================================*/
static int SigDictionary_comp(void const *arg1, void const *arg2) {
    // entries sorted by the signal, then by the object (total order)
    SigDictEntry const *e1 = (SigDictEntry const *)arg1;
    SigDictEntry const *e2 = (SigDictEntry const *)arg2;
    if (e1->sig != e2->sig) {
        return (e1->sig > e2->sig) ? 1 : -1;
    }
    if (e1->obj != e2->obj) {
        return (e1->obj > e2->obj) ? 1 : -1;
    }
    return 0;
}
//............................................................................
void SigDictionary_ctor(SigDictionary * const me,
                        SigDictEntry *sto, uint32_t capacity)
{
    me->sto      = sto;
    me->capacity = (int)capacity;
    me->entries  = 0;
    me->ptrSize  = 4;
    me->overflow = false;
}
//............................................................................
void SigDictionary_config(SigDictionary * const me, int ptrSize) {
    me->ptrSize = ptrSize;
}
//............................................................................
bool SigDictionary_put(SigDictionary * const me,
                       SigType sig, ObjType obj, char const *name)
{
    int idx = SigDictionary_findExact(me, sig, obj);
    if (idx >= 0) { // the exact (sig, obj) key found?
        string_copy(me->sto[idx].name, sizeof(me->sto[idx].name), name);
        return true;
    }
    int n = me->entries;
    if (n < me->capacity) {
        me->sto[n].sig = sig;
        me->sto[n].obj = obj;
        string_copy(me->sto[n].name, sizeof(me->sto[n].name), name);
        ++me->entries;
        // keep the entries sorted by (sig, obj)
        qsort(me->sto, (uint32_t)me->entries, sizeof(me->sto[0]),
              &SigDictionary_comp);
        return true;
    }
    if (!me->overflow) { // overflow not reported yet?
        me->overflow = true;
        SNPRINTF_LINE("   <QSPY-> ERROR    Signal Dictionary full "
                      "(capacity=%d), entry NOT stored: %s",
                      me->capacity, name);
        QSPY_printError();
    }
    return false;
}
//............................................................................
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
    else { // key not found
        if (buf == 0) { // extra buffer not provided?
            buf = me->notFound.name; // use the internal location
        }
        // otherwise use the provided buffer...
        if ((me->ptrSize <= 4) && (obj <= 0xFFFFFFFFU)) {
            SNPRINTF_S(buf, QS_DNAME_LEN_MAX, "%08u,Obj=0x%08X",
                      (unsigned)sig, (unsigned)obj);
        }
        else {
            SNPRINTF_S(buf, QS_DNAME_LEN_MAX, "%08u,Obj=0x%016"PRIX64"",
                       (unsigned)sig, (uint64_t)obj);
        }
        //SigDictionary_put(me, sig, obj, buf); // put into the dictionary
        return buf;
    }
}
//............................................................................
// finds the range [*pLo..*pHi] of entries with the given signal;
// returns false when the signal is not in the dictionary
static bool SigDictionary_range(SigDictionary const * const me, SigType sig,
                                int *pLo, int *pHi)
{
    int first = 0;
    int last  = me->entries - 1;
    while (first <= last) {
        int mid = first + ((last - first) / 2);
        if (me->sto[mid].sig == sig) {
            int lo = mid;
            int hi = mid;
            while ((lo > 0) && (me->sto[lo - 1].sig == sig)) {
                --lo;
            }
            while ((hi < me->entries - 1) && (me->sto[hi + 1].sig == sig)) {
                ++hi;
            }
            *pLo = lo;
            *pHi = hi;
            return true;
        }
        if (me->sto[mid].sig > sig) {
            last = mid - 1;
        }
        else {
            first = mid + 1;
        }
    }
    return false;
}
//............................................................................
// finds the entry with exactly the (sig, obj) key, or -1
int SigDictionary_findExact(SigDictionary * const me,
                            SigType sig, ObjType obj)
{
    int lo;
    int hi;
    if (SigDictionary_range(me, sig, &lo, &hi)) {
        for (int i = lo; i <= hi; ++i) {
            if (me->sto[i].obj == obj) {
                return i;
            }
        }
    }
    return -1; // entry not found
}
//............................................................................
// finds the entry for looking up the name of (sig, obj):
// 1. the exact (sig, obj) entry; 2. the global (sig, 0) entry;
// 3. for obj == 0 only: any entry with the signal; otherwise -1
int SigDictionary_find(SigDictionary * const me,
                       SigType sig, ObjType obj)
{
    int lo;
    int hi;
    if (!SigDictionary_range(me, sig, &lo, &hi)) {
        return -1; // signal not found
    }
    int glob = -1;
    for (int i = lo; i <= hi; ++i) {
        if (me->sto[i].obj == obj) {
            return i; // exact match
        }
        if (me->sto[i].obj == (ObjType)0) {
            glob = i;
        }
    }
    if (glob >= 0) {
        return glob;
    }
    return (obj == (ObjType)0) ? lo : -1;
}
//............................................................................
SigType SigDictionary_findSig(SigDictionary * const me,
                              char const *name, ObjType obj)
{
    // brute-force search algorithm...
    // prefer the signal of the given object over a global signal
    SigType glob = SIG_NOT_FOUND;
    for (int i = 0; i < me->entries; ++i) {
        if (strncmp(me->sto[i].name, name, sizeof(me->sto[i].name)) == 0) {
            if ((me->sto[i].obj == obj) && (obj != (ObjType)0)) {
                return me->sto[i].sig;
            }
            if ((me->sto[i].obj == (ObjType)0) && (glob == SIG_NOT_FOUND)) {
                glob = me->sto[i].sig;
            }
        }
    }
    return glob; // global signal or SIG_NOT_FOUND
}
//............................................................................
void SigDictionary_reset(SigDictionary * const me) {
    for (int i = 0; i < me->capacity; ++i) {
        me->sto[i].sig = (SigType)0;
        me->sto[i].obj = (ObjType)0;
        me->sto[i].name[0] = '\0';
    }
    me->entries  = 0;
    me->overflow = false;
}
//----------------------------------------------------------------------------
// simplified string_copy() implementation "good enough" for the intended use
int string_copy(char *dest, size_t dest_size, char const *src) {
    size_t i = 0U;
    for (; i < dest_size; ++i) {
        dest[i] = src[i];
        if (src[i] == '\0') {
            break;
        }
    }
    if (i == dest_size) {
        dest[dest_size - 1U] = '\0';
        return -1; // destination holds *truncated* source
    }
    return 0; // destination fits the source
}
