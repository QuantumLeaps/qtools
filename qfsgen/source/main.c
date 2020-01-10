/**
* @file
* @brief QFSgen ROM file system generator main function
* @ingroup qfsgen
* @cond
******************************************************************************
* Last updated for version 6.7.0
* Last updated on  2020-01-05
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

#include <stdbool.h>

#include "safe_std.h" /* "safe" <stdio.h> and <string.h> facilities */
#include "qfsgen.h"

static char const *l_fsDir = "fs";
static FILE *l_file;
static bool  l_genHttpHeaders = false;
static int   l_nFiles;
static char  l_prevFile[256];

/*..........................................................................*/
static void dumpStrHex(char const *s) {
    int len = 0;
    int i = 0;
    while (*s != '\0') {
        if (i == 0) { /* new line? */
            ++i;
            if (len == 0) { /* beginning of the file? */
                FPRINTF_S(l_file, "    ");
            }
            else {
                FPRINTF_S(l_file, ",\x0A    ");
            }
        }
        else if (i < 9) { /* inside the line? */
            ++i;
            FPRINTF_S(l_file, ", ");
        }
        else { /* end of line */
            i = 0;
            FPRINTF_S(l_file, ", ");
        }
        FPRINTF_S(l_file, "0x%02X", ((unsigned)*s & 0xFF));
        ++s;
        ++len;
    }
    if (i == 0) {
        FPRINTF_S(l_file, "    ");
    }
}
/*..........................................................................*/
unsigned isMatching(char const *fullPath) {
    /* skip SVN, CVS, or special files */
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
/*..........................................................................*/
void onMatchFound(char const *fullPath, unsigned flags, int ro_info) {
    static char buf[1024]; /* working buffer */
    char const *fname = strstr(fullPath, l_fsDir);
    FILE *fin;
    char const *s;
    char *d;
    char fvar[256];
    int len;
    int nBytes;
    int i;

    (void)flags;
    (void)ro_info;

    if (fname == (char *)0) {
        return;
    }
    fname = strchr(fname, dir_separator);
    if (fname == (char *)0) {
        return;
    }
    FOPEN_S(fin, fullPath, "rb"); /* open for reading binary */
    if (fin == (FILE *)0) {
        return;
    }

    ++l_nFiles;
    /* copy the file name into the buffer and masage it a bit */
    s = fname;
    d = buf;
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
    PRINTF_S("\nAdding: %s%s", l_fsDir, buf);
    FPRINTF_S(l_file, "/* %s */\x0A", buf);

    /* defive the C-variable name from the file name */
    s = buf + 1; /* skip the first '/' */
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

    FPRINTF_S(l_file, "static unsigned char const data_%s[] = {\x0A", fvar);
    FPRINTF_S(l_file, "    /* name: */\x0A");
    dumpStrHex(buf); /* dump the file name */
    FPRINTF_S(l_file, ", 0x00,\x0A"); /* zero-terminate */

    if (l_genHttpHeaders) { /* encode HTTP header, if option -h provided */
        if (strstr(fname, "404") != 0) {
            STRCPY_S(buf, sizeof(buf), "HTTP/1.0 404 File not found\r\x0A");
        }
        else {
            STRCPY_S(buf, sizeof(buf), "HTTP/1.0 200 OK\r\x0A");
        }
        STRCAT_S(buf, sizeof(buf),
                  "Server: QL (https://state-machine.com)\r\x0A");

        /* analyze the file type... */
        s = strrchr(fname, '.');
        if (s != 0) { // '.' found?
            if ((strcmp(s, ".htm") == 0)
                || (strcmp(s, ".html") == 0)) /* .htm/.html files */
            {
                STRCAT_S(buf, sizeof(buf), "Content-type: text/html\r\x0A");
            }
            else if ((strcmp(s, ".shtm") == 0)
                     || (strcmp(s, ".shtml") == 0)) /* .shtm/.shtml files */
            {
                STRCAT_S(buf, sizeof(buf), "Content-type: text/html\r\x0A"
                            "Pragma: no-cache\r\x0A\r\x0A");
            }
            else if (strcmp(s, ".css") == 0) {
                STRCAT_S(buf, sizeof(buf), "Content-type: text/css\r\x0A");
            }
            else if (strcmp(s, ".gif") == 0) {
                STRCAT_S(buf, sizeof(buf), "Content-type: image/gif\r\x0A");
            }
            else if (strcmp(s, ".png") == 0) {
                STRCAT_S(buf, sizeof(buf), "Content-type: image/png\r\x0A");
            }
            else if (strcmp(s, ".jpg") == 0) {
                STRCAT_S(buf, sizeof(buf), "Content-type: image/jpeg\r\x0A");
            }
            else if (strcmp(s, ".bmp") == 0) {
                STRCAT_S(buf, sizeof(buf), "Content-type: image/bmp\r\x0A");
            }
            else if (strcmp(s, ".class") == 0) {
                STRCAT_S(buf, sizeof(buf),
                          "Content-type: application/octet-stream\r\x0A");
            }
            else if (strcmp(s, ".ram") == 0) {
                STRCAT_S(buf, sizeof(buf),
                          "Content-type: audio/x-pn-realaudio\r\x0A");
            }
            else {
                STRCAT_S(buf, sizeof(buf), "Content-type: text/plain\r\x0A");
            }
        }
        else {
            STRCAT_S(buf, sizeof(buf), "Content-type: text/plain\r\x0A");
        }
        STRCAT_S(buf, sizeof(buf), "\r\x0A");

        FPRINTF_S(l_file, "    /* HTTP header: */\x0A");
        dumpStrHex(buf);
        FPRINTF_S(l_file, ",\x0A");
    }

    FPRINTF_S(l_file, "    /* data: */\x0A");
    len = 0;
    i = 0;
    while ((nBytes = fread(buf, 1, sizeof(buf), fin)) != 0) {
        char const *pc = buf;
        int n = nBytes;
        while (n-- != 0) {
            if (i == 0) { /* new line? */
                ++i;
                if (len == 0) { /* beginning of the file? */
                    FPRINTF_S(l_file, "    ");
                }
                else {
                    FPRINTF_S(l_file, ",\x0A    ");
                }
            }
            else if (i < 9) { /* inside the line? */
                ++i;
                FPRINTF_S(l_file, ", ");
            }
            else { /* end of line */
                i = 0;
                FPRINTF_S(l_file, ", ");
            }
            FPRINTF_S(l_file, "0x%02X", ((unsigned)*pc & 0xFF));
            ++pc;
            len += nBytes;
        }
    }
    FPRINTF_S(l_file, "\x0A};\x0A\x0A");
    fclose(fin);

    FPRINTF_S(l_file, "struct fsdata_file const file_%s[] = {\x0A    {\x0A",
                    fvar);
    FPRINTF_S(l_file, "        %s,\x0A",      l_prevFile);
    FPRINTF_S(l_file, "        data_%s,\x0A", fvar);
    FPRINTF_S(l_file, "        data_%s + %d,\x0A",
                    fvar, (int)(strlen(fvar) + 2));
    FPRINTF_S(l_file, "        sizeof(data_%s) - %d\x0A",
                    fvar, (int)(strlen(fvar) + 2));
    FPRINTF_S(l_file, "    }\x0A};\x0A\x0A");
    SNPRINTF_S(l_prevFile, sizeof(l_prevFile) - 1U, "file_%s", fvar);
}
/*..........................................................................*/
int main(int argc, char *argv[]) {
    char const *fileName = "fsdata.h";

    PRINTF_S("QFSGen " VERSION " Copyright (c) 2005-2020 Quantum Leaps\n"
             "Documentation: https://state-machine.com/qtools/qfsgen.html\n");
    PRINTF_S("Usage: qfsgen fs-dir [output-file] [-h]\n"
             "       fs-dir      file-system directory (must be provided)\n"
             "       output-file optional (default is %s)\n"
             "       -h          generate the HTTP headers\n",
             fileName);

    /* parse the command line... */
    if (argc < 2) {
        PRINTF_S("the fs-dir argument must be provided\n");
        return -1;
    }
    else {
        l_fsDir = argv[1];
        if (argc > 2) {
            if (strcmp(argv[2], "-h") == 0) {
                l_genHttpHeaders = true;
            }
            else {
                fileName = argv[2];
            }
            if (argc > 3) {
                if (strcmp(argv[3], "-h") == 0) {
                    l_genHttpHeaders = true;
                }
            }
        }
    }
    PRINTF_S("fs-directory: %s\n", l_fsDir);
    PRINTF_S("outut-file  : %s\n", fileName);
    PRINTF_S("HTTP headers: %s\n", l_genHttpHeaders
                                 ? "generated" : "not-generated");

    /* binary to use LF EOL convention */
    FOPEN_S(l_file, fileName, "wb");
    if (l_file == 0) {
        PRINTF_S("File %s could not be opened for writing.", fileName);
        return -1;
    }

    FPRINTF_S(l_file, "/* This file has been generated "
                    "with the qfsgen utility. */\x0A\x0A");
    l_nFiles = 0;
    STRCPY_S(l_prevFile, sizeof(l_prevFile), "(struct fsdata_file *)0");
    filesearch(l_fsDir); /* search through the file-system directory tree */
    FPRINTF_S(l_file, "#define FS_ROOT %s\x0A\x0A", l_prevFile);
    FPRINTF_S(l_file, "#define FS_NUMFILES %d\x0A", l_nFiles);
    fclose(l_file);

    PRINTF_S("\n---------------------------------------"
           "----------------------------------------\n"
           "Files processed:%d; Generated:%s\n",
           l_nFiles, fileName);

    return 0;
}
