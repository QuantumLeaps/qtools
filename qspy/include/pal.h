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
* @date Last updated on: 2022-01-12
* @version Last updated for version: 7.0.0
*
* @file
* @brief Platform Abstraction Layer (PAL)
*/
#ifndef PAL_H_
#define PAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/* events for the QSPY event loop... */
typedef enum {
    QSPY_NO_EVT,
    QSPY_TARGET_INPUT_EVT,
    QSPY_FE_INPUT_EVT,
    QSPY_KEYBOARD_EVT,
    QSPY_DONE_EVT,
    QSPY_ERROR_EVT
} QSPYEvtType;

/* The PAL "virtual table" contains operations that are dependent
* on the choice of target connection. This connection is chosen
* by command-line options and is established by calling one of the
* PAL_openTarget???() functions
*/
typedef struct {
    QSPYEvtType (*getEvt)(unsigned char *buf, uint32_t *pBytes);
    QSpyStatus  (*send2Target)(unsigned char *buf, uint32_t nBytes);
    void (*cleanup)(void);
} PAL_VtblType;

extern PAL_VtblType PAL_vtbl;

/* typedefs needed for qpc_qs.h */
typedef int      int_t;
typedef int      enum_t;
typedef float    float32_t;
typedef double   float64_t;

QSpyStatus PAL_openBE(int portNum); /* open Back-End socket */
void PAL_closeBE(void);             /* close Back-End socket */
void PAL_send2FE(unsigned char const *buf, uint32_t nBytes); /* to Front-End */
void PAL_detachFE(void);            /* detach Front-End */
void PAL_clearScreen(void);

QSpyStatus PAL_openTargetSer(char const *comName, int baudRate);
QSpyStatus PAL_openTargetTcp(int portNum);
QSpyStatus PAL_openTargetFile(char const *fName);

QSpyStatus PAL_openKbd(bool kbd_inp, bool color);
void       PAL_closeKbd(void);

QSPYEvtType PAL_receiveBe (unsigned char *buf, uint32_t *pBytes);
QSPYEvtType PAL_receiveKbd(unsigned char *buf, uint32_t *pBytes);
void PAL_updateReadySet(int targetConn);

#ifdef __cplusplus
}
#endif

/* QSPY assertions... */
#ifdef Q_NASSERT /* Q_NASSERT defined--assertion checking disabled */
#define Q_ASSERT(test_)  ((void)0)
#else  /* assertion checking enabled */
#define Q_ASSERT(test_) ((test_) \
        ? (void)0 : Q_onAssert(__FILE__, __LINE__))

void Q_onAssert(char const* const module, int location);
#endif

#endif /* PAL_H_ */
