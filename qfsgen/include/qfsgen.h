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
* @date Last updated on: 2022-12-03
* @version Last updated for: @ref qtools_7_1_3
*
* @file
* @brief QFSgen ROM file system generator internal interface
*/
#ifndef QFSGEN_H_
#define QFSGEN_H_

#define VERSION "6.9.0"

unsigned isMatching  (char const *fullPath);
void     onMatchFound(char const *fullPath, unsigned flags, int ro_info);
void     filesearch  (char const *dirname);

extern char const dir_separator; /* platform-dependent directory separator */

#endif /* QFSGEN_H_ */
