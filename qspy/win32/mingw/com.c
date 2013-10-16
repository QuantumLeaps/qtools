/*****************************************************************************
* Product: Quantum Spy -- Serial communication HAL
* Last Updated for Version: 4.5.04
* Date of the Last Update:  Jan 31, 2013
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2002-2013 Quantum Leaps, LLC. All rights reserved.
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
* Quantum Leaps Web sites: http://www.quantum-leaps.com
*                          http://www.state-machine.com
* e-mail:                  info@quantum-leaps.com
*****************************************************************************/
#include <string.h>                                           /* for size_t */
#include <stdint.h>
#include <windows.h>         /* for Windows serial communication facilities */
#include <stdio.h>
#include <conio.h>

#include "hal.h"
#include "qspy.h"

/*..........................................................................*/
static HANDLE l_com;
static COMMTIMEOUTS l_timeouts;

#define BUF_SIZE 1024
#define COM_SETTINGS \
    "baud=%d parity=N data=8 stop=1 odsr=off dtr=on octs=off rts=on"

/*..........................................................................*/
int HAL_comOpen(char const *comName, int baudRate) {
    DCB dcb = {0};
    char comPortName[40];
    char comSettings[120];

    l_com = 0;
    snprintf(comPortName, sizeof(comPortName),
             "\\\\.\\%s", comName);
    snprintf(comSettings, sizeof(comSettings),
             COM_SETTINGS,
             baudRate);

    l_com = CreateFile(comPortName,
                       GENERIC_READ | GENERIC_WRITE,
                       0,                               /* exclusive access */
                       NULL,                           /* no security attrs */
                       OPEN_EXISTING,
                       0,                  /* standard (not-overlapped) I/O */
                       NULL);

    if (l_com == INVALID_HANDLE_VALUE) {
        printf("Error by opening the COM port: %s at the baud rate %d\n",
               comName, baudRate);
        return 0;
    }

    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(l_com, &dcb)) {
        printf("Error by retreiving port settings\n");
        return 0;
    }

    dcb.fAbortOnError = 0;                          /* don't abort on error */
    /* Fill in the DCB */
    if (!BuildCommDCB(comSettings, &dcb)) {
        printf("Error by parsing command line parameters\n");
        return 0;
    }

    if (!SetCommState(l_com, &dcb)) {
        printf("Error setting up the port\n");
        return 0;
    }

    SetupComm(l_com, BUF_SIZE, BUF_SIZE);     /* setup the device buffers */

    /* purge any information in the buffers */
    PurgeComm(l_com, PURGE_TXABORT | PURGE_RXABORT |
                     PURGE_TXCLEAR | PURGE_RXCLEAR);

    /* the timeouts for the serial communication are set accorging to the
    * following remark from the Win32 help documentation:
    *
    * If an application sets ReadIntervalTimeout and
    * ReadTotalTimeoutMultiplier to MAXDWORD and sets
    * ReadTotalTimeoutConstant to a value greater than zero and less than
    * MAXDWORD, one of the following occurs when the ReadFile function
    * is called:
    * 1. If there are any characters in the input buffer, ReadFile
    * returns immediately with the characters in the buffer.
    * 2. If there are no characters in the input buffer, ReadFile waits
    * until a character arrives and then returns immediately.
    * 3. If no character arrives within the time specified by
    * ReadTotalTimeoutConstant, ReadFile times out.
    */
    l_timeouts.ReadIntervalTimeout = MAXDWORD;
    l_timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    l_timeouts.ReadTotalTimeoutConstant = 50;
    l_timeouts.WriteTotalTimeoutMultiplier = 0;
    l_timeouts.WriteTotalTimeoutConstant = 50;
    SetCommTimeouts(l_com, &l_timeouts);

    return 1;                                                    /* success */
}
/*..........................................................................*/
void HAL_comClose(void) {
    CloseHandle(l_com);
}
/*..........................................................................*/
int HAL_comRead(unsigned char *buf, size_t size) {
    DWORD n;
    if (!ReadFile(l_com, buf, (DWORD)size, &n, NULL)) {
        COMSTAT comstat;
        DWORD errors;

        printf("*** Reading serial port failed; GetLastError=%ld\n",
               GetLastError());
        ClearCommError(l_com, &errors, &comstat);
    }
    if (_kbhit()) {
        return -1;                                             /* terminate */
    }
    return (int)n;
}
