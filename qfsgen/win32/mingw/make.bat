@echo off
:: ==========================================================================
:: Product: qfsgen buld script for Win32 port with GNU (MinGW)
:: Last Updated for Version: 4.3.01
:: Date of the Last Update:  Jan 12, 2012
::
::                    Q u a n t u m     L e a P s
::                    ---------------------------
::                    innovating embedded systems
::
:: Copyright (C) 2002-2012 Quantum Leaps, LLC. All rights reserved.
::
:: This software may be distributed and modified under the terms of the GNU
:: General Public License version 2 (GPL) as published by the Free Software
:: Foundation and appearing in the file GPL.TXT included in the packaging of
:: this file. Please note that GPL Section 2[b] requires that all works based
:: on this software must also be made publicly available under the terms of
:: the GPL ("Copyleft").
::
:: Alternatively, this software may be distributed and modified under the
:: terms of Quantum Leaps commercial licenses, which expressly supersede
:: the GPL and are specifically designed for licensees interested in
:: retaining the proprietary status of their code.
::
:: Contact information:
:: Quantum Leaps Web site:  http://www.quantum-leaps.com
:: e-mail:                  info@quantum-leaps.com
:: ==========================================================================
setlocal

:: NOTE:
:: -----
:: define the MINGW environment variable to point to the location 
:: where you've installed the MinGW toolset or adjust the following 
:: set instruction 
if "%MINGW%"=="" set MINGW=C:\tools\MinGW

set CC=%MINGW%\bin\g++
set LINK=%MINGW%\bin\g++
set LIBDIR=%MINGW%\lib

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