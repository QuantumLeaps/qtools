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
#ifndef PAL_H_
#define PAL_H_

#ifdef __cplusplus
extern "C" {
#endif

// events for the QSPY event loop...
typedef enum {
    QSPY_NO_EVT,
    QSPY_TARGET_INPUT_EVT,
    QSPY_FE_INPUT_EVT,
    QSPY_KEYBOARD_EVT,
    QSPY_DONE_EVT,
    QSPY_ERROR_EVT
} QSPYEvtType;

// The PAL "virtual table" contains operations that are dependent
// on the choice of target connection. This connection is chosen
// by command-line options and is established by calling one of the
// PAL_openTarget???() functions
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

QSpyStatus PAL_openBE(int portNum); // open Back-End socket
void PAL_closeBE(void);             // close Back-End socket
void PAL_send2FE(unsigned char const *buf, uint32_t nBytes); // to Front-End
void PAL_detachFE(void);            // detach Front-End
void PAL_clearScreen(void);

QSpyStatus PAL_openTargetSer(char const *comName, int baudRate);
QSpyStatus PAL_openTargetTcp(int portNum);
QSpyStatus PAL_openTargetFile(char const *fName);

QSpyStatus PAL_openKbd(bool kbd_inp, bool color);
void       PAL_closeKbd(void);
void       PAL_exit(void);

QSPYEvtType PAL_receiveBe (unsigned char *buf, uint32_t *pBytes);
QSPYEvtType PAL_receiveKbd(unsigned char *buf, uint32_t *pBytes);
void PAL_updateReadySet(int targetConn);

#ifdef __cplusplus
}
#endif

// QSPY assertions...
#ifdef Q_NASSERT // Q_NASSERT defined--assertion checking disabled
#define Q_ASSERT(test_)  ((void)0)
#else  // assertion checking enabled
#define Q_ASSERT(test_) ((test_) \
        ? (void)0 : Q_onError(__FILE__, __LINE__))

_Noreturn void Q_onError(char const * const module, int const id);
#endif

#endif // PAL_H_
