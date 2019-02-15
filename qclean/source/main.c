/**
* @file
* @brief main for QClean host utility
* @ingroup qclean
* @cond
******************************************************************************
* Last updated for version 6.2.0
* Last updated on  2018-04-09
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2005-2018 Quantum Leaps, LLC. All rights reserved.
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
* https://www.state-machine.com
* mailto:info@state-machine.com
******************************************************************************
* @endcond
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "qclean.h"
#include "getopt.h"

static char l_src[10*1024*1024]; /* 10MB */
static char l_dst[10*1024*1024]; /* 10MB */
static int  l_nFiles     = 0;
static int  l_nReadOnly  = 0;
static int  l_nCleaned   = 0;
static int  l_nDirty     = 0;
static int  l_lineLimit  = 0;
static bool l_noCleanup  = false; /* perform cleanup by default */
static bool l_doReadOnly = false; /* don't check read-only files by default */

enum Constants {
    TAB        = 0x09,
    LF         = 0x0A,
    CR         = 0x0D,
    TAB_SIZE   = 4,            /* default TAB size */
    LINE_LIMIT = 80            /* default line limit */
};

enum TodoFlags {
    TRAIL_WS_FLG  = (1 << 0), /* clean Trailing Whitespace (always cleaned) */
    TAB_FLG       = (1 << 1), /* clean TABs */
    CR_FLG        = (1 << 2), /* clean CR (LF EOL convention) */
    LONG_LINE_FLG = (1 << 3), /* find/found long lines */
    LF_FLG        = (1 << 4), /* cleaned single LF (CRLF EOL convention) */
};

typedef struct {
    char ending[14];
    uint8_t len;
    uint8_t flags;
} FileType;

/* array of file types recognized by QClean...
* NOTE: For greater flexibility, this array could be read from an external
* config file in the future.
*/
static FileType l_fileTypes[] = {
    { ".c",       2, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".h",       2, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".cpp",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".hpp",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".s",       2, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".asm",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".lnt",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG }, /* Lint */
    { ".txt",     4, /*CRLF*/ TAB_FLG                 },
    { ".md",      3, /*CRLF*/ TAB_FLG                 }, /* markdown */
    { ".bat",     4, /*CRLF*/ TAB_FLG                 },
    { ".ld",      3, CR_FLG | TAB_FLG | LONG_LINE_FLG }, /* GNU ld */
    { ".tcl",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".py",      3, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".java",    5, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { "Makefile", 8, CR_FLG           | LONG_LINE_FLG },
    { ".mak",     4, CR_FLG           | LONG_LINE_FLG },
    { ".html",    5, CR_FLG | TAB_FLG                 },
    { ".htm",     4, CR_FLG | TAB_FLG                 },
    { ".php",     4, CR_FLG | TAB_FLG                 },
    { ".dox",     4, CR_FLG | TAB_FLG                 }, /* Doxygen */
    { ".m",       2, CR_FLG | TAB_FLG | LONG_LINE_FLG }, /* MATLAB*/
};
static int l_fileNum = sizeof(l_fileTypes)/sizeof(l_fileTypes[0]);

/*..........................................................................*/
/* This function looks for a match between the fname and any of the file
* types defined in the l_fileTypes array. If a match is found, the function
* returns the flags associated with the matching file type. Otherwise
* the function returns 0.
*/
unsigned isMatching(char const *fname) {
    int len = strlen(fname);
    FileType const *ft = &l_fileTypes[0];

    int n;
    for (n = l_fileNum; n > 0; --n, ++ft) { /* go over all file types... */
        char const *s = &ft->ending[0];
        if (((*s == '.') && (len > ft->len))
            || (len == ft->len))
        {
            char const *t = &fname[len - ft->len];
            int i;
            for (i = ft->len; i > 0; --i, ++s, ++t) { /* compare the names */
                if (*s != *t) { /* missmatch? */
                    break;
                }
            }
            if (i == 0) { /* match found? */
                unsigned flags = (unsigned)ft->flags;
                if (l_lineLimit == 0) {
                    flags &= ~LONG_LINE_FLG; /* don't report long lines */
                }
                return flags;
            }
        }
    }
    return 0;
}
/*..........................................................................*/
void onMatchFound(char const *fname, unsigned flags, int ro_info) {
    FILE *f;
    int nBytes;
    int lineCtr = 1;
    int lineLen = 0;
    char prev = 0x00;
    char *src = l_src;
    char *dst = l_dst;
    unsigned found = 0;
    bool foundLLs = false;
    bool isReadOnly = false;

    ++l_nFiles;
    printf(".");

    if (ro_info >= 0) { /* read-only information available right away? */
        if (ro_info > 0) {
            isReadOnly = true;
            ++l_nReadOnly;
        }
    }
    else { /* read-only information not available */
        f = fopen(fname, "r+"); /*open for reading/writing (non destructive)*/
        if (f == (FILE*)0) { /* can't write */
            isReadOnly = true;
            ++l_nReadOnly;
        }
        else {
            fclose(f);
        }
    }
    if (isReadOnly && !l_doReadOnly) {
        return;
    }

    f = fopen(fname, "rb"); /* open for reading */
    if (f == (FILE*)0) {
        return;
    }

    nBytes = fread(l_src, 1, sizeof(l_src), f);
    fclose(f);
    if (nBytes == sizeof(l_src)) { /* full buffer? */
        printf("\n%s(too big -- skipped)\n", fname);
        return;
    }
    for (; nBytes > 0; --nBytes, ++src) {
        switch (*src) {
            case TAB: {
                if ((flags & TAB_FLG) != 0) { /* cleanup tabs? */
                    int tab;
                    for (tab = TAB_SIZE; tab > 0; --tab) {
                        *dst++ = ' ';
                    }
                    found |= TAB_FLG; /* removed TAB */
                }
                else {
                    *dst++ = *src; /* copy TAB over */
                }
                lineLen += 4;
                break;
            }
            case LF: {
                if (((flags & LONG_LINE_FLG) != 0)
                    && (lineLen > l_lineLimit))
                {
                    foundLLs = true;
                }
                lineLen = 0;
                ++lineCtr;

                /* always cleanup trailing blanks... */
                while (*(dst - 1) == ' ') {
                    found |= TRAIL_WS_FLG; /* removed trailing blank */
                    --dst;
                }

                if (((flags & CR_FLG) == 0) /* don't clean CRLF? */
                    && (prev != CR))  /* CR NOT present? */
                {
                    *dst++ = CR;      /* add CR to the stream */
                    found |= LF_FLG;  /* cleaned up single LF */
                }
                *dst++ = LF;          /* copy LF over */
                break;
            }
            case CR: {
                if ((flags & CR_FLG) != 0) { /* clean CR? */
                    /* don't copy CR over */
                    found |= CR_FLG;
                }
                else {
                    *dst++ = CR; /* copy CR over */
                    lineLen += 1;
                }
                break;
            }
            default: {
                *dst++ = *src;
                lineLen += 1;
                break;
            }
        }
        prev = *src;
        if (dst >= &l_dst[sizeof(l_dst)]) {
            printf("\nError: too big!\n");
            return;
        }
    }
    if (found) { /* anything found? */
        printf("\n%s", fname);
        if (!l_noCleanup && !isReadOnly) { /* not read-only? */
            ++l_nCleaned;
            f = fopen(fname, "wb"); /* binary to use LF EOL convention */
            if (f == 0) {
                printf(" ERROR: cannot modify!\n");
                return;
            }
            fwrite(l_dst, 1, dst - l_dst, f);
            fclose(f);
            printf(" CLEANED(");
        }
        else {
            ++l_nDirty;
            printf(" FOUND(");
        }
        if (isReadOnly)                   printf("Read-only,");
        if ((found  & TRAIL_WS_FLG) != 0) printf("Trail-WS,");
        if ((found  & TAB_FLG     ) != 0) printf("TABs,");
        if ((found  & CR_FLG      ) != 0) printf("CRs,");
        if ((found  & LF_FLG      ) != 0) printf("LFs,");
    }
    if (foundLLs) {
        ++l_nDirty;
        if (found == 0) {
            printf("\n%s FOUND(Long-lines", fname);
        }
        else if (!isReadOnly) {
            printf(") FOUND(Long-lines");
        }
        else {
            printf("Long-lines");
        }
    }
    if (found || foundLLs) {
        printf(")\n");
    }

    fflush(stdout);
}

/*..........................................................................*/
static char const l_helpStr[] =
    "\nUsage: qclean [root-dir] [options]\n"
    "\n"
    "ARGUMENT      DEFAULT   COMMENT\n"
    "---------------------------------------------------------------\n"
    "[root-dir]    .         root directory (relative or absolute)\n"
    "\n"
    "OPTIONS:\n"
    "-h                      help (show this message and exit)\n"
    "-q                      query only (no cleanup when -q present)\n"
    "-r                      check also read-only files\n"
    "-l[limit]     %d        line length limit (not checked when -l absent)\n";

/*..........................................................................*/
int main(int argc, char *argv[]) {
    char const *rootDir = ".";
    int optChar;

    printf("QClean " VERSION " Copyright (c) 2005-2019 Quantum Leaps\n"
           "Documentation: https://www.state-machine.com/qtools/qclean.html\n");
    printf("Usage: qclean [root-dir] [options]\n"
    "       root-dir root directory for recursive cleanup (default is .)\n"
    "       options  control the cleanup, -h prints the help\n");

    /* parse the command-line parameters ...*/
    if ((argc > 1) && (argv[1][0] != '-')) { /* root directory */
        rootDir = argv[1];
    }
    printf("root-directory: %s\n", rootDir);
    while ((optChar = getopt(argc, argv, ":hqrl::")) != -1) {
         switch (optChar) {
             case 'h': { /* help */
                 printf(l_helpStr, LINE_LIMIT);
                 return 0;
             }
             case 'q': { /* query only (no cleanup) */
                 l_noCleanup = true;
                 printf("-q query-only\n");
                 break;
             }
             case 'r': { /* check also read-only files */
                 l_doReadOnly = true;
                 printf("-r check also read-only files\n");
                 break;
             }
             case 'l': { /* check long lines */
                 if (optarg != NULL) { /* is optional argument provided? */
                     l_lineLimit = (int)strtoul(optarg, NULL, 10);
                 }
                 else { /* apply the default */
                     l_lineLimit = LINE_LIMIT;
                 }
                 printf("-l line-length:%d\n", l_lineLimit);
                 break;
             }
             default: { /* unknown option */
                 printf(l_helpStr, LINE_LIMIT);
                 return -1;
             }
         }
    }

    l_nFiles = 0;
    filesearch(rootDir);
    printf("\n---------------------------------------"
           "----------------------------------------\n"
           "Files processed:%d ", l_nFiles);
    if (l_noCleanup) {
        printf("read-only:%d%s, nothing-cleaned(-q), still-dirty:%d\n",
               l_nReadOnly, (l_doReadOnly ? "(checked)" : "(skipped)"),
               l_nDirty);
    }
    else {
        printf("read-only:%d%s, cleaned:%d, still-dirty:%d\n",
               l_nReadOnly, (l_doReadOnly ? "(checked)" : "(skipped)"),
               l_nCleaned,
               l_nDirty);
    }
    return 0;
}
