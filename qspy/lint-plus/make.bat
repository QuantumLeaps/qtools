@echo off
:: ===========================================================================
:: Batch script for linting QP/C with PC-Lint-Plus2
:: Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
::
:: This software is licensed under the terms of the Quantum Leaps
:: QSPY SOFTWARE TRACING HOST UTILITY SOFTWARE END USER LICENSE.
:: Please see the file LICENSE-qspy.txt for the complete license text.
::
:: Quantum Leaps contact information:
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
%PCLP% -os(lint.log) std.lnt %LINTFLAGS%  ..\priv_src\*.c

:end
@endlocal
