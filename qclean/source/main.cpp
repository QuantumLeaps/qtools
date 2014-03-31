//////////////////////////////////////////////////////////////////////////////
// Product: qclean utility
// Last updated for version 5.3.0
// Last updated on  2014-03-31
//
//                    Q u a n t u m     L e a P s
//                    ---------------------------
//                    innovating embedded systems
//
// Copyright (C) Quantum Leaps, www.state-machine.com.
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
// Web:   www.state-machine.com
// Email: info@state-machine.com
//////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <string.h>
#include "filesearch.h"

static char l_src[1024*1024];                                           // 1MB
static char l_dst[1024*1024];                                           // 1MB
static long l_nFiles = 0UL;
static long l_nSkipped = 0UL;
static long l_nCleaned = 0UL;

enum CleanupFlags {
    CR_FLG          = (1U << 0),
    TAB_FLG         = (1U << 1),
    TRAIL_BLANK_FLG = (1U << 2),
    LONG_LINE_FLG   = (1U << 3),

    RDONLY_FLG      = (1U << 8),
};

//............................................................................
unsigned isMatching(char const *fname, bool isReadonly) {
    int len = strlen(fname);
    int flags = 0;                                       // no flags initially

    if (isReadonly) {
        flags |= RDONLY_FLG;                           // the file is readonly
    }

    if ((len > 2)
        && (fname[len - 2] == '.')
        && ((fname[len - 1] == 'c')                                // C source
            || (fname[len - 1] == 'h')                         // C/C++ header
            || (fname[len - 1] == 's')))                          // assembler
    {
        flags |= (CR_FLG | TAB_FLG | TRAIL_BLANK_FLG | LONG_LINE_FLG);
    }
    else if ((len > 4)
             && ((strcmp(&fname[len - 4], ".cpp") == 0)          // C++ source
                  || (strcmp(&fname[len - 4], ".hpp") == 0)      // C++ header
                  || (strcmp(&fname[len - 4], ".tcl") == 0)          // TCL/TK
                  || (strcmp(&fname[len - 4], ".asm") == 0)))     // assembler
    {
        flags |= (CR_FLG | TAB_FLG | TRAIL_BLANK_FLG | LONG_LINE_FLG);
    }
    else if ((len == 8) && (strcmp(&fname[0], "Makefile") == 0)) { // Makefile
        flags |= (CR_FLG | TRAIL_BLANK_FLG | LONG_LINE_FLG);//don't clean tabs
    }
    else if ((len > 4)
             && (strcmp(&fname[len - 4], ".mak") == 0))            // makefile
    {
        flags |= (CR_FLG | TRAIL_BLANK_FLG | LONG_LINE_FLG);//don't clean tabs
    }
    else if ((len > 4)
             && ((strcmp(&fname[len - 4], ".bat") == 0)          // batch file
                  || (strcmp(&fname[len - 4], ".txt") == 0)       // text file
                  || (strcmp(&fname[len - 4], ".lnt") == 0)))     // lint file
    {
        flags |= (TAB_FLG | TRAIL_BLANK_FLG);  // don't clean CR or long lines
    }
    else if ((len > 3)
             && (strcmp(&fname[len - 3], ".ld") == 0))        // GNU ld script
    {
        flags |= (CR_FLG | TAB_FLG | TRAIL_BLANK_FLG | LONG_LINE_FLG);
    }
    else if ((len > 5) && (strcmp(&fname[len - 5], ".java") == 0)) {   // Java
        flags |= (CR_FLG | TAB_FLG | TRAIL_BLANK_FLG | LONG_LINE_FLG);
    }
    else if ((len > 2)
        && (fname[len - 2] == '.')
        && (fname[len - 1] == 'm'))                           // Matlab m-file
    {
        flags |= (CR_FLG | TAB_FLG | TRAIL_BLANK_FLG);//don't clean long lines
    }

    return flags;
}
//............................................................................
void onMatchFound(char const *fname, unsigned flags) {
    ++l_nFiles;
    printf(".");

    if ((flags & RDONLY_FLG) != 0) {                         // readonly file?
        ++l_nSkipped;
        return;
    }

    FILE *f = fopen(fname, "rb");                          // open for reading
    if (f == (FILE*)0) {
        return;
    }

    int nBytes = fread(l_src, 1, sizeof(l_src), f);
    fclose(f);
    if (nBytes == sizeof(l_src)) {                             // full buffer?
        printf("%s(big!)\n", fname);
        return;
    }
    int lineCtr = 1;
    int lineLen = 0;
    char *src = l_src;
    char *dst = l_dst;
    unsigned char diff = 0;
    for (; nBytes > 0; --nBytes, ++src) {
        switch (*src) {
            case 0x09: {                                                // tab
                if ((flags & TAB_FLG) != 0) {                 // cleanup tabs?
                    diff |= TAB_FLG;                            // removed TAB
                    *dst++ = ' ';
                    *dst++ = ' ';
                    *dst++ = ' ';
                    *dst++ = ' ';
                }
                else {
                    *dst++ = *src;                            // copy TAB over
                }
                lineLen += 4;
                break;
            }
            case 0x0A: {                                                // EOL
                lineLen = 0;
                ++lineCtr;
                if ((flags & TRAIL_BLANK_FLG) != 0) {       // cleanup blanks?
                    while (*(dst - 1) == ' ') {
                        diff |= TRAIL_BLANK_FLG;     // removed trailing blank
                        --dst;
                    }
                }
                *dst++ = *src;
                break;
            }
            case 0x0D: {                                                 // CR
                if ((flags & CR_FLG) != 0) {                    // cleanup CR?
                    diff |= CR_FLG;                              // removed CR
                }
                else {
                    *dst++ = *src;                             // copy CR over
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
        if (dst >= &l_dst[sizeof(l_dst)]) {
            printf("\nError: too big!\n");
            return;
        }
    }
    if (diff) {
        printf("\n %s", fname);
        ++l_nCleaned;
        f = fopen(fname, "wb");                            // open for writing
        if (f == 0) {
            printf(" Error: cannot modify!\n");
            return;
        }
        fwrite(l_dst, 1, dst - l_dst, f);
        fclose(f);

        printf(" CLEANED(");
        if ((diff & CR_FLG         ) != 0) printf("CR,");
        if ((diff & TAB_FLG        ) != 0) printf("TAB,");
        if ((diff & TRAIL_BLANK_FLG) != 0) printf("BLK,");
        if ((diff & LONG_LINE_FLG  ) != 0) printf("LL,");
        if ((diff & RDONLY_FLG     ) != 0) printf("RO,");
        printf(" )\n");
    }

    fflush(stdout);
}
//............................................................................
int main(int argc, char *argv[]) {
    char const *rootDir = ".";

    printf("qclean 5.3.0 (c) Quantum Leaps. www.state-machine.com\n");
    if (argc > 1) {
        rootDir = argv[1];
    }
    l_nFiles = 0;
    filesearch(rootDir);
    printf("\nFiles processed: %ld, skipped=%ld, cleaned %ld\n",
           l_nFiles,
           l_nSkipped,
           l_nCleaned);
    return 0;
}

