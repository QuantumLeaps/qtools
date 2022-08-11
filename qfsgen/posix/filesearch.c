/**
* @file
* @brief QFSGen implementation for POSIX
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
