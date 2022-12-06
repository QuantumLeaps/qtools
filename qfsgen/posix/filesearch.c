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
* @brief QFSGen implementation for POSIX
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdbool.h>

#include "safe_std.h" /* "safe" <stdio.h> and <string.h> facilities */
#include "qfsgen.h"

/*..........................................................................*/
char const dir_separator = '\\'; /* platform-dependent directory separator */

/*..........................................................................*/
void filesearch(char const *dname) {
    struct dirent *dent;
    DIR *dir;
    struct stat st;
    char fn[FILENAME_MAX];
    int len = strlen(dname);
    if (len >= FILENAME_MAX - 1) {
        return;
    }

    strcpy(fn, dname);
    fn[len++] = '/';
    if ((dir = opendir(dname)) == NULL) {
        return;
    }

    while ((dent = readdir(dir)) != NULL) {
        if ((dent->d_name[0] != '.')
             && (strcmp(dent->d_name, ".") != 0)
             && (strcmp(dent->d_name, "..") != 0))
        {
            strncpy(fn + len, dent->d_name, FILENAME_MAX - len);
            if (lstat(fn, &st) != -1) {
                /* don't follow symlink unless told so */
                if (S_ISLNK(st.st_mode)) {
                }
                else if (S_ISDIR(st.st_mode)) { /* false for symlinked dirs */
                    filesearch(fn); /* recursively follow dirs */
                }
                else {
                    int flags = isMatching(dent->d_name);
                    if ((flags & 0xFF) != 0) {
                        onMatchFound(fn, flags, -1); /* no read-only info */
                    }
                }
            }
        }
    }

    closedir(dir);
}
