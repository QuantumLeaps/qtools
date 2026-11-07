//============================================================================
// QP/C Real-Time Event Framework (RTEF)
//
// Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
//
//                    Q u a n t u m  L e a P s
//                    ------------------------
//                    Modern Embedded Software
//
// SPDX-License-Identifier: LicenseRef-QL-commercial
//
// This software is licensed under the terms of the Quantum Leaps commercial
// licenses. Please contact Quantum Leaps for more information about the
// available licensing options.
//
// RESTRICTIONS
// You may NOT :
// (a) redistribute, encumber, sell, rent, lease, sublicense, or otherwise
//     transfer rights in this software,
// (b) remove or alter any trademark, logo, copyright or other proprietary
//     notices, legends, symbols or labels present in this software,
// (c) plagiarize this software to sidestep the licensing obligations.
//
// Quantum Leaps contact information:
// <www.state-machine.com/licensing>
// <info@state-machine.com>
//============================================================================
#ifndef QS_PKG_H_
#define QS_PKG_H_

// NOTE: QSPY uses ONLY the QS protocol constants from the QP package
// header qs_pkg.h: the QS-RX record IDs and the framing constants below.
// They MUST be kept identical to qs_pkg.h/qs_pkg.hpp of the QP frameworks
// (qpc, qpcpp, safe-qpc, safe-qpcpp). The Target-side QS macros of
// qs_pkg.h are deliberately NOT part of this file (QSPY does not use them).

//----------------------------------------------------------------------------
//! QS received record types (RX channel)
typedef enum {
    QS_RX_INFO,           //!< query Target info (ver, config, tstamp)
    QS_RX_COMMAND,        //!< execute a user-defined command in the Target
    QS_RX_RESET,          //!< reset the Target
    QS_RX_TICK,           //!< call system clock tick in the Target
    QS_RX_PEEK,           //!< peek Target memory
    QS_RX_POKE,           //!< poke Target memory
    QS_RX_FILL,           //!< fill Target memory
    QS_RX_TEST_SETUP,     //!< test setup
    QS_RX_TEST_TEARDOWN,  //!< test teardown
    QS_RX_TEST_PROBE,     //!< set a Test-Probe in the Target
    QS_RX_GLB_FILTER,     //!< set global filters in the Target
    QS_RX_LOC_FILTER,     //!< set local  filters in the Target
    QS_RX_AO_FILTER,      //!< set local AO filter in the Target
    QS_RX_CURR_OBJ,       //!< set the "current-object" in the Target
    QS_RX_TEST_CONTINUE,  //!< continue a test after QS_TEST_PAUSE()
    QS_RX_QUERY_CURR,     //!< query the "current object" in the Target
    QS_RX_EVENT,          //!< inject an event to the Target
} QS_RxRecords;

#define QS_FRAME          ((uint8_t)0x7EU)
#define QS_ESC            ((uint8_t)0x7DU)
#define QS_ESC_XOR        ((uint8_t)0x20U)
#define QS_GOOD_CHKSUM    ((uint8_t)0xFFU)

#endif // QS_PKG_H_
