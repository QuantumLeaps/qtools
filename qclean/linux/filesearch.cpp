//////////////////////////////////////////////////////////////////////////////
// Product: filesarch utility
// Last Updated for Version: 4.3.00
// Date of the Last Update:  Nov 03, 2011
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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>

#include "filesearch.h"

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
                else if (S_ISDIR(st.st_mode)) {    // false for symlinked dirs
                    filesearch(fn);                 // recursively follow dirs
                }
                else {
                    int flags = isMatching(dent->d_name,
                                           access(fn, W_OK) != 0);
                    if (flags != 0) {
                        onMatchFound(fn, flags);
                    }
                }
            }
        }
    }

    closedir(dir);
}

