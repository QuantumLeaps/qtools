/*============================================================================
* QP/C Real-Time Embedded Framework (RTEF)
* Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
*
* SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
*
* This software is dual-licensed under the terms of the open source GNU
* General Public License version 3 (or any later version), or alternatively,
* under the terms of one of the closed source Quantum Leaps commercial
* licenses.
*
* The terms of the open source GNU General Public License version 3
* can be found at: <www.gnu.org/licenses/gpl-3.0>
*
* The terms of the closed source Quantum Leaps commercial licenses
* can be found at: <www.state-machine.com/licensing>
*
* Redistributions in source code must retain this top-level comment block.
* Plagiarizing this software to sidestep the license obligations is illegal.
*
* Contact information:
* <www.state-machine.com>
* <info@state-machine.com>
============================================================================*/
/*!
* @date Last updated on: 2022-12-03
* @version Last updated for: @ref qtools_7_1_3
*
* @file
* @brief file search implementation for Windows
*/
#include <stdio.h>
#include <stdbool.h>
#include <io.h>
#include <direct.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>  /* Win32 API */

#include "safe_std.h" /* "safe" <stdio.h> and <string.h> facilities */
#include "qfsgen.h"

/*..........................................................................*/
char const dir_separator = '\\'; /* platform-dependent directory separator */

/*..........................................................................*/
void filesearch(char const *dirname) {
    static char buffer[1024];
    struct _finddata_t fdata;
    long hnd;
    unsigned flags;

    _chdir(dirname);
    hnd = _findfirst("*", &fdata);  /* set _findfirst to find everthing */
    if (hnd == -1) { /* if handle fails, directory is empty... */
        return;
    }
    /* get first entity on drive - check if it's a directory */
    if ((GetFileAttributes(fdata.name) & FILE_ATTRIBUTE_DIRECTORY) != 0
        && (GetFileAttributes(fdata.name) & FILE_ATTRIBUTE_HIDDEN) == 0)
    {
        /* change to that directory and recursively call filesearch */
        if (*fdata.name != '.') {
            filesearch(fdata.name); /* call itself recursively */
            _chdir(".."); /* go back up one directory level */
        }
    }
    else {
        /* if it's not a directory and it matches what you want... */
        flags = isMatching(fdata.name);
        if (flags != 0) {
            _getcwd(buffer,  sizeof(buffer));
            strcat(buffer, "\\");
            strcat(buffer, fdata.name);
            onMatchFound(buffer, flags,
                         (fdata.attrib & _A_RDONLY) != 0 ? 1 : 0);
        }
    }

    while (!(_findnext(hnd, &fdata))) {
        if ((GetFileAttributes(fdata.name) & FILE_ATTRIBUTE_DIRECTORY) != 0
            && (GetFileAttributes(fdata.name) & FILE_ATTRIBUTE_HIDDEN) == 0)
        {
            if (*fdata.name != '.') {
                filesearch(fdata.name);
                _chdir("..");
            }
        }
        else {
            flags = isMatching(fdata.name);
            if (flags != 0) {
                _getcwd(buffer, sizeof(buffer));
                strcat(buffer, "\\");
                strcat(buffer, fdata.name);
                onMatchFound(buffer, flags,
                             (fdata.attrib & _A_RDONLY) != 0 ? 1 : 0);
            }
        }
    }
    _findclose(hnd);
}
