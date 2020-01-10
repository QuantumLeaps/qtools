/**
* @file
* @brief file search implementation for Windows
* @ingroup qfsgen
* @cond
******************************************************************************
* Last updated for version 6.7.0
* Last updated on  2020-12-03
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005-2020 Quantum Leaps, LLC. All rights reserved.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Alternatively, this program may be distributed and modified under the
* terms of Quantum Leaps commercial licenses, which expressly supersede
* the GNU General Public License and are specifically designed for
* licensees interested in retaining the proprietary status of their code.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <www.gnu.org/licenses/>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
******************************************************************************
* @endcond
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
