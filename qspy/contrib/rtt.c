/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 1995 - 2019 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER RTT * Real Time Transfer for embedded targets         *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* SEGGER strongly recommends to not make any changes                 *
* to or modify the source code of this software in order to stay     *
* compatible with the RTT protocol and J-Link.                       *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* condition is met:                                                  *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this condition and the following disclaimer.             *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
---------------------------END-OF-HEADER------------------------------
File    : rtt.c
Purpose : Implementation of SEGGER real-time transfer (RTT) for QSPY.
Remarks : This is a stripped version of SEGGER_RTT.c (Rev: 25842) without
          all fancy stuff for "Terminal" and more channels.
                                                                  (JH)

Additional information:
          Type "int" is assumed to be 32-bits in size
          H->T    Host to target communication
          T->H    Target to host communication

          Only RTT channel 0 is present and reserved for QSPY usage.
          Name is fixed to RTT_QSPY_BUFFER_NAME.

          Effective buffer size: SizeOfBuffer - 1

          WrOff == RdOff:       Buffer is empty
          WrOff == (RdOff - 1): Buffer is full
          WrOff >  RdOff:       Free space includes wrap-around
          WrOff <  RdOff:       Used space includes wrap-around
          (WrOff == (SizeOfBuffer - 1)) && (RdOff == 0):  
                                Buffer full and wrap-around after next byte


----------------------------------------------------------------------
*/

#include "rtt.h"
#include <string.h>

#ifndef RTT_QSPY_BUFFER_NAME
  #define RTT_QSPY_BUFFER_NAME "qspy"
#endif

/*********************************************************************
*
*       Configuration, default values
*
**********************************************************************
*/

#if SEGGER_RTT_CPU_CACHE_LINE_SIZE
  #ifdef SEGGER_RTT_CB_ALIGN
    #error "Custom SEGGER_RTT_CB_ALIGN() is not supported for SEGGER_RTT_CPU_CACHE_LINE_SIZE != 0"
  #endif
  #ifdef SEGGER_RTT_BUFFER_ALIGN
    #error "Custom SEGGER_RTT_BUFFER_ALIGN() is not supported for SEGGER_RTT_CPU_CACHE_LINE_SIZE != 0"
  #endif
  #ifdef SEGGER_RTT_PUT_CB_SECTION
    #error "Custom SEGGER_RTT_PUT_CB_SECTION() is not supported for SEGGER_RTT_CPU_CACHE_LINE_SIZE != 0"
  #endif
  #ifdef SEGGER_RTT_PUT_BUFFER_SECTION
    #error "Custom SEGGER_RTT_PUT_BUFFER_SECTION() is not supported for SEGGER_RTT_CPU_CACHE_LINE_SIZE != 0"
  #endif
  #ifdef SEGGER_RTT_BUFFER_ALIGNMENT
    #error "Custom SEGGER_RTT_BUFFER_ALIGNMENT is not supported for SEGGER_RTT_CPU_CACHE_LINE_SIZE != 0"
  #endif
  #ifdef SEGGER_RTT_ALIGNMENT
    #error "Custom SEGGER_RTT_ALIGNMENT is not supported for SEGGER_RTT_CPU_CACHE_LINE_SIZE != 0"
  #endif
#endif

#ifndef   BUFFER_SIZE_UP
  #define BUFFER_SIZE_UP                                  1024  // Size of the buffer for terminal output of target, up to host
#endif

#ifndef   BUFFER_SIZE_DOWN
  #define BUFFER_SIZE_DOWN                                 16   // Size of the buffer for terminal input to target from host (Usually keyboard input)
#endif

#ifndef   SEGGER_RTT_MAX_NUM_UP_BUFFERS
  #define SEGGER_RTT_MAX_NUM_UP_BUFFERS                    2    // Number of up-buffers (T->H) available on this target
#endif

#ifndef   SEGGER_RTT_MAX_NUM_DOWN_BUFFERS
  #define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS                  2    // Number of down-buffers (H->T) available on this target
#endif

#ifndef SEGGER_RTT_BUFFER_SECTION
  #if defined(SEGGER_RTT_SECTION)
    #define SEGGER_RTT_BUFFER_SECTION SEGGER_RTT_SECTION
  #endif
#endif

#ifndef   SEGGER_RTT_ALIGNMENT
  #define SEGGER_RTT_ALIGNMENT                            SEGGER_RTT_CPU_CACHE_LINE_SIZE
#endif

#ifndef   SEGGER_RTT_BUFFER_ALIGNMENT
  #define SEGGER_RTT_BUFFER_ALIGNMENT                     SEGGER_RTT_CPU_CACHE_LINE_SIZE
#endif

#ifndef   SEGGER_RTT_MODE_DEFAULT
  #define SEGGER_RTT_MODE_DEFAULT                         SEGGER_RTT_MODE_NO_BLOCK_SKIP
#endif

//
// For some environments, NULL may not be defined until certain headers are included
//
#ifndef NULL
  #define NULL 0
#endif

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/
#if (defined __ICCARM__) || (defined __ICCRX__)
  #define RTT_PRAGMA(P) _Pragma(#P)
#endif

#if SEGGER_RTT_ALIGNMENT || SEGGER_RTT_BUFFER_ALIGNMENT
  #if ((defined __GNUC__) || (defined __clang__))
    #define SEGGER_RTT_ALIGN(Var, Alignment) Var __attribute__ ((aligned (Alignment)))
  #elif (defined __ICCARM__) || (defined __ICCRX__)
    #define PRAGMA(A) _Pragma(#A)
#define SEGGER_RTT_ALIGN(Var, Alignment) RTT_PRAGMA(data_alignment=Alignment) \
                                  Var
  #elif (defined __CC_ARM)
    #define SEGGER_RTT_ALIGN(Var, Alignment) Var __attribute__ ((aligned (Alignment)))
  #else
    #error "Alignment not supported for this compiler."
  #endif
#else
  #define SEGGER_RTT_ALIGN(Var, Alignment) Var
#endif

#if defined(SEGGER_RTT_SECTION) || defined (SEGGER_RTT_BUFFER_SECTION)
  #if ((defined __GNUC__) || (defined __clang__))
    #define SEGGER_RTT_PUT_SECTION(Var, Section) __attribute__ ((section (Section))) Var
  #elif (defined __ICCARM__) || (defined __ICCRX__)
#define SEGGER_RTT_PUT_SECTION(Var, Section) RTT_PRAGMA(location=Section) \
                                        Var
  #elif (defined __CC_ARM)
    #define SEGGER_RTT_PUT_SECTION(Var, Section) __attribute__ ((section (Section), zero_init))  Var
  #else
    #error "Section placement not supported for this compiler."
  #endif
#else
  #define SEGGER_RTT_PUT_SECTION(Var, Section) Var
#endif

#if SEGGER_RTT_ALIGNMENT
  #define SEGGER_RTT_CB_ALIGN(Var)  SEGGER_RTT_ALIGN(Var, SEGGER_RTT_ALIGNMENT)
#else
  #define SEGGER_RTT_CB_ALIGN(Var)  Var
#endif

#if SEGGER_RTT_BUFFER_ALIGNMENT
  #define SEGGER_RTT_BUFFER_ALIGN(Var)  SEGGER_RTT_ALIGN(Var, SEGGER_RTT_BUFFER_ALIGNMENT)
#else
  #define SEGGER_RTT_BUFFER_ALIGN(Var)  Var
#endif


#if defined(SEGGER_RTT_SECTION)
  #define SEGGER_RTT_PUT_CB_SECTION(Var) SEGGER_RTT_PUT_SECTION(Var, SEGGER_RTT_SECTION)
#else
  #define SEGGER_RTT_PUT_CB_SECTION(Var) Var
#endif

#if defined(SEGGER_RTT_BUFFER_SECTION)
  #define SEGGER_RTT_PUT_BUFFER_SECTION(Var) SEGGER_RTT_PUT_SECTION(Var, SEGGER_RTT_BUFFER_SECTION)
#else
  #define SEGGER_RTT_PUT_BUFFER_SECTION(Var) Var
#endif

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static char const * const _qspyBufferName = RTT_QSPY_BUFFER_NAME;

//
// RTT Control Block and allocate buffers for channel 0
//
#if SEGGER_RTT_CPU_CACHE_LINE_SIZE
  #if ((defined __GNUC__) || (defined __clang__))
    SEGGER_RTT_CB _SEGGER_RTT                                                             __attribute__ ((aligned (SEGGER_RTT_CPU_CACHE_LINE_SIZE)));
    static char   _qspyUpBuffer  [SEGGER_RTT__ROUND_UP_2_CACHE_LINE_SIZE(BUFFER_SIZE_UP)]   __attribute__ ((aligned (SEGGER_RTT_CPU_CACHE_LINE_SIZE)));
    static char   _qspyDownBuffer[SEGGER_RTT__ROUND_UP_2_CACHE_LINE_SIZE(BUFFER_SIZE_DOWN)] __attribute__ ((aligned (SEGGER_RTT_CPU_CACHE_LINE_SIZE)));
  #else
    #error "Don't know how to place _SEGGER_RTT, _acUpBuffer, _acDownBuffer cache-line aligned"
  #endif
#else
  SEGGER_RTT_PUT_CB_SECTION(SEGGER_RTT_CB_ALIGN(SEGGER_RTT_CB _SEGGER_RTT));
  SEGGER_RTT_PUT_BUFFER_SECTION(SEGGER_RTT_BUFFER_ALIGN(static char _qspyUpBuffer  [BUFFER_SIZE_UP]));
  SEGGER_RTT_PUT_BUFFER_SECTION(SEGGER_RTT_BUFFER_ALIGN(static char _qspyDownBuffer[BUFFER_SIZE_DOWN]));
#endif

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/

/*********************************************************************
*
*       _DoInit()
*
*  Function description
*    Initializes the control block an buffers.
*    May only be called via INIT() to avoid overriding settings.
*
*/
#define INIT()  {                                                                                    \
                  volatile SEGGER_RTT_CB* pRTTCBInit;                                                \
                  pRTTCBInit = (volatile SEGGER_RTT_CB*)((char*)&_SEGGER_RTT + SEGGER_RTT_UNCACHED_OFF); \
                  do {                                                                               \
                    if (pRTTCBInit->acID[0] == '\0') {                                               \
                      _DoInit();                                                                     \
                    }                                                                                \
                  } while (0);                                                                       \
                }

static void _DoInit(void) {
  volatile SEGGER_RTT_CB* p;   // Volatile to make sure that compiler cannot change the order of accesses to the control block
  static const char _aInitStr[] = "\0\0\0\0\0\0TTR REGGES";  // Init complete ID string to make sure that things also work if RTT is linked to a no-init memory area
  unsigned i;
  //
  // Initialize control block
  //
  p                     = (volatile SEGGER_RTT_CB*)((char*)&_SEGGER_RTT + SEGGER_RTT_UNCACHED_OFF);  // Access control block uncached so that nothing in the cache ever becomes dirty and all changes are visible in HW directly
  memset((SEGGER_RTT_CB*)p, 0, sizeof(_SEGGER_RTT));         // Make sure that the RTT CB is always zero initialized.
  p->MaxNumUpBuffers    = SEGGER_RTT_MAX_NUM_UP_BUFFERS;
  p->MaxNumDownBuffers  = SEGGER_RTT_MAX_NUM_DOWN_BUFFERS;
  //
  // Initialize up buffer 0
  //
  p->aUp[0].sName         = _qspyBufferName;
  p->aUp[0].pBuffer       = _qspyUpBuffer;
  p->aUp[0].SizeOfBuffer  = sizeof(_qspyUpBuffer);
  p->aUp[0].RdOff         = 0u;
  p->aUp[0].WrOff         = 0u;
  p->aUp[0].Flags         = SEGGER_RTT_MODE_NO_BLOCK_SKIP;
  //
  // Initialize down buffer 0
  //
  p->aDown[0].sName         = _qspyBufferName;
  p->aDown[0].pBuffer       = _qspyDownBuffer;
  p->aDown[0].SizeOfBuffer  = sizeof(_qspyDownBuffer);
  p->aDown[0].RdOff         = 0u;
  p->aDown[0].WrOff         = 0u;
  p->aDown[0].Flags         = SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL;
  //
  // Finish initialization of the control block.
  // Copy Id string backwards to make sure that "SEGGER RTT" is not found in initializer memory (usually flash),
  // as this would cause J-Link to "find" the control block at a wrong address.
  //
  RTT__DMB();                       // Force order of memory accesses for cores that may perform out-of-order memory accesses
  for (i = 0; i < sizeof(_aInitStr) - 1; ++i) {
    p->acID[i] = _aInitStr[sizeof(_aInitStr) - 2 - i];  // Skip terminating \0 at the end of the array
  }
  RTT__DMB();                       // Force order of memory accesses for cores that may perform out-of-order memory accesses
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/

/*
 * 'Local' defines just to save some typing
 */
#define pQSpyRingT2H	((SEGGER_RTT_BUFFER_UP*)((char*)_SEGGER_RTT.aUp + SEGGER_RTT_UNCACHED_OFF))
#define pQSpyRingH2T	((SEGGER_RTT_BUFFER_DOWN*)((char*)_SEGGER_RTT.aDown + SEGGER_RTT_UNCACHED_OFF))
/*
 * RTT initialization (needs to be called before any R/W
 */
void rtt_init(void) {
	INIT();
}
/**
 * Test for free space to send one byte of data to the host.
 */
int rtt_can_write(void) {
	unsigned WrOff = pQSpyRingT2H->WrOff + 1;
	if (WrOff == sizeof(_qspyUpBuffer)) {
		WrOff = 0;
	}
	return (WrOff != pQSpyRingT2H->RdOff);
}
/**
 * @note It is has to be called after rtt_can_write() only. It does not check
 *       free space again and just 'blindly' write.
 */
void rtt_putchar(const char c) {
	unsigned WrOff = pQSpyRingT2H->WrOff;
	if (WrOff == sizeof(_qspyUpBuffer)) {
		WrOff = 0;
	}
	volatile char * pDst = (pQSpyRingT2H->pBuffer + WrOff++) + SEGGER_RTT_UNCACHED_OFF;
	*pDst = c;
    RTT__DMB(); // Force data write to be complete before writing the <WrOff>
    pQSpyRingT2H->WrOff = WrOff;
}
/**
 * Test for availability of any data from the host
 */
int rtt_has_data(void) {
	return pQSpyRingH2T->RdOff != pQSpyRingH2T->WrOff;
}
/**
 * @note It is has to be called after rtt_has_data() only. It does not check
 *       availability of any data again.
 * @return
 */
char rtt_getchar(void) {
	unsigned RdOff = pQSpyRingH2T->RdOff;
	char c = *(pQSpyRingH2T->pBuffer + RdOff + SEGGER_RTT_UNCACHED_OFF);

	RdOff++;
    if (RdOff == sizeof(_qspyDownBuffer)) {
    	RdOff = 0u;
    }
    pQSpyRingH2T->RdOff = RdOff;
    return c;
}

#undef pQSpyRingH2T
#undef pQSpyRingT2H

/*************************** End of file ****************************/
