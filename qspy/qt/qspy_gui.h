//////////////////////////////////////////////////////////////////////////////
// Product: QSPY GUI class
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
#ifndef qspy_gui_h
#define qspy_gui_h

#include <QDialog>

class QLabel;
class QPushButton;

class QSPY_Gui : public QDialog {
    Q_OBJECT

public:
    explicit QSPY_Gui(QWidget *parent = 0);
    static QSPY_Gui *instance;

private slots:
    void onBtnPressed();
    void onBtnReleased();

protected:
    virtual void wheelEvent(QWheelEvent *e);
    //virtual void keyPressEvent(QKeyEvent *e);

public:
    QLabel      *m_background;
    QPushButton *m_button;

    QLabel      *m_scoreLbl;
    QLabel      *m_score;
};

#endif                                                           // qspy_gui_h
