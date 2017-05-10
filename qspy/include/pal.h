/**
* @file
* @brief Platform Abstraction Layer (PAL)
* @ingroup qpspy
* @cond
******************************************************************************
* Last updated for version 5.9.0
* Last updated on  2017-04-04
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) Quantum Leaps, LLC. All rights reserved.
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
#ifndef pal_h
#define pal_h

#ifdef __cplusplus
extern "C" {
#endif

QSpyStatus PAL_openBE(int portNum); /* open Back-End socket */
void PAL_closeBE(void);             /* close Back-End socket */
void PAL_send2FE(unsigned char const *buf, size_t nBytes); /* to Front-End*/
void PAL_detachFE(void);            /* detach Front-End */

void PAL_clearScreen(void);

QSpyStatus PAL_openTargetSer(char const *comName, int baudRate);
QSpyStatus PAL_openTargetTcp(int portNum);
QSpyStatus PAL_openTargetFile(char const *fName);

/* events for the QSPY event loop... */
typedef enum {
    QSPY_NO_EVT,
    QSPY_TARGET_INPUT_EVT,
    QSPY_FE_INPUT_EVT,
    QSPY_KEYBOARD_EVT,
    QSPY_DONE_EVT,
    QSPY_ERROR_EVT
} QSPYEvtType;

#define QS_FRAME            ((uint8_t)0x7E)
#define QS_ESC              ((uint8_t)0x7D)
#define QS_GOOD_CHKSUM      ((uint8_t)0xFF)
#define QS_ESC_XOR          ((uint8_t)0x20)

/* The PAL "virtual table" contains operations that are dependent
* on the choice of target connection. This connection is chosen
* by command-line options and is established by calling one of the
* PAL_openTarget???() functions
*/
typedef struct {
    QSPYEvtType (*getEvt)(unsigned char *buf, size_t *pBytes);
    QSpyStatus  (*send2Target)(unsigned char *buf, size_t nBytes);
    void (*cleanup)(void);
} PAL_VtblType;

extern PAL_VtblType PAL_vtbl;

/* QSPY assertions... */
#ifdef Q_NASSERT /* Q_NASSERT defined--assertion checking disabled */
    #define Q_ASSERT(test_)             ((void)0)
#else  /* assertion checking enabled */
    #define Q_ASSERT(test_) ((test_) \
        ? (void)0 : Q_onAssert(__FILE__, __LINE__))

    void Q_onAssert(char const * const file, int line);
#endif

#ifdef __cplusplus
}
#endif

/* portable "safe" facilities from <stdio.h> and <string.h> ................*/
#ifdef _MSC_VER /* Microsoft C/C++ compiler? */

#define SNPRINTF_S(buf_, len_, format_, ...) \
    _snprintf_s(buf_, len_, _TRUNCATE, format_, ##__VA_ARGS__)

#define STRNCPY_S(dest_, src_, len_) \
    strncpy_s(dest_, len_, src_, _TRUNCATE)

#define FOPEN_S(fp_, fName_, mode_) \
if (fopen_s(&fp_, fName_, mode_) != 0) { \
    fp_ = (FILE *)0; \
} else (void)0

#define LOCALTIME_S(tm_, time_) \
    localtime_s(tm_, time_)

#define SSCANF_S(buf_, format_, ...) \
    sscanf_s(buf_, format_, ##__VA_ARGS__)

#else /* other C/C++ compilers (GNU, etc.) */

#define SNPRINTF_S(buf_, len_, format_, ...) \
    snprintf(buf_, len_, format_, ##__VA_ARGS__)

#define STRNCPY_S(dest_, src_, len_) strncpy(dest_, src_, len_)

#define FOPEN_S(fp_, fName_, mode_) \
    (fp_ = fopen(fName_, mode_))

#define LOCALTIME_S(tm_, time_) \
    memcpy(tm_, localtime(time_), sizeof(struct tm))

#define SSCANF_S(buf_, format_, ...) \
    sscanf(buf_, format_, ##__VA_ARGS__)

#endif /* _MSC_VER */

#endif /* pal_h */
