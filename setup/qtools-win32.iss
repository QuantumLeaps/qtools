;-----------------------------------------------------------------------------
; Product: QTools installation script for Windows
; Last Updated for Version: 6.6.0
; Date of the Last Update:  2019-08-21
;
;                    Q u a n t u m  L e a P s
;                    ------------------------
;                    Modern Embedded Software
;
; Copyright (C) 2005-2019 Quantum Leaps, LLC. All rights reserved.
;
; Contact information:
; www.state-machine.com
; info@state-machine.com
;-----------------------------------------------------------------------------

;.............................................................................
[Setup]
PrivilegesRequiredOverridesAllowed=dialog
AppName=QTools
AppVersion=6.6.0
AppVerName=QTools 6.6.0
OutputBaseFilename=qtools-win32_6.6.0
AppPublisher=Quantum Leaps, LLC
AppPublisherURL=https://www.state-machine.com/
AppCopyright=Copyright (C) 2005-2019 Quantum Leaps, LLC. All rights reserved.
AppReadmeFile=https://www.state-machine.com/qtools/
AppSupportURL=https://www.state-machine.com/qtools/
AppUpdatesURL=https://www.state-machine.com/qtools/
LicenseFile=..\LICENSE.md
DefaultDirName={sd}\qp\qtools
DisableWelcomePage=no
DisableDirPage=no
UsePreviousAppDir=yes
SetupIconFile=qp_install.ico
UninstallDisplayIcon=qp_uninstall.ico
OutputDir=.
WizardStyle=modern
WizardImageFile=qtools_install.bmp
WizardSmallImageFile=qtools_install-small.bmp
DisableProgramGroupPage=yes
SolidCompression=yes
ChangesEnvironment=yes

;.............................................................................
[Tasks]
Name: modifypath; Description: "&Add <qtools>\bin directory to your system PATH"; Flags: checkablealone; Check: IsAdminInstallMode


;.............................................................................
[Registry]
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType:string; ValueName: "QTOOLS"; \
    ValueData: "{app}"; Flags: preservestringtype uninsdeletevalue; Check: IsAdminInstallMode

;.............................................................................
[Messages]
; *** Application titles
SetupAppTitle=Setup
SetupWindowTitle=QTools 6.6.0 Setup
UninstallAppTitle=Uninstall
UninstallAppFullTitle=QTools 6.6.0 Uninstall

; *** "Welcome" wizard page
WelcomeLabel1=Thank you for downloading%nQTools 6.6.0 from Quantum Leaps (state-machine.com)
WelcomeLabel2=QTools™ is a is a collection of open source desktop tools for the QP™ family of real-time embedded frameworks (RTEFs).%n%nThis setup will install QTools 6.6.0 on your computer.
ClickNext=Click Next to continue, or Cancel to exit setup.

; *** "License Agreement" wizard page
WizardLicense=License Agreement
LicenseLabel=Please read the following End User License Agreement before continuing.
LicenseLabel3=By installing the software, you accept the license terms below. If you do not accept them, press Cancel now and do NOT install the software.
LicenseAccepted=I &accept the agreement
LicenseNotAccepted=I do &not accept the agreement

;.............................................................................
[Types]
Name: "full";   Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

;.............................................................................
[Components]
Name: "MinGW";   Description: "GNU C/C++ toolchain for Windows (MinGW)"; Types: full custom
Name: "GNUARM";  Description: "GNU C/C++ toolchain for ARM Cortex-M/R";  Types: full custom
Name: "Unity";   Description: "Unity unit testing harness";              Types: full custom
Name: "Python";  Description: "Python";                                  Types: full custom
Name: "TclTk";   Description: "Tcl/Tk"

;.............................................................................
[Files]
Source: "..\README.md";          DestDir: "{app}"; Flags: isreadme
Source: "..\LICENSE.md";         DestDir: "{app}";
Source: "..\README.url";         DestDir: "{app}";
Source: "..\version*";           DestDir: "{app}";

Source: "..\doc\*";              DestDir: "{app}\doc"
Source: "..\licenses\*";         DestDir: "{app}\licenses"
Source: "..\lint\*";             DestDir: "{app}\lint"
Source: "..\matlab\*";           DestDir: "{app}\matlab"
Source: "..\mscgen\*";           DestDir: "{app}\mscgen"
Source: "..\source\*";           DestDir: "{app}\source"
Source: "..\qclean\*";           DestDir: "{app}\qclean"; Flags: recursesubdirs
Source: "..\qfsgen\*";           DestDir: "{app}\qfsgen"; Flags: recursesubdirs
Source: "..\qspy\*";             DestDir: "{app}\qspy";   Flags: recursesubdirs; Excludes: "tcl,*.tcl"
Source: "..\qwin\*";             DestDir: "{app}\qwin";   Flags: recursesubdirs

; bin directory
Source: "..\bin\*.dll";          DestDir: "{app}\bin";
Source: "..\bin\*.exe";          DestDir: "{app}\bin";

; MinGW
Source: "\tools\MinGW\*";        DestDir: "{app}\MinGW"; Flags: recursesubdirs; Components: MinGW

; GNUARM
Source: "\tools\gnu_arm-none-eabi\*";   DestDir: "{app}\gnu_arm-none-eabi"; Flags: recursesubdirs; Components: GNUARM

; Unity
Source: "\tools\Unity\*";        DestDir: "{app}\Unity"; Flags: recursesubdirs; Components: Unity

; Python
Source: "\tools\Python37\*";     DestDir: "{app}\Python37"; Flags: recursesubdirs; Components: Python
Source: "..\bin\python.bat";     DestDir: "{app}\bin"; Components: Python

; Tcl/Tk
Source: "\tools\tcl_8.6\*";      DestDir: "{app}\tcl_8.6"; Flags: recursesubdirs; Components: TclTk
Source: "..\bin\tclsh.bat";      DestDir: "{app}\bin";      Components: TclTk
Source: "..\bin\wish.bat";       DestDir: "{app}\bin";      Components: TclTk
Source: "..\bin\*.tcl";          DestDir: "{app}\bin";      Components: TclTk
Source: "..\qspy\tcl\*";         DestDir: "{app}\qspy\tcl"; Components: TclTk

;.............................................................................
[Code]
const
	ModPathName = 'modifypath';
	ModPathType = 'system';

function ModPathDir(): TArrayOfString;
begin
	setArrayLength(Result, 2)
	Result[0] := ExpandConstant('{app}\bin');
	Result[1] := ExpandConstant('{app}\MinGW\bin');
end;
#include "C:\tools\Inno-Setup\modpath.iss"

