@echo off
:: ==========================================================================
:: Product: qfsgen buld script for Win32 port with GNU (MinGW)
:: Last Updated for Version: 4.5.04
:: Date of the Last Update:  Feb 21, 2013
::
::                    Q u a n t u m     L e a P s
::                    ---------------------------
::                    innovating embedded systems
::
:: Copyright (C) 2002-2012 Quantum Leaps, LLC. All rights reserved.
::
:: This program is open source software: you can redistribute it and/or
:: modify it under the terms of the GNU General Public License as published
:: by the Free Software Foundation, either version 3 of the License, or
:: (at your option) any later version.
::
:: Alternatively, this program may be distributed and modified under the
:: terms of Quantum Leaps commercial licenses, which expressly supersede
:: the GNU General Public License and are specifically designed for
:: licensees interested in retaining the proprietary status of their code.
::
:: This program is distributed in the hope that it will be useful,
:: but WITHOUT ANY WARRANTY; without even the implied warranty of
:: MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
:: GNU General Public License for more details.
::
:: You should have received a copy of the GNU General Public License
:: along with this program. If not, see <http://www.gnu.org/licenses/>.
::
:: Contact information:
:: Quantum Leaps Web sites: http://www.quantum-leaps.com
::                          http://www.state-machine.com
:: e-mail:                  info@quantum-leaps.com
:: ==========================================================================
setlocal

:: NOTE:
:: -----
:: This Makefile assumes that the MinGW compiler is in the PATH.
:: The Qtools collection for Windows now includes the MinGW compiler,
:: so if you have added the Qtools\bin\ directory to your PATH,
:: you should be covered.

set CC=gcc
set LINK=gcc

:: ==========================================================================
if "%1"=="" (
    echo default selected
    set BINDIR=rel
    set CCFLAGS=-O2 -c -Wall 
)
if "%1"=="dbg" (
    echo dbg selected
    set BINDIR=dbg
    set CCFLAGS=-g -c -Wall 
)

set LINKFLAGS=-static-libgcc

:: QSPY ---------------------------------------------------------------------
set SRCDIR=..\..\source
set CCINC=-I..\..\include

@echo on
%CC% %CCFLAGS% %CCINC% -o%BINDIR%\main.o       %SRCDIR%\main.cpp
%CC% %CCFLAGS% %CCINC% -o%BINDIR%\filesearch.o ..\filesearch.cpp

%LINK% %LINKFLAGS% -o %BINDIR%\qfsgen.exe %BINDIR%\main.o %BINDIR%\filesearch.o

@echo off
erase %BINDIR%\*.o

:end


endlocal