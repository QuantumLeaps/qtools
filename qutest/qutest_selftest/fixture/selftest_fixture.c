//============================================================================
// QUTest self-test fixture
//
// A deliberately minimal QUTest target whose behavior is fully controlled
// by the test scripts through QS_onCommand(). It exists only to verify the
// QUTest script runner (qutest.py) itself, in particular its fail-paths.
// It does NOT test any QP functionality.
//
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
//============================================================================
#if defined(Q_HOST) && !defined(_WIN32)
#define _POSIX_C_SOURCE 199309L // for nanosleep()
#endif
#include "qpc.h"          // QP/C framework

#include <stdlib.h>       // for exit()
#ifdef Q_HOST
#ifdef _WIN32
#include <windows.h>      // for Sleep()
#define SLEEP_MS(ms_)     Sleep((DWORD)(ms_))
#else
#include <time.h>         // for nanosleep()
static void SLEEP_MS(uint32_t ms) {
    struct timespec ts;
    ts.tv_sec  = (time_t)(ms / 1000U);
    ts.tv_nsec = (long)((ms % 1000U) * 1000000UL);
    nanosleep(&ts, (struct timespec *)0);
}
#endif
#endif // Q_HOST

Q_DEFINE_THIS_MODULE("selftest_fixture")

//----------------------------------------------------------------------------
// application-specific trace records
enum UserRec {
    ECHO = QS_USER,       // output requested by a command
    SETUP,                // produced by QS_onTestSetup() when armed
    TEARDOWN              // produced by QS_onTestTeardown() when armed
};

// commands (the numeric values are part of the fixture "API" used by
// the scripts; keep them stable)
enum CmdEnum {
    CMD_ECHO,             // emit one ECHO record: ECHO <param1>
    CMD_BURST,            // emit <param1> ECHO records: ECHO 0..param1-1
    CMD_SILENT,           // emit nothing (beyond the Trg-Ack)
    CMD_ASSERT,           // trigger an assertion with id <param1>
    CMD_DELAY_ECHO,       // wait <param1> ms, then emit ECHO <param2>
    CMD_EXIT,             // terminate the target process (host only)
    CMD_ARM_SETUP,        // QS_onTestSetup() emits SETUP <param1> (0=off)
    CMD_ARM_TEARDOWN,     // QS_onTestTeardown() emits TEARDOWN <param1>
    CMD_SET_VAR,          // g_var = param1; emit ECHO <param1>
    CMD_GET_VAR,          // emit ECHO <g_var>
    CMD_AO_ECHO           // emit ECHO <param1> with qs_id = prio of AO_Tst
};

static uint32_t l_setupArm;      // 0 means "not armed"
static uint32_t l_teardownArm;   // 0 means "not armed"
uint32_t g_var;                  // variable for peek/poke/fill tests

// test active object (for the ao_filter() tests) ...........................
// AO_Tst is started with prio 1 and handles no events. Its only purpose is
// to be the target of ao_filter(), which sets/clears the local filter
// for the AO's prio. CMD_AO_ECHO emits a record with qs_id == that prio,
// so the effect of ao_filter() is observable in the output.
typedef struct {
    QActive super;
} TstAO;
static TstAO l_tstAO;
QActive * const AO_Tst = &l_tstAO.super;
#define AO_TST_PRIO 1U

static QState TstAO_initial(TstAO * const me, void const * const par);
static QState TstAO_active(TstAO * const me, QEvt const * const e);

//----------------------------------------------------------------------------
#ifdef Q_HOST
int main(int argc, char *argv[]) { // host takes command-line arguments
    void * const qs_arg = ((argc > 1) ? argv[1] : (void*)0);
#else // embedded target
int main(void) {
    void * const qs_arg = (void*)0;
#endif

    QF_init(); // initialize the framework

    if (!QS_INIT(qs_arg)) { // initialize the QS software tracing
        Q_ERROR();
    }

    // dictionaries...
    QS_OBJ_DICTIONARY(&g_var);
    QS_OBJ_DICTIONARY(AO_Tst);
    QS_USR_DICTIONARY(ECHO);
    QS_USR_DICTIONARY(SETUP);
    QS_USR_DICTIONARY(TEARDOWN);

    QS_ENUM_DICTIONARY(CMD_ECHO,         QS_CMD);
    QS_ENUM_DICTIONARY(CMD_BURST,        QS_CMD);
    QS_ENUM_DICTIONARY(CMD_SILENT,       QS_CMD);
    QS_ENUM_DICTIONARY(CMD_ASSERT,       QS_CMD);
    QS_ENUM_DICTIONARY(CMD_DELAY_ECHO,   QS_CMD);
    QS_ENUM_DICTIONARY(CMD_EXIT,         QS_CMD);
    QS_ENUM_DICTIONARY(CMD_ARM_SETUP,    QS_CMD);
    QS_ENUM_DICTIONARY(CMD_ARM_TEARDOWN, QS_CMD);
    QS_ENUM_DICTIONARY(CMD_SET_VAR,      QS_CMD);
    QS_ENUM_DICTIONARY(CMD_GET_VAR,      QS_CMD);
    QS_ENUM_DICTIONARY(CMD_AO_ECHO,      QS_CMD);

    // the application-specific records of this fixture are always enabled,
    // so that the scripts don't depend on glb_filter() (tested separately)
    QS_GLB_FILTER(QS_GRP_UA);

    // start the test AO (its initial transition produces no output,
    // because only the QS_GRP_UA records are enabled)
    QActive_ctor(AO_Tst, Q_STATE_CAST(&TstAO_initial));
    static QEvtPtr l_tstAOQueueSto[2];
    QActive_start(AO_Tst,
        (QPrioSpec)AO_TST_PRIO,   // QF-prio.
        l_tstAOQueueSto,          // event queue storage
        Q_DIM(l_tstAOQueueSto),   // event queue length [events]
        (void *)0,                // no stack storage
        0U,                       // zero stack size [bytes]
        (void *)0);               // initialization param

    return QF_run(); // run the tests
}

//----------------------------------------------------------------------------
//............................................................................
static QState TstAO_initial(TstAO * const me, void const * const par) {
    Q_UNUSED_PAR(par);
    return Q_TRAN(&TstAO_active);
}
static QState TstAO_active(TstAO * const me, QEvt const * const e) {
    Q_UNUSED_PAR(e);
    return Q_SUPER(&QHsm_top);
}

//............................................................................
static void echo(uint32_t val) {
    QS_BEGIN_ID(ECHO, 0U)
        QS_U32(0, val);
    QS_END()
}

//............................................................................
void QS_onTestSetup(void) {
    if (l_setupArm != 0U) {
        QS_BEGIN_ID(SETUP, 0U)
            QS_U32(0, l_setupArm);
        QS_END()
    }
}
//............................................................................
void QS_onTestTeardown(void) {
    if (l_teardownArm != 0U) {
        QS_BEGIN_ID(TEARDOWN, 0U)
            QS_U32(0, l_teardownArm);
        QS_END()
    }
}

//............................................................................
void QS_onCommand(uint8_t cmdId,
                  uint32_t param1, uint32_t param2, uint32_t param3)
{
    Q_UNUSED_PAR(param3);
    switch (cmdId) {
        case CMD_ECHO:
            echo(param1);
            break;
        case CMD_BURST:
            for (uint32_t i = 0U; i < param1; ++i) {
                echo(i);
            }
            break;
        case CMD_SILENT:
            break;
        case CMD_ASSERT:
            Q_ERROR_ID((int_t)param1);
            break;
        case CMD_DELAY_ECHO:
#ifdef Q_HOST
            SLEEP_MS(param1);
#endif
            echo(param2);
            break;
        case CMD_EXIT:
#ifdef Q_HOST
            exit((int)param1);
#endif
            break;
        case CMD_ARM_SETUP:
            l_setupArm = param1;
            echo(param1);
            break;
        case CMD_ARM_TEARDOWN:
            l_teardownArm = param1;
            echo(param1);
            break;
        case CMD_SET_VAR:
            g_var = param1;
            echo(param1);
            break;
        case CMD_GET_VAR:
            echo(g_var);
            break;
        case CMD_AO_ECHO: // subject to the local filter of AO_Tst
            QS_BEGIN_ID(ECHO, AO_Tst->prio)
                QS_U32(0, param1);
            QS_END()
            break;
        default:
            Q_ERROR();
            break;
    }
}

//............................................................................
void QS_onTestEvt(QEvt *e) {
    Q_UNUSED_PAR(e);
}
//............................................................................
void QS_onTestPost(void const *sender, QActive *recipient,
                   QEvt const *e, bool status)
{
    Q_UNUSED_PAR(sender);
    Q_UNUSED_PAR(recipient);
    Q_UNUSED_PAR(e);
    Q_UNUSED_PAR(status);
}
