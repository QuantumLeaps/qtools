/**
* @file
* @brief QFSgen ROM file system generator main function
* @ingroup qfsgen
* @cond
******************************************************************************
* Last updated for version 6.6.0
* Last updated on  2019-07-30
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005-2019 Quantum Leaps, LLC. All rights reserved.
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
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*
* Contact information:
* <www.state-machine.com>
* <info@state-machine.com>
******************************************************************************
* @endcond
*/

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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
                fprintf(l_file, "    ");
            }
            else {
                fprintf(l_file, ",\x0A    ");
            }
        }
        else if (i < 9) { /* inside the line? */
            ++i;
            fprintf(l_file, ", ");
        }
        else { /* end of line */
            i = 0;
            fprintf(l_file, ", ");
        }
        fprintf(l_file, "0x%02X", ((unsigned)*s & 0xFF));
        ++s;
        ++len;
    }
    if (i == 0) {
        fprintf(l_file, "    ");
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
    fin = fopen(fullPath, "rb"); /* open for reading binary */
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
    printf("\nAdding: %s%s", l_fsDir, buf);
    fprintf(l_file, "/* %s */\x0A", buf);

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

    fprintf(l_file, "static unsigned char const data_%s[] = {\x0A", fvar);
    fprintf(l_file, "    /* name: */\x0A");
    dumpStrHex(buf); /* dump the file name */
    fprintf(l_file, ", 0x00,\x0A"); /* zero-terminate */

    if (l_genHttpHeaders) { /* encode HTTP header, if option -h provided */
        if (strstr(fname, "404") != 0) {
            strcpy(buf, "HTTP/1.0 404 File not found\r\x0A");
        }
        else {
            strcpy(buf, "HTTP/1.0 200 OK\r\x0A");
        }
        strcat(buf, "Server: QL (https://state-machine.com)\r\x0A");

        /* analyze the file type... */
        s = strrchr(fname, '.');
        if (s != 0) { // '.' found?
            if ((strcmp(s, ".htm") == 0)
                || (strcmp(s, ".html") == 0)) /* .htm/.html files */
            {
                strcat(buf, "Content-type: text/html\r\x0A");
            }
            else if ((strcmp(s, ".shtm") == 0)
                     || (strcmp(s, ".shtml") == 0)) /* .shtm/.shtml files */
            {
                strcat(buf, "Content-type: text/html\r\x0A"
                            "Pragma: no-cache\r\x0A\r\x0A");
            }
            else if (strcmp(s, ".css") == 0) {
                strcat(buf, "Content-type: text/css\r\x0A");
            }
            else if (strcmp(s, ".gif") == 0) {
                strcat(buf, "Content-type: image/gif\r\x0A");
            }
            else if (strcmp(s, ".png") == 0) {
                strcat(buf, "Content-type: image/png\r\x0A");
            }
            else if (strcmp(s, ".jpg") == 0) {
                strcat(buf, "Content-type: image/jpeg\r\x0A");
            }
            else if (strcmp(s, ".bmp") == 0) {
                strcat(buf, "Content-type: image/bmp\r\x0A");
            }
            else if (strcmp(s, ".class") == 0) {
                strcat(buf, "Content-type: application/octet-stream\r\x0A");
            }
            else if (strcmp(s, ".ram") == 0) {
                strcat(buf, "Content-type: audio/x-pn-realaudio\r\x0A");
            }
            else {
                strcat(buf, "Content-type: text/plain\r\x0A");
            }
        }
        else {
            strcat(buf, "Content-type: text/plain\r\x0A");
        }
        strcat(buf, "\r\x0A");

        fprintf(l_file, "    /* HTTP header: */\x0A");
        dumpStrHex(buf);
        fprintf(l_file, ",\x0A");
    }

    fprintf(l_file, "    /* data: */\x0A");
    len = 0;
    i = 0;
    while ((nBytes = fread(buf, 1, sizeof(buf), fin)) != 0) {
        char const *s = buf;
        int n = nBytes;
        while (n-- != 0) {
            if (i == 0) { /* new line? */
                ++i;
                if (len == 0) { /* beginning of the file? */
                    fprintf(l_file, "    ");
                }
                else {
                    fprintf(l_file, ",\x0A    ");
                }
            }
            else if (i < 9) { /* inside the line? */
                ++i;
                fprintf(l_file, ", ");
            }
            else { /* end of line */
                i = 0;
                fprintf(l_file, ", ");
            }
            fprintf(l_file, "0x%02X", ((unsigned)*s & 0xFF));
            ++s;
            len += nBytes;
        }
    }
    fprintf(l_file, "\x0A};\x0A\x0A");
    fclose(fin);

    fprintf(l_file, "struct fsdata_file const file_%s[] = {\x0A    {\x0A",
                    fvar);
    fprintf(l_file, "        %s,\x0A",      l_prevFile);
    fprintf(l_file, "        data_%s,\x0A", fvar);
    fprintf(l_file, "        data_%s + %d,\x0A",
                    fvar, (int)(strlen(fvar) + 2));
    fprintf(l_file, "        sizeof(data_%s) - %d\x0A",
                    fvar, (int)(strlen(fvar) + 2));
    fprintf(l_file, "    }\x0A};\x0A\x0A");
    snprintf(l_prevFile, sizeof(l_prevFile) - 1U, "file_%s", fvar);
}
/*..........................................................................*/
int main(int argc, char *argv[]) {
    char const *fileName = "fsdata.h";

    printf("QFSGen " VERSION " Copyright (c) 2005-2017 Quantum Leaps\n"
           "Documentation: https://state-machine.com/qtools/qfsgen.html\n");
    printf("Usage: qfsgen fs-dir [output-file] [-h]\n"
           "       fs-dir      file-system directory (must be provided)\n"
           "       output-file optional (default is %s)\n"
           "       -h          generate the HTTP headers\n",
           fileName);

    /* parse the command line... */
    if (argc < 2) {
        printf("the fs-dir argument must be provided\n");
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
    printf("fs-directory: %s\n", l_fsDir);
    printf("outut-file  : %s\n", fileName);
    printf("HTTP headers: %s\n", l_genHttpHeaders
                                 ? "generated" : "not-generated");

    l_file = fopen(fileName, "wb"); /* binary to use LF EOL convention */
    if (l_file == 0) {
        printf("File %s could not be opened for writing.", fileName);
        return -1;
    }

    fprintf(l_file, "/* This file has been generated "
                    "with the qfsgen utility. */\x0A\x0A");
    l_nFiles = 0;
    strcpy(l_prevFile, "(struct fsdata_file *)0");
    filesearch(l_fsDir); /* search through the file-system directory tree */
    fprintf(l_file, "#define FS_ROOT %s\x0A\x0A", l_prevFile);
    fprintf(l_file, "#define FS_NUMFILES %d\x0A", l_nFiles);
    fclose(l_file);

    printf("\n---------------------------------------"
           "----------------------------------------\n"
           "Files processed:%d; Generated:%s\n",
           l_nFiles, fileName);

    return 0;
}
