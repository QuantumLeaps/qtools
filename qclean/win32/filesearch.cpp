//////////////////////////////////////////////////////////////////////////////
// Product: QP utility
// Last Updated for Version: 1.1.00
// Date of the Last Update:  Mar 28, 2011
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) 2002-2011 Quantum Leaps, LLC. All rights reserved.
//
// This software may be distributed and modified under the terms of the GNU
// General Public License version 2 (GPL) as published by the Free Software
// Foundation and appearing in the file GPL.TXT included in the packaging of
// this file. Please note that GPL Section 2[b] requires that all works based
// on this software must also be made publicly available under the terms of
// the GPL ("Copyleft").
//
// Alternatively, this software may be distributed and modified under the
// terms of Quantum Leaps commercial licenses, which expressly supersede
// the GPL and are specifically designed for licensees interested in
// retaining the proprietary status of their code.
//
// Contact information:
// Quantum Leaps Web site:  http://www.quantum-leaps.com
// e-mail:                  info@quantum-leaps.com
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <io.h>
#include <direct.h>
#include <windows.h>
#include "filesearch.h"

//............................................................................
char const dir_separator = '\\';     // platform-dependent directory separator

//............................................................................
void filesearch(char const *dirname) {
    static char buffer[1024];
    struct _finddata_t fdata;
    long hnd;
    unsigned flags;

    _chdir(dirname);
    hnd = _findfirst("*", &fdata);         // set _findfirst to find everthing
    if (hnd == -1) {                 // if handle fails, directory is empty...
        return;
    }
    // get first entity on drive - check if it's a directory
    if ((::GetFileAttributes(fdata.name) & FILE_ATTRIBUTE_DIRECTORY) != 0
        && (::GetFileAttributes(fdata.name) & FILE_ATTRIBUTE_HIDDEN) == 0)
    {
                   // change to that directory and recursively call filesearch
        if (*fdata.name != '.') {
            filesearch(fdata.name);                 // call itself recursively
            _chdir("..");                    // go back up one directory level
        }
    }
    else {
                    // if it's not a directory and it matches what you want...
        flags = isMatching(fdata.name, (fdata.attrib & _A_RDONLY) != 0);
        if (flags != 0) {
            _getcwd(buffer,  sizeof(buffer));
            strcat(buffer, "\\");
            strcat(buffer, fdata.name);
            onMatchFound(buffer, flags);
        }
    }

    while (!(_findnext(hnd, &fdata))) {
        if ((::GetFileAttributes(fdata.name) & FILE_ATTRIBUTE_DIRECTORY) != 0
            && (::GetFileAttributes(fdata.name) & FILE_ATTRIBUTE_HIDDEN) == 0)
        {
            if (*fdata.name != '.') {
                filesearch(fdata.name);
                _chdir("..");
            }
        }
        else {
            flags = isMatching(fdata.name, (fdata.attrib & _A_RDONLY) != 0);
            if (flags != 0) {
                _getcwd(buffer,  sizeof(buffer));
                strcat(buffer, "\\");
                strcat(buffer, fdata.name);
                onMatchFound(buffer, flags);
            }
        }
    }
    _findclose(hnd);
}
