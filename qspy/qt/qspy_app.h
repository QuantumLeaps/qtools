//////////////////////////////////////////////////////////////////////////////
// Product: QSPY application class
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
#ifndef qspy_app_h
#define qspy_app_h

#include <QApplication>
#include <QEvent>

int const QSPY_DATA_EVT = QEvent::MaxUser;

//////////////////////////////////////////////////////////////////////////////
class QSPY_App : public QApplication {
    Q_OBJECT

public:
    QSPY_App(int &argc, char **argv);

protected:
    virtual bool event(QEvent *e);
};

//////////////////////////////////////////////////////////////////////////////
class QSPY_Event : public QEvent {
public:
    QSPY_Event(int type) : QEvent(static_cast<QEvent::Type>(type)) {
    }

private:
    quint8 m_payload[1024];
    int    m_len;

    friend class QSPY_App;
    friend class QSPY_Thread;
};

#endif                                                           // qspy_app_h
