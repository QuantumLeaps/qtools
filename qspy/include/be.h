/**
* @file
* @brief Back-End connection point for the external Front-Ends
* @ingroup qpspy
* @cond
******************************************************************************
* Last updated for version 5.9.0
* Last updated on  2017-05-12
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
#ifndef be_h
#define be_h

#ifdef __cplusplus
extern "C" {
#endif

void BE_parse(unsigned char *buf, size_t nBytes);
void BE_parseRecFromFE    (QSpyRecord * const qrec);
int  BE_parseRecFromTarget(QSpyRecord * const qrec); /*see QSPY_CustParseFun*/

void BE_onStartup(void);
void BE_onCleanup(void);

void BE_sendPkt(int pktId); /* send the packet to the Front-End */
void BE_sendLine(void);     /* send the QSPY parsed line to the Front-End */

void BE_putU8(uint8_t d);
void BE_putU16(uint16_t d);
void BE_putU32(uint32_t d);
void BE_putStr(char const *str);
void BE_putMem(uint8_t const *mem, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif /* be_h */

