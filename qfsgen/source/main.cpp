//////////////////////////////////////////////////////////////////////////////
// Product: qfsgen utility
// Last Updated for Version: 5.1.1
// Date of the Last Update:  Oct 15, 2013
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) 2002-2013 Quantum Leaps, LLC. All rights reserved.
//
// This program is open source software: you can redistribute it and/or
// modify it under the terms of the GNU General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Alternatively, this program may be distributed and modified under the
// terms of Quantum Leaps commercial licenses, which expressly supersede
// the GNU General Public License and are specifically designed for
// licensees interested in retaining the proprietary status of their code.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//
// Contact information:
// Quantum Leaps Web sites: http://www.quantum-leaps.com
//                          http://www.state-machine.com
// e-mail:                  info@quantum-leaps.com
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include "filesearch.h"

static char const *l_rootDir = "fs";
static FILE *l_file;
static bool  l_noHtml = false;
static int   l_nFiles;
static char  l_prevFile[256];

//............................................................................
static void dumpStrHex(char const *s) {
    int i = 0;
    while (*s != '\0') {
        if (i == 0) {
            fprintf(l_file, "    ");
        }
        fprintf(l_file, "0x%02X, ", ((unsigned)*s & 0xFF));
        ++i;
        if (i == 10) {
            fprintf(l_file, "\n");
            i = 0;
        }
        ++s;
    }
    if (i == 0) {
        fprintf(l_file, "    ");
    }
}
//............................................................................
unsigned isMatching(char const *fullPath, bool /*isReadonly*/) {
                                            // skip SVN, CVS, or special files
    if ((strcmp(fullPath, ".svn") == 0)
        || (strcmp(fullPath, "CVS") == 0)
        || (strcmp(fullPath, "thumbs.db") == 0)
        || (strcmp(fullPath, "filelist.txt") == 0)
        || (strcmp(fullPath, "dirlist.txt") == 0))
    {
        return 0;
    }
    else {
        return 1;
    }
}
//............................................................................
void onMatchFound(char const *fullPath, unsigned /*flags*/) {
    static char buf[1024];                                   // working buffer

    char const *fname = strstr(fullPath, l_rootDir);
    if (fname == 0) {
        return;
    }
    fname = strchr(fname, dir_separator);
    if (fname == 0) {
        return;
    }
    FILE *fin = fopen(fullPath, "rb");              // open for reading binary
    if (fin == (FILE *)0) {
        return;
    }

    ++l_nFiles;
                     // copy the file name into the buffer and masage it a bit
    char const *s = fname;
    char *d = buf;
    while (*s != '\0') {
        if (*s == dir_separator) {
            *d = '/';
        }
        else {
            *d = *s;
        }
        ++s;
        ++d;
    }
    *d = '\0';
    printf("    Adding: %s...\n", buf);
    fprintf(l_file, "/* %s */\n", buf);

                              // defive the C-variable name from the file name
    char fvar[256];
    s = buf + 1;                                         // skip the first '/'
    d = fvar;
    while (*s != '\0') {
        if ((*s == '/')  || (*s == '.')) {
            *d = '_';
        }
        else {
            *d = *s;
        }
        ++s;
        ++d;
    }
    *d = '\0';

    fprintf(l_file, "static unsigned char const data_%s[] = {\n", fvar);
    fprintf(l_file, "    /* name: */\n");
    dumpStrHex(buf);                                     // dump the file name
    fprintf(l_file, "0x00,\n");                              // zero-terminate

    if (!l_noHtml) {      // encode the HTTP header, if option -h NOT provided
        if (strstr(fname, "404") != 0) {
            strcpy(buf, "HTTP/1.0 404 File not found\r\n");
        }
        else {
            strcpy(buf, "HTTP/1.0 200 OK\r\n");
        }
        strcat(buf, "Server: QP-lwIP (http://www.state-machine.com)\r\n");

                                                   // analyze the file type...
        s = strrchr(fname, '.');
        if (s != 0) {                                            // '.' found?
            if ((strcmp(s, ".htm") == 0)
                || (strcmp(s, ".html") == 0))           // .htm or .html files
            {
                strcat(buf, "Content-type: text/html\r\n");
            }
            else if ((strcmp(s, ".shtm") == 0)
                     || (strcmp(s, ".shtml") == 0))   // .shtm or .shtml files
            {
                strcat(buf, "Content-type: text/html\r\n"
                            "Pragma: no-cache\r\n\r\n");
            }
            else if (strcmp(s, ".css") == 0) {
                strcat(buf, "Content-type: text/css\r\n");
            }
            else if (strcmp(s, ".gif") == 0) {
                strcat(buf, "Content-type: image/gif\r\n");
            }
            else if (strcmp(s, ".png") == 0) {
                strcat(buf, "Content-type: image/png\r\n");
            }
            else if (strcmp(s, ".jpg") == 0) {
                strcat(buf, "Content-type: image/jpeg\r\n");
            }
            else if (strcmp(s, ".bmp") == 0) {
                strcat(buf, "Content-type: image/bmp\r\n");
            }
            else if (strcmp(s, ".class") == 0) {
                strcat(buf, "Content-type: application/octet-stream\r\n");
            }
            else if (strcmp(s, ".ram") == 0) {
                strcat(buf, "Content-type: audio/x-pn-realaudio\r\n");
            }
            else {
                strcat(buf, "Content-type: text/plain\r\n");
            }
        }
        else {
            strcat(buf, "Content-type: text/plain\r\n");
        }
        strcat(buf, "\r\n");

        fprintf(l_file, "    /* HTTP header: */\n");
        dumpStrHex(buf);
        fprintf(l_file, "\n");
    }

    fprintf(l_file, "    /* data: */\n");
    int len = 0;
    int nBytes;
    int i = 0;
    while ((nBytes = fread(buf, 1, sizeof(buf), fin)) != 0) {
        len += nBytes;
        char const *s = buf;
        while (nBytes-- != 0) {
            if (i == 0) {
                fprintf(l_file, "    ");
            }
            fprintf(l_file, "0x%02X, ", ((unsigned)*s & 0xFF));
            ++i;
            if (i == 10) {
                fprintf(l_file, "\n");
                i = 0;
            }
            ++s;
        }
    }
    fprintf(l_file, "\n};\n\n");
    fclose(fin);

    fprintf(l_file, "struct fsdata_file const file_%s[] = {\n    {\n", fvar);
    fprintf(l_file, "        %s,\n",      l_prevFile);
    fprintf(l_file, "        data_%s,\n", fvar);
    fprintf(l_file, "        data_%s + %d,\n", fvar, strlen(fvar) + 2);
    fprintf(l_file, "        sizeof(data_%s) - %d\n", fvar, strlen(fvar) + 2);
    fprintf(l_file, "    }\n};\n\n");
    sprintf(l_prevFile, "file_%s", fvar);
}
//............................................................................
int main(int argc, char *argv[]) {
    char const *fileName = "fsdata.h";

    printf("qfsgen 5.1.1 (c) Quantum Leaps, www.state-machine.com\n"
           "Usage: qfsgen [root-dir [output-file]] [-h]\n"
           "      -h suppresses generation of the HTTP headers\n\n");

                                                  // parse the command line...
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0) {
            l_noHtml = true;
        }
        else {
            l_rootDir = argv[1];
        }
        if (argc > 2) {
            if (strcmp(argv[2], "-h") == 0) {
                l_noHtml = true;
            }
            else {
                fileName = argv[2];
            }
            if (argc > 3) {
                if (strcmp(argv[3], "-h") == 0) {
                    l_noHtml = true;
                }
            }
        }
    }

    l_file = fopen(fileName, "w");
    if (l_file == 0) {
        printf("The file %s could not be opened for writing.", fileName);
        return -1;
    }

    fprintf(l_file, "/* This file has been generated"
                    "with the qfsgen utility. */\n\n");
    l_nFiles = 0;
    strcpy(l_prevFile, "(struct fsdata_file *)0");
    filesearch(l_rootDir);    // search through the file-system directory tree
    fprintf(l_file, "#define FS_ROOT %s\n\n", l_prevFile);
    fprintf(l_file, "#define FS_NUMFILES %d\n", l_nFiles);
    fclose(l_file);

    printf("\nFiles processed: %d\n", l_nFiles);
    return 0;
}

