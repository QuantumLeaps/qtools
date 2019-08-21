/**
* @file
* @brief QClean internal interface
* @ingroup qclean
* @cond
******************************************************************************
* Last updated for version 6.6.0
* Last updated on  2019-07-30
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005-2019 Quantum Leaps, LLC. All rights reserved.
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
* www.state-machine.com
* info@state-machine.com
******************************************************************************
* @endcond
*/
#ifndef qclean_h
#define qclean_h

#define VERSION "6.6.0"

unsigned isMatching  (char const *fullPath);
void     onMatchFound(char const *fullPath, unsigned flags, int ro_info);
void     filesearch  (char const *dirname);

extern char const dir_separator; /* platform-dependent directory separator */

#endif /* qclean_h */
