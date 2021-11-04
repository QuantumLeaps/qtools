/**
* @file
* @brief QSPY host uility: sequence diagram generation
* @ingroup qpspy
* @cond
******************************************************************************
* Last updated for version 6.9.4
* Last updated on  2021-10-19
*
*                    Q u a n t u m  L e a P s
*                    ------------------------
*                    Modern Embedded Software
*
* Copyright (C) 2005-2021 Quantum Leaps, LLC. All rights reserved.
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
* along with this program. If not, see <www.gnu.org/licenses>.
*
* Contact information:
* <www.state-machine.com/licensing>
* <info@state-machine.com>
******************************************************************************
* @endcond
*/
#include <stdint.h>
#include <stdbool.h>

#include "safe_std.h" /* "safe" <stdio.h> and <string.h> facilities */
#include "qspy.h"     /* QSPY data parser */
#include "pal.h"      /* QSPY PAL */

enum {
    SEQ_ITEMS_MAX = 10,  /* max number of items in the Sequence list */
};

static FILE*       l_seqFile;
static char        l_seqList[QS_SEQ_LIST_LEN_MAX];
static char        l_seqNames[SEQ_ITEMS_MAX][QS_DNAME_LEN_MAX];
static int         l_seqNum;
static int         l_seqLines;
static int         l_seqSystem;
static DictEntry   l_seqSto[SEQ_ITEMS_MAX];
static Dictionary  l_seqDict;

/*..........................................................................*/
char const *my_strtok(char *str, char delim) {
    static char *next;
    char const *token;
    if (str != (char*)0) {
        next = str;
    }
    token = next;
    if (token != (char *)0) {
        char *s = next;
        for (;;) {
            if (*s == delim) {
                *s = '\0';
                next = s + 1;
                break;
            }
            else if (*s == '\0') {
                next = (char *)0;
                break;
            }
            else {
                ++s;
            }
        }
    }
    return token;
}
/*. . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . */
void QSEQ_config(void* seqFile, const char* seqList) {
    l_seqFile   = (FILE *)seqFile;
    l_seqNum    = 0;
    l_seqSystem = -1;
    if ((seqList != (void*)0) && (*seqList != '\0')) {
        STRNCPY_S(l_seqList, sizeof(l_seqList), seqList);

        /* split the comma-separated 'seqList' into string array l_seqNames[] */
        char seqTokens[QS_SEQ_LIST_LEN_MAX]; /* local mutable copy */
        STRNCPY_S(seqTokens, sizeof(seqTokens), l_seqList);
        char const *token;
        for (token = my_strtok(seqTokens, ',');
             (token != (char*)0) && (l_seqNum < SEQ_ITEMS_MAX);
             token = my_strtok((char *)0, ','), ++l_seqNum)
        {
            if (strncmp(token, "?", 2) == 0) { /* system border? */
                l_seqSystem = l_seqNum;
            }
            STRNCPY_S(l_seqNames[l_seqNum], sizeof(l_seqNames[l_seqNum]),
                     token);
        }
        if (token != (char*)0) {
            SNPRINTF_LINE("   <QSPY-> Too many names in the Sequence list\n"
                          "   %s\n", l_seqList);
            QSPY_printInfo();
        }
        else {
            l_seqLines = 0;
            QSEQ_genHeader();
        }
    }

    Dictionary_ctor(&l_seqDict, l_seqSto,
        sizeof(l_seqSto) / sizeof(l_seqSto[0]));
    Dictionary_config(&l_seqDict, QSPY_conf.objPtrSize);
}

enum {
    SEQ_LANE_WIDTH   = 20,
    SEQ_LEFT_OFFSET  = 19,  /* offset to the middle of the first lane */
    SEQ_BOX_WIDTH    = SEQ_LANE_WIDTH - 3,
    SEQ_LABEL_MAX    = SEQ_LANE_WIDTH - 5,
    SEQ_HEADER_EVERY = 100, /* # lines between repeated headers */
};

/*..........................................................................*/
void QSEQ_configFile(void* seqFile) {
    if (l_seqFile != (FILE*)0) {
        fclose(l_seqFile);
    }
    l_seqFile = (FILE*)seqFile;
    l_seqLines = 0;
    QSEQ_genHeader();
}
/*..........................................................................*/
bool QSEQ_isActive(void) {
    return l_seqFile != (FILE*)0;
}
/*..........................................................................*/
void QSEQ_dictionaryReset(void) {
    Dictionary_reset(&l_seqDict);
}
/*..........................................................................*/
void QSEQ_updateDictionary(char const* name, KeyType key) {
    int n;
    for (n = 0; n < l_seqNum; ++n) { /* brute-force search */
        if (strncmp(l_seqNames[n], name, QS_DNAME_LEN_MAX) == 0) {
            char str[2];
            str[0] = (char)n;
            str[1] = '\0';
            Dictionary_put(&l_seqDict, key, str);
            break;
        }
    }
}
/*..........................................................................*/
int QSEQ_find(KeyType key) {
    int idx = Dictionary_find(&l_seqDict, key);
    if (idx >= 0) {
        char const *str = Dictionary_at(&l_seqDict, idx);
        return (int)str[0];
    }
    return -1;
}
/*..........................................................................*/
void QSEQ_genHeader(void) {
    if ((l_seqNum == 0) || (l_seqFile == (FILE*)0)) {
        return;
    }
    static char seq_header[(SEQ_LEFT_OFFSET
                            + SEQ_LANE_WIDTH*SEQ_ITEMS_MAX + 4) * 3];
    static uint32_t seq_header_len = 0;
    if (seq_header_len == 0) { /* not initialized yet? */
        int n;
        int i = 0;
        int left_box_edge;
        char *seq_line = &seq_header[0];

        /* clear the whole header */
        for (i = 0; (unsigned)i < sizeof(seq_header); ++i) {
            seq_header[i] = ' ';
        }

        /* top box edges... */
        left_box_edge = SEQ_LEFT_OFFSET - SEQ_BOX_WIDTH/2;
        for (n = 0; n < l_seqNum; ++n, left_box_edge += SEQ_LANE_WIDTH) {
            i = left_box_edge;
            seq_line[i] = '+';
            for (i += 1; i < left_box_edge - 1 + SEQ_BOX_WIDTH; ++i) {
                seq_line[i] = '-';
            }
            seq_line[i - SEQ_BOX_WIDTH / 2] = '+';
            seq_line[i] = '+'; i += 1;
        }
        seq_line[i] = '\n';
        seq_line = &seq_line[i + 1];

        /* box content... */
        left_box_edge = SEQ_LEFT_OFFSET - SEQ_BOX_WIDTH/2;
        for (n = 0; n < l_seqNum; ++n, left_box_edge += SEQ_LANE_WIDTH) {
            seq_line[left_box_edge] = '|';

            /* write the name */
            char const *name = l_seqNames[n];
            int len = strlen(name);
            i = left_box_edge + 1;
            if (len < SEQ_BOX_WIDTH - 2) {
                i += (SEQ_BOX_WIDTH - 2 - len)/2; /* center the name */
            }
            for (; (*name != '\0')
                    && (i < left_box_edge + SEQ_BOX_WIDTH - 1);
                    ++name, ++i)
            {
                seq_line[i] = *name;
            }

            i = left_box_edge - 1 + SEQ_BOX_WIDTH;
            seq_line[i] = '|'; i += 1;
        }
        seq_line[i] = '\n';
        seq_line = &seq_line[i + 1];

        /* bottom box edges... */
        left_box_edge = SEQ_LEFT_OFFSET - SEQ_BOX_WIDTH/2;
        for (n = 0; n < l_seqNum; ++n, left_box_edge += SEQ_LANE_WIDTH) {
            i = left_box_edge;
            seq_line[i] = '+';
            for (i += 1; i < left_box_edge - 1 + SEQ_BOX_WIDTH; ++i) {
                seq_line[i] = '-';
            }
            seq_line[i - SEQ_BOX_WIDTH/2] = '+';
            seq_line[i] = '+'; i += 1;
        }
        seq_line[i] = '\n';
        seq_header_len = &seq_line[i + 1] - &seq_header[0];
    }
    if (l_seqLines == 0) {
        FPRINTF_S(l_seqFile, "-g %s\n\n", l_seqList);
    }
    fwrite(seq_header, 1, seq_header_len, l_seqFile);
    l_seqLines += 3;
}
/*..........................................................................*/
void QSEQ_genPost(uint32_t tstamp, int src, int dst, char const* sig,
                       bool isAttempt)
{
    if ((l_seqNum == 0) || (l_seqFile == (FILE*)0)) {
        return;
    }
    if (src < 0) {
        src = l_seqSystem;
    }
    if (dst < 0) {
        dst = l_seqSystem;
    }
    if ((src < 0) || (dst < 0)) {
        return;
    }
    if ((src == l_seqSystem) && (dst == l_seqSystem)) {
        return;
    }
    if ((l_seqLines % SEQ_HEADER_EVERY) == 0) {
        QSEQ_genHeader();
    }
    char seq_line[SEQ_LEFT_OFFSET+ SEQ_LABEL_MAX
                    + SEQ_LANE_WIDTH*SEQ_ITEMS_MAX];
    uint32_t seq_line_len;
    int i = 0;
    int j;

    SNPRINTF_S(&seq_line[i], sizeof(seq_line), "%010u", (unsigned)tstamp);
    i += 10;
    for (; i < SEQ_LEFT_OFFSET + (l_seqNum - 1)*SEQ_LANE_WIDTH; ++i) {
        seq_line[i] = ((i - SEQ_LEFT_OFFSET)%SEQ_LANE_WIDTH) == 0
                        ? '|'
                        : ' ';
    }
    seq_line[i] = '|'; i += 1;
    seq_line[i] = '\n';
    seq_line_len = i + 1;
    char dash = isAttempt ? '~' : '-';
    if (src < dst) {
        for (i = SEQ_LEFT_OFFSET + src*SEQ_LANE_WIDTH;
                i < SEQ_LEFT_OFFSET + dst*SEQ_LANE_WIDTH; ++i)
        {
            seq_line[i] = ((i - SEQ_LEFT_OFFSET) % SEQ_LANE_WIDTH) == 0
                            ? '+'
                            : dash;
        }
        seq_line[i - 1] = '>';
        /* write the signal */
        for (i = SEQ_LEFT_OFFSET + 3 + src*SEQ_LANE_WIDTH;
                ((*sig != '\0')
                    && (i < SEQ_LEFT_OFFSET + SEQ_LABEL_MAX + 3
                            + src*SEQ_LANE_WIDTH));
                ++i, ++sig)
        {
                seq_line[i] = *sig;
        }
    }
    else if (src > dst) {
        i = SEQ_LEFT_OFFSET + dst*SEQ_LANE_WIDTH;
        for (i = SEQ_LEFT_OFFSET + dst*SEQ_LANE_WIDTH;
                i < SEQ_LEFT_OFFSET + src* SEQ_LANE_WIDTH; ++i)
        {
            seq_line[i] = ((i - SEQ_LEFT_OFFSET)% SEQ_LANE_WIDTH) == 0
                            ? '+'
                            : dash;
        }
        seq_line[SEQ_LEFT_OFFSET + 1 + dst*SEQ_LANE_WIDTH] = '<';
        seq_line[SEQ_LEFT_OFFSET + dst*SEQ_LANE_WIDTH] = '|';

        /* write the signal */
        int len = strlen(sig);
        if (len > SEQ_LABEL_MAX) {
            len = SEQ_LABEL_MAX;
        }
        i = SEQ_LEFT_OFFSET - 2 + src*SEQ_LANE_WIDTH - len;
        j = i + SEQ_LABEL_MAX;
        for (; (*sig != '\0') && (i < j); ++i, ++sig) {
            seq_line[i] = *sig;
        }
    }
    else { /* self-posting */
        /* write the signal */
        i = SEQ_LEFT_OFFSET + 3 + src*SEQ_LANE_WIDTH;
        j = i + SEQ_LABEL_MAX;
        for (; (*sig != '\0') && (i < j); ++i, ++sig) {
            seq_line[i] = *sig;
        }
        seq_line[i] = ']'; i += 1;
        if (seq_line_len <= (uint32_t)i) {
            seq_line[i] = '\n';
            seq_line_len = i + 1;
        }
        i = SEQ_LEFT_OFFSET + src*SEQ_LANE_WIDTH;
        seq_line[i + 1] = '<';
        seq_line[i + 2] = '-';
    }
    if (l_seqSystem >= 0) {
        seq_line[SEQ_LEFT_OFFSET + l_seqSystem*SEQ_LANE_WIDTH] = '/';
    }
    seq_line[SEQ_LEFT_OFFSET + src*SEQ_LANE_WIDTH] = isAttempt ? 'A' : '*';
    Q_ASSERT(seq_line_len <= sizeof(seq_line));
    fwrite(seq_line, 1, seq_line_len, l_seqFile);
    l_seqLines += 1;
}
/*..........................................................................*/
void QSEQ_genPostLIFO(uint32_t tstamp, int src, char const* sig) {
    if ((l_seqNum == 0) || (l_seqFile == (FILE*)0)) {
        return;
    }
    if ((l_seqLines % SEQ_HEADER_EVERY) == 0) {
        QSEQ_genHeader();
    }
    char seq_line[SEQ_LEFT_OFFSET + SEQ_LABEL_MAX
                  + SEQ_LANE_WIDTH*SEQ_ITEMS_MAX];
    uint32_t seq_line_len;
    int i = 0;
    int j;

    SNPRINTF_S(&seq_line[i], sizeof(seq_line), "%010u", (unsigned)tstamp);
    i += 10;
    for (; i < SEQ_LEFT_OFFSET + (l_seqNum - 1)*SEQ_LANE_WIDTH; ++i) {
        seq_line[i] = ((i - SEQ_LEFT_OFFSET) % SEQ_LANE_WIDTH) == 0
            ? '|'
            : ' ';
    }
    seq_line[i] = '|'; i += 1;
    seq_line[i] = '\n';
    seq_line_len = i + 1;

    /* write the signal */
    i = SEQ_LEFT_OFFSET + 3 + src * SEQ_LANE_WIDTH;
    j = i + SEQ_LABEL_MAX;
    for (; (*sig != '\0') && (i <= j); ++i, ++sig) {
        seq_line[i] = *sig;
    }
    seq_line[i] = ']'; i += 1;
    if (seq_line_len <= (uint32_t)i) {
        seq_line[i] = '\n';
        seq_line_len = i + 1;
    }
    i = SEQ_LEFT_OFFSET + src * SEQ_LANE_WIDTH;
    seq_line[i + 1] = '<';
    seq_line[i + 2] = '=';

    if (l_seqSystem >= 0) {
        seq_line[SEQ_LEFT_OFFSET + l_seqSystem*SEQ_LANE_WIDTH] = '/';
    }
    seq_line[SEQ_LEFT_OFFSET + src * SEQ_LANE_WIDTH] = '*';
    Q_ASSERT(seq_line_len <= sizeof(seq_line));
    fwrite(seq_line, 1, seq_line_len, l_seqFile);
    l_seqLines += 1;
}
/*..........................................................................*/
void QSEQ_genPublish(uint32_t tstamp, int obj, char const* sig) {
    if ((l_seqNum == 0) || (l_seqFile == (FILE*)0)) {
        return;
    }
    if (obj < 0) {
        obj = l_seqSystem;
    }
    if (obj < 0) {
        return;
    }
    if ((l_seqLines % SEQ_HEADER_EVERY) == 0) {
        QSEQ_genHeader();
    }
    char seq_line[SEQ_LEFT_OFFSET + SEQ_LABEL_MAX
        + SEQ_LANE_WIDTH * SEQ_ITEMS_MAX];
    uint32_t seq_line_len;
    int i = 0;
    int j;

    SNPRINTF_S(&seq_line[i], sizeof(seq_line), "%010u", (unsigned)tstamp);
    i += 10;
    for (;
         i < SEQ_LEFT_OFFSET - SEQ_LANE_WIDTH + SEQ_BOX_WIDTH/2
             + l_seqNum*SEQ_LANE_WIDTH; ++i)
    {
        seq_line[i] = ((i - SEQ_LEFT_OFFSET)%SEQ_LANE_WIDTH) == 0
                       ? '|'
                       : ((i % 2) ? '.' : ' ');
    }
    seq_line[i] = '\n';
    seq_line_len = i + 1;
    i = SEQ_LEFT_OFFSET + obj * SEQ_LANE_WIDTH;

    /* write the signal */
    int len = strlen(sig);
    if (len > SEQ_LABEL_MAX) {
        len = SEQ_LABEL_MAX;
    }
    if (obj < l_seqNum - 1) { /* signal to the right of '*' */
        i = SEQ_LEFT_OFFSET + 3 + obj*SEQ_LANE_WIDTH;
        j = i + SEQ_LABEL_MAX;
        for (; (*sig != '\0') && (i <= j); ++i, ++sig) {
            seq_line[i] = *sig;
        }
    }
    else { /* signal to the left of '*' */
        i = SEQ_LEFT_OFFSET - 2 + obj*SEQ_LANE_WIDTH - len;
        j = i + SEQ_LABEL_MAX;
        for (; (*sig != '\0') && (i <= j); ++i, ++sig) {
            seq_line[i] = *sig;
        }
    }
    if (l_seqSystem >= 0) {
        seq_line[SEQ_LEFT_OFFSET + l_seqSystem*SEQ_LANE_WIDTH] = '/';
    }
    seq_line[SEQ_LEFT_OFFSET + obj * SEQ_LANE_WIDTH] = '*';
    Q_ASSERT(seq_line_len <= sizeof(seq_line));
    fwrite(seq_line, 1, seq_line_len, l_seqFile);
    l_seqLines += 1;
}
/*..........................................................................*/
void QSEQ_genTran(uint32_t tstamp, int obj, char const* state) {
    if ((l_seqNum == 0) || (l_seqFile == (FILE*)0)) {
        return;
    }
    if ((l_seqLines % SEQ_HEADER_EVERY) == 0) {
        QSEQ_genHeader();
    }
    char seq_line[SEQ_LEFT_OFFSET + SEQ_LABEL_MAX
                  + SEQ_LANE_WIDTH*SEQ_ITEMS_MAX];
    uint32_t seq_line_len;
    int i = 0;
    int j;

    SNPRINTF_S(&seq_line[i], sizeof(seq_line), "%010u", (unsigned)tstamp);
    i += 10;
    for (; i < SEQ_LEFT_OFFSET + (l_seqNum - 1)*SEQ_LANE_WIDTH; ++i) {
        seq_line[i] = ((i - SEQ_LEFT_OFFSET)% SEQ_LANE_WIDTH) == 0
                      ? '|'
                      : ' ';
    }
    seq_line[i] = '|'; i += 1;
    seq_line[i] = '\n';
    seq_line_len = i + 1;
    i = SEQ_LEFT_OFFSET + obj* SEQ_LANE_WIDTH;
    /* write the state */
    int len = strlen(state);
    if (len > SEQ_LABEL_MAX) {
        len = SEQ_LABEL_MAX;
    }
    i = SEQ_LEFT_OFFSET + obj* SEQ_LANE_WIDTH - (len + 1)/2;
    j = i + SEQ_LABEL_MAX;
    seq_line[i] = '<'; i += 1;
    for (; (*state != '\0') && (i <= j); ++i, ++state) {
        seq_line[i] = *state;
    }
    seq_line[i] = '>'; i += 1;
    if (seq_line_len <= (uint32_t)i) {
        seq_line[i] = '\n';
        seq_line_len = i + 1;
    }
    if (l_seqSystem >= 0) {
        seq_line[SEQ_LEFT_OFFSET + l_seqSystem*SEQ_LANE_WIDTH] = '/';
    }
    Q_ASSERT(seq_line_len <= sizeof(seq_line));
    fwrite(seq_line, 1, seq_line_len, l_seqFile);
    l_seqLines += 1;
}
/*..........................................................................*/
void QSEQ_genAnnotation(uint32_t tstamp, int obj, char const* ann) {
    if ((l_seqNum == 0) || (l_seqFile == (FILE*)0)) {
        return;
    }
    if ((l_seqLines % SEQ_HEADER_EVERY) == 0) {
        QSEQ_genHeader();
    }
    char seq_line[SEQ_LEFT_OFFSET + SEQ_LABEL_MAX
        + SEQ_LANE_WIDTH*SEQ_ITEMS_MAX];
    uint32_t seq_line_len;
    int i = 0;
    int j;

    SNPRINTF_S(&seq_line[i], sizeof(seq_line), "%010u", (unsigned)tstamp);
    i += 10;
    for (; i < SEQ_LEFT_OFFSET + (l_seqNum - 1)*SEQ_LANE_WIDTH; ++i) {
        seq_line[i] = ((i - SEQ_LEFT_OFFSET)%SEQ_LANE_WIDTH) == 0
            ? '|'
            : ' ';
    }
    seq_line[i] = '|'; i += 1;
    seq_line[i] = '\n';
    seq_line_len = i + 1;
    i = SEQ_LEFT_OFFSET + obj*SEQ_LANE_WIDTH;
    /* write the annotation */
    int len = strlen(ann);
    if (len > SEQ_LABEL_MAX) {
        len = SEQ_LABEL_MAX;
    }
    i = SEQ_LEFT_OFFSET + obj*SEQ_LANE_WIDTH - (len + 1) / 2;
    j = i + SEQ_LABEL_MAX;
    seq_line[i] = '('; i += 1;
    for (; (*ann != '\0') && (i <= j); ++i, ++ann) {
        seq_line[i] = *ann;
    }
    seq_line[i] = ')'; i += 1;
    if (seq_line_len <= (uint32_t)i) {
        seq_line[i] = '\n';
        seq_line_len = i + 1;
    }
    if (l_seqSystem >= 0) {
        seq_line[SEQ_LEFT_OFFSET + l_seqSystem*SEQ_LANE_WIDTH] = '/';
    }
    Q_ASSERT(seq_line_len <= sizeof(seq_line));
    fwrite(seq_line, 1, seq_line_len, l_seqFile);
    l_seqLines += 1;
}
/*..........................................................................*/
void QSEQ_genTick(uint32_t rate, uint32_t nTick) {
    if ((l_seqNum == 0) || (l_seqFile == (FILE*)0)) {
        return;
    }
    if ((l_seqLines % SEQ_HEADER_EVERY) == 0) {
        QSEQ_genHeader();
    }
    char seq_line[SEQ_LEFT_OFFSET + SEQ_LABEL_MAX
                  + SEQ_LANE_WIDTH*SEQ_ITEMS_MAX];
    uint32_t seq_line_len;
    int i = 0;

    SNPRINTF_S(&seq_line[i], sizeof(seq_line),
               "##########  Tick<%1u> Ctr=%010u",
               (unsigned)rate, (unsigned)nTick);
    i += 34;
    for (;
        i < SEQ_LEFT_OFFSET - SEQ_LANE_WIDTH + SEQ_BOX_WIDTH/2
            + l_seqNum*SEQ_LANE_WIDTH; ++i)
    {
        seq_line[i] = ((i - SEQ_LEFT_OFFSET) % SEQ_LANE_WIDTH) == 0
            ? '|'
            : ((i % 2) ? ' ' : '\'');
    }
    if (l_seqSystem >= 0) {
        seq_line[SEQ_LEFT_OFFSET + l_seqSystem*SEQ_LANE_WIDTH] = '/';
    }
    seq_line[i] = '\n';
    seq_line_len = i + 1;
    Q_ASSERT(seq_line_len <= sizeof(seq_line));
    fwrite(seq_line, 1, seq_line_len, l_seqFile);
    l_seqLines += 1;
}

