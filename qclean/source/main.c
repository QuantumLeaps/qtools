//============================================================================
// QClean white space cleanup host utility
//
//                   Q u a n t u m  L e a P s
//                   ------------------------
//                   Modern Embedded Software
//
// Copyright (C) 2005 Quantum Leaps, LLC. <state-machine.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
//
// This software is dual-licensed under the terms of the open source GNU
// General Public License version 3 (or any later version), or alternatively,
// under the terms of one of the closed source Quantum Leaps commercial
// licenses.
//
// The terms of the open source GNU General Public License version 3
// can be found at: <www.gnu.org/licenses/gpl-3.0>
//
// The terms of the closed source Quantum Leaps commercial licenses
// can be found at: <www.state-machine.com/licensing>
//
// Redistributions in source code must retain this top-level comment block.
// Plagiarizing this software to sidestep the license obligations is illegal.
//
// Contact information:
// <www.state-machine.com>
// <info@state-machine.com>
//============================================================================
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "safe_std.h" // "safe" <stdio.h> and <string.h> facilities
#include "qclean.h"
#include "getopt.h"

static int  l_nFiles     = 0;
static int  l_nReadOnly  = 0;
static int  l_nCleaned   = 0;
static int  l_nDirty     = 0;
static int  l_lineLimit  = 0;
static bool l_noCleanup  = false; // perform cleanup by default
static bool l_doReadOnly = false; // don't check read-only files by default

enum Constants {
    TAB        = 0x09,
    LF         = 0x0A,
    CR         = 0x0D,
    TAB_SIZE   = 4,           // default TAB size
    LINE_LIMIT = 80           // default line limit
};

enum TodoFlags {
    TRAIL_WS_FLG  = (1 << 0), // clean Trailing Whitespace (always cleaned)
    TAB_FLG       = (1 << 1), // clean TABs
    CR_FLG        = (1 << 2), // clean CR (LF EOL convention)
    LONG_LINE_FLG = (1 << 3), // find/found long lines
    LF_FLG        = (1 << 4), // cleaned single LF (CRLF EOL convention)
    ASCII_FLG     = (1 << 5), // clean non-ascii characters)
};

typedef struct {
    char pattern[32];
    uint8_t len;
    uint8_t flags;
} FileType;

// array of file types recognized by QClean...
// NOTE: For greater flexibility, this array could be read from an external
// config file in the future.
static FileType const l_fileTypes[] = {
    { ".c",       2, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".h",       2, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".cpp",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".hpp",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".s",       2, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".S",       2, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".asm",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".txt",     4, CR_FLG | TAB_FLG | ASCII_FLG     },
    { ".xml",     4, CR_FLG | TAB_FLG                 },
    { ".dox",     4, CR_FLG | TAB_FLG | ASCII_FLG     }, // Doxygen
    { ".md",      3, CR_FLG | TAB_FLG                 }, // markdown
    { ".bat",     4, CR_FLG | TAB_FLG | ASCII_FLG     },
    { ".ld",      3, CR_FLG | TAB_FLG | LONG_LINE_FLG }, // GNU linker
    { ".py",      3, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".pyi",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".pyw",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG },
    { ".java",    5, CR_FLG | TAB_FLG | LONG_LINE_FLG },

    { "Makefile", 8, CR_FLG           | LONG_LINE_FLG },
    { "mak_",     4, CR_FLG           | LONG_LINE_FLG },
    { ".mak",     4, CR_FLG           | LONG_LINE_FLG },
    { ".make",    5, CR_FLG           | LONG_LINE_FLG },
    { ".cmake",   6, CR_FLG           | TAB_FLG       },
    { ".json",    5, CR_FLG           | TAB_FLG       },

    { ".html",    5, CR_FLG | TAB_FLG                 },
    { ".htm",     4, CR_FLG | TAB_FLG                 },
    { ".css",     4, CR_FLG | TAB_FLG                 },

    { ".eww",     4, CR_FLG                           }, // IAR workspace
    { ".ewp",     4, CR_FLG                           }, // IAR project
    { ".ewd",     4, CR_FLG                           }, // IAR debug config
    { ".icf",     4, CR_FLG | TAB_FLG                 }, // IAR linker

    { ".uvprojx", 8, CR_FLG                           }, // uVision project
    { ".uvoptx",  7, CR_FLG                           }, // uVision option

    { ".sln",     4, CR_FLG                           }, // VS solution
    { ".vcxproj", 8, CR_FLG                           }, // VS project
    { ".filters", 8, CR_FLG                           }, // VS sub-folders
    { ".vcxproj.filters",16, CR_FLG                   }, // VS sub-folders

    { ".project", 8, CR_FLG                           }, // Eclipse project
    { ".cproject",9, CR_FLG                           }, // Eclipse CDT project

    { ".sha1",    5, CR_FLG | TAB_FLG                 }, // Sha1 file
    { ".pro",     4, CR_FLG | TAB_FLG                 }, // Qt project

    { ".m",       2, CR_FLG | TAB_FLG | LONG_LINE_FLG }, // MATLAB

    { ".lnt",     4, CR_FLG | TAB_FLG | LONG_LINE_FLG }, // lint
    { ".cfg",     4, CR_FLG | TAB_FLG                 }, // RSM config
};
static int const l_fileNum = sizeof(l_fileTypes)/sizeof(l_fileTypes[0]);
static const uint32_t ascii[256/32] = {
    0x00002600u, // TAB, LF, CR
    0xFFFFFFFFu, // ' '..'?'
    0xFFFFFFFFu, // '@'..'_'
    0x7FFFFFFFu, // '`'..'~'
    0x00000000u, // non ASCII
    0x00000000u, // non ASCII
    0x00000000u, // non ASCII
    0x00000000u, // non ASCII
};
#define IS_ASCII(ch_) ((ascii[ch_ >> 5u] & (1u << (ch_ & 0x1Fu))) != 0u)

//............................................................................
// This function looks for a match between the fname and any of the file
// types defined in the l_fileTypes array. If a match is found, the function
// returns the flags associated with the matching file type. Otherwise
// the function returns 0.
unsigned isMatching(char const *fname) {
    int const flen = (int)strlen(fname);

    FileType const *ft = &l_fileTypes[0];
    for (int n = l_fileNum; n > 0; --n, ++ft) { // go over all file types..
        char const *s = &ft->pattern[0];
        if (((*s == '.') && (flen > ft->len)) // extension
            || (flen >= ft->len))             // beginning of file name
        {
            char const *t = &fname[flen - ft->len];
            int i;
            for (i = ft->len; i > 0; --i, ++s, ++t) { // compare to pattern
                if (*s != *t) { // mismatch?
                    break;
                }
            }
            if (i == 0) { // match found?
                unsigned flags = (unsigned)ft->flags;
                if (l_lineLimit == 0) {
                    flags &= ~LONG_LINE_FLG; // don't report long lines
                }
                return flags;
            }
        }
    }
    return 0U;
}
//............................................................................
void onMatchFound(char const *fname, unsigned flags, int ro_info) {
    char prev = 0x00;
    bool isReadOnly = false;

    ++l_nFiles;
    PRINTF_S("%c", '.');

    FILE *f;
    if (ro_info >= 0) { // read-only information available right away?
        if (ro_info > 0) {
            isReadOnly = true;
            ++l_nReadOnly;
        }
    }
    else { // read-only information not available
        // open for reading/writing (non destructive)
        FOPEN_S(f, fname, "r+");
        if (f == (FILE*)0) { // can't write
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

    FOPEN_S(f, fname, "rb"); // open for reading
    if (f == (FILE*)0) {
        return;
    }

    static uint8_t src_buf[10*1024*1024]; // 10MB buffer
    uint8_t *src = src_buf;
    static uint8_t dst_buf[10*1024*1024]; // 10MB buffer
    uint8_t *dst = dst_buf;
    int nBytes = (int)FREAD_S(src_buf, sizeof(src_buf),
                              1U, sizeof(src_buf), f);
    fclose(f);
    if (nBytes == sizeof(src_buf)) { // full buffer?
        PRINTF_S("\n%s(too big -- skipped)\n", fname);
        return;
    }
    unsigned found = 0;
    bool foundLLs = false;
    int lineCtr = 1;
    int lineLen = 0;

    for (; nBytes > 0; --nBytes, ++src) {
        uint8_t ch = *src;
        switch (ch) {
            case TAB: {
                if ((flags & TAB_FLG) != 0u) { // cleanup tabs?
                    int tab;
                    for (tab = TAB_SIZE; tab > 0; --tab) {
                        *dst++ = ' ';
                    }
                    found |= TAB_FLG; // removed TAB
                }
                else {
                    *dst++ = *src; // copy TAB over
                }
                lineLen += 4;
                break;
            }
            case LF: {
                if (((flags & LONG_LINE_FLG) != 0u)
                    && (lineLen > l_lineLimit))
                {
                    foundLLs = true;
                }
                lineLen = 0;
                ++lineCtr;

                // always cleanup trailing blanks...
                for (; (*(dst - 1) == ' ') && (dst > dst_buf); --dst) {
                    found |= TRAIL_WS_FLG; // removed trailing blank
                }

                if (((flags & CR_FLG) == 0u) // don't clean CRLF?
                    && (prev != CR))  // CR NOT present?
                {
                    *dst++ = CR;      // add CR to the stream
                    found |= LF_FLG;  // cleaned up single LF
                }
                *dst++ = LF;          // copy LF over
                break;
            }
            case CR: {
                if ((flags & CR_FLG) != 0u) { // clean CR?
                    // don't copy CR over
                    found |= CR_FLG;
                }
                else {
                    *dst++ = CR; // copy CR over
                    lineLen += 1;
                }
                break;
            }
            default: {
                if ((flags & ASCII_FLG) != 0u) { // clean non ASCII?
                    if (IS_ASCII(ch)) {
                        *dst++ = ch;
                        lineLen += 1;
                    }
                    else {
                        // don't copy non-ASCII over
                        found |= ASCII_FLG;
                    }
                }
                else {
                    *dst++ = ch;
                    lineLen += 1;
                }
                break;
            }
        }
        prev = ch;
        if (dst >= &dst_buf[sizeof(dst_buf)]) {
            PRINTF_S("\n%s\n", "Error: too big!");
            return;
        }
    }
    if (found) { // anything found?
        PRINTF_S("\n%s", fname);
        if (!l_noCleanup && !isReadOnly) { // not read-only?
            ++l_nCleaned;
            // binary to use LF EOL convention
            FOPEN_S(f, fname, "wb");
            if (f == 0) {
                PRINTF_S(" %s\n", "ERROR: cannot modify!");
                return;
            }
            fwrite(dst_buf, 1, dst - dst_buf, f);
            fclose(f);
            PRINTF_S(" %s", "CLEANED(");
        }
        else {
            ++l_nDirty;
            PRINTF_S(" %s", "FOUND(");
        }
        if (isReadOnly)                   PRINTF_S("%s", "Read-only,");
        if ((found  & TRAIL_WS_FLG) != 0) PRINTF_S("%s", "Trail-WS,");
        if ((found  & TAB_FLG     ) != 0) PRINTF_S("%s", "TABs,");
        if ((found  & CR_FLG      ) != 0) PRINTF_S("%s", "CRs,");
        if ((found  & LF_FLG      ) != 0) PRINTF_S("%s", "LFs,");
        if ((found  & ASCII_FLG   ) != 0) PRINTF_S("%s", "Non-ASCII,");
    }
    if (foundLLs) {
        ++l_nDirty;
        if (found == 0) {
            PRINTF_S("\n%s FOUND(Long-lines", fname);
        }
        else if (!isReadOnly) {
            PRINTF_S(") %s", "FOUND(Long-lines");
        }
        else {
            PRINTF_S("%s", "Long-lines");
        }
    }
    if (found || foundLLs) {
        PRINTF_S("%s\n", ")");
    }

    fflush(stdout);
}

//............................................................................
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

//............................................................................
int main(int argc, char *argv[]) {
    char const *rootDir = ".";
    int optChar;

    PRINTF_S("%s", "QClean " VERSION " Copyright (c) 2005-2024 Quantum Leaps\n"
           "Documentation: https://www.state-machine.com/qtools/qclean.html\n");
    PRINTF_S("%s", "Usage: qclean [root-dir] [options]\n"
    "       root-dir root directory for recursive cleanup (default is .)\n"
    "       options  control the cleanup, -h prints the help\n");

    // parse the command-line parameters ...*/
    if ((argc > 1) && (argv[1][0] != '-')) { // root directory
        rootDir = argv[1];
    }
    PRINTF_S("root-directory: %s\n", rootDir);
    while ((optChar = getopt(argc, argv, ":hqrl::")) != -1) {
         switch (optChar) {
             case 'h': { // help
                 PRINTF_S(l_helpStr, LINE_LIMIT);
                 return 0;
             }
             case 'q': { // query only (no cleanup)
                 l_noCleanup = true;
                 PRINTF_S("%s\n", "-q query-only");
                 break;
             }
             case 'r': { // check also read-only files
                 l_doReadOnly = true;
                 PRINTF_S("%s\n", "-r check also read-only files");
                 break;
             }
             case 'l': { // check long lines
                 if (optarg != NULL) { // is optional argument provided?
                     l_lineLimit = (int)strtoul(optarg, NULL, 10);
                 }
                 else { // apply the default
                     l_lineLimit = LINE_LIMIT;
                 }
                 PRINTF_S("-l line-length:%d\n", l_lineLimit);
                 break;
             }
             default: { // unknown option
                 PRINTF_S(l_helpStr, LINE_LIMIT);
                 return -1;
             }
         }
    }

    l_nFiles = 0;
    filesearch(rootDir);
    PRINTF_S("\n---------------------------------------"
           "----------------------------------------\n"
           "Files processed:%d ", l_nFiles);
    if (l_noCleanup) {
        PRINTF_S("read-only:%d%s, nothing-cleaned(-q), still-dirty:%d\n",
               l_nReadOnly, (l_doReadOnly ? "(checked)" : "(skipped)"),
               l_nDirty);
    }
    else {
        PRINTF_S("read-only:%d%s, cleaned:%d, still-dirty:%d\n",
               l_nReadOnly, (l_doReadOnly ? "(checked)" : "(skipped)"),
               l_nCleaned,
               l_nDirty);
    }
    return 0;
}
