//////////////////////////////////////////////////////////////////////////////
// Product: QSPY thread class
// Last Updated for Version: 4.5.02
// Date of the Last Update:  Jul 08, 2012
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) 2002-2012 Quantum Leaps, LLC. All rights reserved.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Alternatively, this program may be distributed and modified under the
// terms of Quantum Leaps commercial licenses, which expressly supersede
// the GNU General Public License and are specifically designed for
// licensees interested in retaining the proprietary status of their code.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// Contact information:
// Quantum Leaps Web sites: http://www.quantum-leaps.com
//                          http://www.state-machine.com
// e-mail:                  info@quantum-leaps.com
//////////////////////////////////////////////////////////////////////////////
//#include <QtGui>
#include "qspy_app.h"
#include "qspy_thread.h"
#include <stdio.h>
//---
extern "C" {
    #include "hal.h"
}

//............................................................................
void QSPY_Thread::run() {
    QSPY_Event *qse = new QSPY_Event(QSPY_DATA_EVT);
    int n;

    /* process the QS data from the selected intput... */
    switch (m_link) {
        case NO_LINK:                         /* intentionally fall through */
        case SERIAL_LINK: {        /* input trace data from the serial port */
            if (!HAL_comOpen(comPort, baudRate)) {
                m_error = tr("Can't open serial port");
                return;
            }
            else {
                printf("\nSerial port %s opened, hit any key to quit...\n\n",
                       comPort);
            }
            while ((n = HAL_comRead(qse->m_payload,
                                    sizeof(qse->m_payload))) != -1)
            {
                if (n > 0) {
                    if (savFile != (FILE *)0) {
                        fwrite(qse->m_payload, 1, n, savFile);
                    }
                    qse->m_len = n;
                    QCoreApplication::postEvent(qApp, qse);
                    qse = new QSPY_Event(QSPY_DATA_EVT);
                }
            }
            HAL_comClose();
            break;
        }
        case FILE_LINK: {                   /* input trace data from a file */
            FILE *f = fopen(inpFileName, "rb");  /* open for reading binary */
            if (f == (FILE *)0) {
                m_error = tr("Input file not found\n");
                return;
            }
            do {
                n = fread(qse->m_payload, 1, (int)sizeof(qse->m_payload), f);
                if (savFile != (FILE *)0) {
                    fwrite(qse->m_payload, 1, n, savFile);
                }
                qse->m_len = n;
                QCoreApplication::postEvent(qApp, qse);
                qse = new QSPY_Event(QSPY_DATA_EVT);
            } while (n == sizeof(qse->m_payload));

            fclose(f);
            break;
        }
        case TCP_LINK: {              /* input trace data from the TCP port */
            if (!HAL_tcpOpen(tcpPort)) {
                m_error = tr("Can't open TCP/IP port");
                return;
            }
            while ((n = HAL_tcpRead(qse->m_payload,
                                    sizeof(qse->m_payload))) != -1)
            {
                if (n > 0) {
                    if (savFile != (FILE *)0) {
                        fwrite(qse->m_payload, 1, n, savFile);
                    }
                    qse->m_len = n;
                    QCoreApplication::postEvent(qApp, qse);
                    qse = new QSPY_Event(QSPY_DATA_EVT);
                }
            }
            HAL_tcpClose();
            break;
        }
    }

    if (savFile != (FILE *)0) {
        fclose(savFile);
    }
    if (l_outFile != (FILE *)0) {
        fclose(l_outFile);
    }
    //QSPY_stop();                       /* update and close all open files */
}
