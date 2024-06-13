@echo off
:: ===========================================================================
:: Batch script for linting QP/C with PC-Lint-Plus2
:: Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
::
:: SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
::
:: This software is dual-licensed under the terms of the open source GNU
:: General Public License version 3 (or any later version), or alternatively,
:: under the terms of one of the closed source Quantum Leaps commercial
:: licenses.
::
:: The terms of the open source GNU General Public License version 3
:: can be found at: <www.gnu.org/licenses/gpl-3.0>
::
:: The terms of the closed source Quantum Leaps commercial licenses
:: can be found at: <www.state-machine.com/licensing>
::
:: Redistributions in source code must retain this top-level comment block.
:: Plagiarizing this software to sidestep the license obligations is illegal.
::
:: Contact information:
:: <www.state-machine.com>
:: <info@state-machine.com>
:: ===========================================================================
@setlocal

:: usage of make.bat
@echo Usage: make [-d...]
@echo examples:
@echo make -uQ_SPY -uQ_UTEST : undefine Q_SPY/Q_UTEST
@echo.

:: NOTE: adjust to for your installation directory of PC-Lint-Plus
@set PCLP=C:\tools\lint-plus2\windows\pclp64.exe

if NOT exist "%PCLP%" (
    @echo The PC-Lint-Plus toolset not found. Please adjust make.bat
    @goto end
)

set LINTFLAGS=-i64bit -i%GCC_INC% -dQSPY_APP options.lnt %1 %2 %3 %4

:: cleanup
@del *.log

:: linting -------------------------------------------------------------------
%PCLP% -os(lint.log) std.lnt %LINTFLAGS%  ..\source\*.c

:end
@endlocal
