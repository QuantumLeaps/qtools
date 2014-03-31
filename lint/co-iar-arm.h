/*****************************************************************************
* Product: PC-Lint 9.x option file for IAR ARM compiler (6.xx)
* Last updated for version 5.3.0
* Last updated on  2014-02-22
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) Quantum Leaps, www.state-machine.com.
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
* Web:   www.state-machine.com
* Email: info@state-machine.com
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

/* defines and typedefs for <stdbool.h> ------------------------------------*/
typedef int _Bool;

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
