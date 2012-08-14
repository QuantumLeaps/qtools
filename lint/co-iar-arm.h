/*****************************************************************************
* Product: PC-Lint 9.x option file for IAR ARM compiler (6.xx)
* Last Updated for Version: 4.4.00
* Date of the Last Update:  Apr 19, 2012
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2002-2012 Quantum Leaps, LLC. All rights reserved.
*
* This software may be distributed and modified under the terms of the GNU
* General Public License version 2 (GPL) as published by the Free Software
* Foundation and appearing in the file GPL.TXT included in the packaging of
* this file. Please note that GPL Section 2[b] requires that all works based
* on this software must also be made publicly available under the terms of
* the GPL ("Copyleft").
*
* Alternatively, this software may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GPL and are specifically designed for licensees interested in
* retaining the proprietary status of their code.
*
* Contact information:
* Quantum Leaps Web site:  http://www.quantum-leaps.com
* e-mail:                  info@quantum-leaps.com
*****************************************************************************/
#ifndef co_iar_arm_h
#define co_iar_arm_h

#ifndef _lint                       /* Make sure no compiler comes this way */
#error "This header file is only for lint. Do not include it in your code!"
#endif

/*lint -save -w1 */

#ifdef __cplusplus

#define __ARRAY_OPERATORS    1

extern "C" {
#endif

/* compiler identification (might need adjustement in the future) ----------*/
#define __ICCARM__           1
#define __IAR_SYSTEMS_ICC__  8
#define __TID__              0xCF60
#define __VER__              6030000
#define __STDC__             1
#define __STDC_VERSION__     199901L

/* settings dependent on selected compiler options -------------------------*/
#define __CPU_MODE_THUMB__   1
#define __CPU_MODE_ARM__     2

#define __CORE__             __ARM7M__
#define __CPU_MODE__         __CPU_MODE_THUMB__
#define __DOUBLE__           64
#define __LITTLE_ENDIAN__    1
#define __embedded_cplusplus 1


/* machine-dependent sizes -------------------------------------------------*/
#define __FLOAT_SIZE__       4
#define __DOUBLE_SIZE__      8
#define __LONG_DOUBLE_SIZE__ 8
#define __INT_SIZE__         4

/* defines and typedefs for <stdint.h> -------------------------------------*/
#define __INT8_T_TYPE__      char
#define __UINT8_T_TYPE__     unsigned char
#define __INT16_T_TYPE__     short
#define __UINT16_T_TYPE__    unsigned short
#define __INT32_T_TYPE__     int
#define __UINT32_T_TYPE__    unsigned int

typedef __UINT16_T_TYPE__    __WCHAR_T_TYPE__;
typedef __INT32_T_TYPE__     __PTRDIFF_T_TYPE__;
typedef __UINT32_T_TYPE__    __SIZE_T_TYPE__;

typedef __INT8_T_TYPE__      __INT_LEAST8_T_TYPE__;
typedef __UINT8_T_TYPE__     __UINT_LEAST8_T_TYPE__;
typedef __INT16_T_TYPE__     __INT_LEAST16_T_TYPE__;
typedef __UINT16_T_TYPE__    __UINT_LEAST16_T_TYPE__;
typedef __INT32_T_TYPE__     __INT_LEAST32_T_TYPE__;
typedef __UINT32_T_TYPE__    __UINT_LEAST32_T_TYPE__;

typedef __INT8_T_TYPE__      __INT_FAST8_T_TYPE__;
typedef __UINT8_T_TYPE__     __UINT_FAST8_T_TYPE__;
typedef __INT16_T_TYPE__     __INT_FAST16_T_TYPE__;
typedef __UINT16_T_TYPE__    __UINT_FAST16_T_TYPE__;
typedef __INT32_T_TYPE__     __INT_FAST32_T_TYPE__;
typedef __UINT32_T_TYPE__    __UINT_FAST32_T_TYPE__;

typedef __INT32_T_TYPE__     __INTMAX_T_TYPE__;
typedef __UINT32_T_TYPE__    __UINTMAX_T_TYPE__;

typedef __INT32_T_TYPE__     __INTPTR_T_TYPE__;
typedef __UINT32_T_TYPE__    __UINTPTR_T_TYPE__;

#define __DATA_PTR_MEMORY_LIST1__()


/* extended keywords -------------------------------------------------------*/
/*lint +rw(inline)        */
/*lint +rw(__intrinsic)   */
/*lint +rw(__nounwind)    */

#define __constrange(min_, max_)
#define __asm(dummy_)


#ifdef __cplusplus
}                                                             /* extern "C" */
#endif

/*lint -restore */

#endif                                                      /* co_iar_arm_h */
