/**
* @file
* @brief Back-End connection point for the external Front-Ends
* @ingroup qpspy
* @cond
******************************************************************************
* Last updated for version 6.8.4
* Last updated on  2021-11-03
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005-2020 Quantum Leaps, LLC. All rights reserved.
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
* along with this program. If not, see <www.gnu.org/licenses>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
******************************************************************************
* @endcond
*/
#ifndef be_h
#define be_h

#ifdef __cplusplus
extern "C" {
#endif

void BE_parse(unsigned char *buf, uint32_t nBytes);
void BE_parseRecFromFE    (QSpyRecord * const qrec);
int  BE_parseRecFromTarget(QSpyRecord * const qrec); /*see QSPY_CustParseFun*/

void BE_onStartup(void);
void BE_onCleanup(void);
void BE_sendLine(void);     /* send the QSPY parsed line to the Front-End */

#ifdef __cplusplus
}
#endif

#endif /* be_h */

