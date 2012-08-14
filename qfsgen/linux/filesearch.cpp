//////////////////////////////////////////////////////////////////////////////
// Product: filesarch utility
// Last Updated for Version: 4.1.07
// Date of the Last Update:  Mar 29, 2011
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
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include "filesearch.h"

//............................................................................
char const dir_separator = '/';                // directory separator for UNIX

//............................................................................
void filesearch(char const *dirname) {
    static char buffer[1024];

    DIR *dir = opendir(dirname);                 // open the current directory
    if (dir == (DIR *)0) {            // if open fails, the directory is empty
        return;
    }

    struct dirent *entry;
    while((entry = readdir(dir)) != (struct dirent *)0) {
        if ((strncmp(entry->d_name, "..", 2) != 0)
            && (strncmp(entry->d_name, ".",  1) != 0))
        {
            strncpy(buffer, dirname,       sizeof(buffer));
            strncat(buffer, "/",           sizeof(buffer));
            strncat(buffer, entry->d_name, sizeof(buffer));

            if (entry->d_type == DT_DIR) {               // is it a directory?
                filesearch(buffer);
            }
            else {
                int flags = isMatching(entry->d_name,
                                       access(buffer, W_OK) != 0);
                if (flags != 0) {
                    onMatchFound(buffer, flags);
                }
            }
        }
    }
    closedir(dir);
}

