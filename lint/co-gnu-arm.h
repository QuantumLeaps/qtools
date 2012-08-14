/*****************************************************************************
* Product: PC-Lint 9.x option file for GNU ARM compiler
* Last Updated for Version: 4.4.00
* Date of the Last Update:  Jan 23, 2012
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2002-2008 Quantum Leaps, LLC. All rights reserved.
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
#ifndef co_gnu_arm_h
#define co_gnu_arm_h

#ifndef _lint                       /* Make sure no compiler comes this way */
#error "This header file is only for lint. Do not include it in your code!"
#endif

/*lint -save -w1 */

#ifdef __cplusplus
extern "C" {
#endif

/* extended keywords -------------------------------------------------------*/
/*lint +rw(inline)             */

/* extended keywords -------------------------------------------------------*/
/*lint +rw(inline)                   */
/*lint +rw(_up_to_brackets)          */
/*lint -d__asm=_up_to_brackets       */ /* __asm is now _up_to_brackets */
/*lint -esym(950, _up_to_brackets)   */

/*
* The headers included below must be generated; For C++, generate
* with:
*
* g++ [usual build options] -E -dM t.cpp >lint_cppmac.h
*
* For C, generate with:
*
* gcc [usual build options] -E -dM t.c >lint_cmac.h
*
* ...where "t.cpp" and "t.c" are empty source files.
*
* It's important to use the same compiler options used when compiling
* project code because they can affect the existence and precise
* definitions of certain predefined macros.  See gcc-readme.txt for
* details and a tutorial.
*/

#ifdef __cplusplus
    #include "lint_cppmac.h"
    /* DO NOT COMMENT THIS OUT. DO NOT SUPPRESS ERROR 322.
    * (If you see an error here, your Lint configuration is broken;
    * check -i options and ensure that you have generated lint_cppmac.h
    * as documented in gcc-readme.txt. Otherwise Gimpel Software cannot
    * support your configuration.)
    */
#else
    #include "lint_cmac.h"
    /* DO NOT COMMENT THIS OUT. DO NOT SUPPRESS ERROR 322.
    * (If you see an error here, your Lint configuration is broken;
    * check -i options and ensure that you have generated lint_cmac.h
    * as documented in gcc-readme.txt. Otherwise Gimpel Software cannot
    * support your configuration.)
    */
#endif

/* no support for 64-bit long long */
#define __have_long64            0
#define __int_fast64_t_defined   0

#ifdef __cplusplus
} /* extern "C" */
#endif

#if _lint >= 909 // For 9.00i and later:
    // __attribute__ is GCC's __attribute__:
    //
    //lint -rw_asgn(__attribute__,__gcc_attribute__)
    //lint -rw_asgn(__attribute,  __gcc_attribute__)
    //
    // Prevent "__attribute__" from being defined as a macro:
    //
    //lint --u"__attribute__"
    //lint --u"__attribute"
    //
    // Because an attribute-specifier is a form of
    // declaration-modifier, and because it can appear at the
    // beginning of a decl-specifier-seq, we must enable "Early
    // Modifiers":
    //
    //lint +fem
#else // for 9.00h and earlier:
    #define __attribute__()
    #define __attribute()
#endif

/*lint -restore */

#endif                                                      /* co_gnu_arm_h */
