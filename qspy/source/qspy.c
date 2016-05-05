/*****************************************************************************
* Product: QSPY -- record parsing and encoding
* Last updated for version 5.6.4
* Last updated on  2016-05-04
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) Quantum Leaps, LLC. All rights reserved.
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
* http://www.state-machine.com
* mailto:info@state-machine.com
*****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

#include "qspy.h"
#include "pal.h"

typedef char     char_t;
typedef float    float32_t;
typedef double   float64_t;
typedef int      enum_t;
typedef unsigned uint_t;

#ifndef Q_SPY
#define Q_SPY 1
#endif
#include "qs_copy.h" /* copy of the target-resident QS interface */

/* global objects ..........................................................*/
char QSPY_line[512]; /* last formatted line ready for output */

/****************************************************************************/
/* name length for internal buffers (longer names truncated) ... */
#define NAME_LEN 64

/*..........................................................................*/
typedef uint64_t KeyType;
typedef uint32_t SigType;
typedef uint64_t ObjType;

typedef struct {
    KeyType key;
    char    name[NAME_LEN];
} DictEntry;

typedef struct {
    DictEntry  notFound;
    DictEntry *sto;
    int        capacity;
    int        entries;
    int        keySize;
} Dictionary;

static void Dictionary_ctor(Dictionary * const me,
                            DictEntry *sto, uint32_t capacity);
static void Dictionary_config(Dictionary * const me, int keySize);
static char const *Dictionary_at(Dictionary * const me, unsigned idx);
static void Dictionary_put(Dictionary * const me,
                           KeyType key, char const *name);
static char const *Dictionary_get(Dictionary * const me,
                                  KeyType key, char *buf);
static int Dictionary_find(Dictionary * const me, KeyType key);
static void Dictionary_reset(Dictionary * const me);
static void Dictionary_write(Dictionary const * const me, FILE *stream);
static bool Dictionary_read(Dictionary * const me, FILE *stream);

static char const *getMatDict(char const *s);

/*..........................................................................*/
typedef struct SigDictEntryTag {
    SigType sig;
    ObjType obj;
    char    name[NAME_LEN];
} SigDictEntry;

typedef struct SigDictionaryTag {
    SigDictEntry  notFound;
    SigDictEntry *sto;
    int           capacity;
    int           entries;
    int           ptrSize;
} SigDictionary;

static void SigDictionary_ctor(SigDictionary * const me,
                        SigDictEntry *sto, uint32_t capacity);
static void SigDictionary_config(SigDictionary * const me, int ptrSize);
static void SigDictionary_put(SigDictionary * const me,
                        SigType sig, ObjType obj, char const *name);
static char const *SigDictionary_get(SigDictionary * const me,
                        SigType sig, ObjType obj, char *buf);
static int SigDictionary_find(SigDictionary * const me,
                              SigType sig, ObjType obj);
static void SigDictionary_reset(SigDictionary * const me);
static void SigDictionary_write(SigDictionary const * const me, FILE *stream);
static bool SigDictionary_read(SigDictionary * const me, FILE *stream);

/*..........................................................................*/
static DictEntry     l_funSto[512];
static DictEntry     l_objSto[256];
static DictEntry     l_mscSto[64];
static DictEntry     l_usrSto[128 + 1 - QS_USER];
static SigDictEntry  l_sigSto[512];
static Dictionary    l_funDict;
static Dictionary    l_objDict;
static Dictionary    l_mscDict;
static Dictionary    l_usrDict;
static SigDictionary l_sigDict;

/*..........................................................................*/
static QSpyConfig    l_config;
static FILE         *l_matFile;
static FILE         *l_mscFile;
static QSPY_CustParseFun l_custParseFun;
static int           l_linePos;

static uint8_t       l_txTargetSeq;    /* transmit Target sequence number */
static char          l_dictName[32];   /* dictionary file name */
static char const *  l_qs_rx_rec[] = { /* QS_RX record names... */
    "QS_RX_INFO",
    "QS_RX_COMMAND",
    "QS_RX_RESET",
    "QS_RX_TICK",
    "QS_RX_PEEK",
    "QS_RX_POKE",
    "QS_RX_RESERVED7",
    "QS_RX_RESERVED6",
    "QS_RX_RESERVED5",
    "QS_RX_RESERVED4",
    "QS_RX_GLB_FILTER",
    "QS_RX_LOC_FILTER",
    "QS_RX_AO_FILTER",
    "QS_RX_RESERVED3",
    "QS_RX_RESERVED2",
    "QS_RX_RESERVED1",
    "QS_RX_EVENT"
};

#define SNPRINF_LINE(format_, ...) \
    SNPRINTF_S(QSPY_line, (sizeof(QSPY_line)-1), format_,  ##__VA_ARGS__)

#define SNPRINF_AT(format_, ...) do { \
    int n = SNPRINTF_S(&QSPY_line[l_linePos], \
                       (sizeof(QSPY_line) - 1U - l_linePos), \
                       format_, ##__VA_ARGS__); \
    if ((0 < n) && (n < (int)sizeof(QSPY_line) - 1 - l_linePos)) { \
        l_linePos += n; \
    } \
} while (0)

/*..........................................................................*/
void QSPY_config(uint16_t version,
                 uint8_t objPtrSize,
                 uint8_t funPtrSize,
                 uint8_t tstampSize,
                 uint8_t sigSize,
                 uint8_t evtSize,
                 uint8_t queueCtrSize,
                 uint8_t poolCtrSize,
                 uint8_t poolBlkSize,
                 uint8_t tevtCtrSize,
                 void   *matFile,
                 void   *mscFile,
                 QSPY_CustParseFun custParseFun)
{
    l_config.version      = version;
    l_config.objPtrSize   = objPtrSize;
    l_config.funPtrSize   = funPtrSize;
    l_config.tstampSize   = tstampSize;
    l_config.sigSize      = sigSize;
    l_config.evtSize      = evtSize;
    l_config.queueCtrSize = queueCtrSize;
    l_config.poolCtrSize  = poolCtrSize;
    l_config.poolBlkSize  = poolBlkSize;
    l_config.tevtCtrSize  = tevtCtrSize;
    l_matFile      = (FILE *)matFile;
    l_mscFile      = (FILE *)mscFile;
    l_custParseFun = custParseFun;

    if (l_mscFile != (FILE *)0) {
        /* reserve space at the beginning of the MscGen file for the header,
        * which is known only at the end of the run in qsStop().
        */
        fprintf(l_mscFile,
            "                                        \n"
            "                                        \n"
            "                                        \n"
            "                                        \n"
            "                                        \n"
            "                                        \n"
            "                                        \n"
            "                                        \n");
    }

    Dictionary_ctor(&l_funDict, l_funSto,
                    sizeof(l_funSto)/sizeof(l_funSto[0]));
    Dictionary_ctor(&l_objDict, l_objSto,
                    sizeof(l_objSto)/sizeof(l_objSto[0]));
    Dictionary_ctor(&l_mscDict, l_mscSto,
                    sizeof(l_mscSto)/sizeof(l_mscSto[0]));
    Dictionary_ctor(&l_usrDict, l_usrSto,
                    sizeof(l_usrSto)/sizeof(l_usrSto[0]));
    SigDictionary_ctor(&l_sigDict, l_sigSto,
                       sizeof(l_sigSto)/sizeof(l_sigSto[0]));
    Dictionary_config(&l_funDict, l_config.funPtrSize);
    Dictionary_config(&l_objDict, l_config.objPtrSize);
    Dictionary_config(&l_usrDict, 1);
    SigDictionary_config(&l_sigDict, l_config.objPtrSize);

    SNPRINF_LINE("-v%d", (unsigned)version);       QSPY_onPrintLn();
    SNPRINF_LINE("-T%d", (unsigned)tstampSize);    QSPY_onPrintLn();
    SNPRINF_LINE("-O%d", (unsigned)objPtrSize);    QSPY_onPrintLn();
    SNPRINF_LINE("-F%d", (unsigned)l_config.funPtrSize); QSPY_onPrintLn();
    SNPRINF_LINE("-S%d", (unsigned)sigSize);       QSPY_onPrintLn();
    SNPRINF_LINE("-E%d", (unsigned)evtSize);       QSPY_onPrintLn();
    SNPRINF_LINE("-Q%d", (unsigned)queueCtrSize);  QSPY_onPrintLn();
    SNPRINF_LINE("-P%d", (unsigned)poolCtrSize);   QSPY_onPrintLn();
    SNPRINF_LINE("-B%d", (unsigned)poolBlkSize);   QSPY_onPrintLn();
    SNPRINF_LINE("-C%d", (unsigned)tevtCtrSize);   QSPY_onPrintLn();
    QSPY_line[0] = '\0'; QSPY_onPrintLn();
}
/*..........................................................................*/
void QSPY_configMatFile(void *matFile) {
    if (l_matFile != (FILE *)0) {
        fclose(l_matFile);
    }
    l_matFile = (FILE *)matFile;
}
/*..........................................................................*/
void QSPY_configMscFile(void *mscFile) {
    if (l_mscFile != (FILE *)0) {
        int i;
        fprintf(l_mscFile, "}\n");
        rewind(l_mscFile);
        fprintf(l_mscFile, "msc {\n");
        for (i = 0; ; ++i) {
            char const *entry = Dictionary_at(&l_mscDict, i);
            if (entry[0] != '\0') {
                if (i == 0) {
                    fprintf(l_mscFile, "\"%s\"", entry);
                }
                else {
                    fprintf(l_mscFile, ",\"%s\"", entry);
                }
            }
            else {
                break;
            }
        }
        fprintf(l_mscFile, ";\n");
        fclose(l_mscFile);
    }
    l_mscFile = (FILE *)mscFile;
}
/*..........................................................................*/
QSpyConfig const *QSPY_getConfig(void) {
    return &l_config;
}
/*..........................................................................*/
void QSpyRecord_init(QSpyRecord * const me,
                     uint8_t const *start, uint32_t tot_len)
{
    me->start   = start;
    me->tot_len = tot_len;
    me->pos     = start + 2;
    me->len     = tot_len - 3U;
    me->rec     = start[1];
}
/*..........................................................................*/
QSpyStatus QSpyRecord_OK(QSpyRecord * const me) {
    if (me->len != (uint8_t)0) {
        SNPRINF_LINE("********** %03d: Error %d bytes unparsed",
               (int)me->rec, (int)me->len);
        QSPY_onPrintLn();
        return QSPY_ERROR;
    }
    return QSPY_SUCCESS;
}
/*..........................................................................*/
uint32_t QSpyRecord_getUint32(QSpyRecord * const me, uint8_t size) {
    uint32_t ret = (uint32_t)0;

    if (me->len >= size) {
        if (size == (uint8_t)1) {
            ret = (uint32_t)me->pos[0];
        }
        else if (size == (uint8_t)2) {
            ret = (((uint32_t)me->pos[1] << 8) | (uint32_t)me->pos[0]);
        }
        else if (size == (uint8_t)4) {
            ret = ((((((uint32_t)me->pos[3] << 8)
                        | (uint32_t)me->pos[2]) << 8)
                          | (uint32_t)me->pos[1]) << 8)
                            | (uint32_t)me->pos[0];
        }
        else {
            Q_ASSERT(0);
        }
        me->pos += size;
        me->len -= size;
    }
    else {
        me->len = -1;
        SNPRINF_LINE("Uint overrun");
        QSPY_onPrintLn();
    }
    return ret;
}
/*..........................................................................*/
int32_t QSpyRecord_getInt32(QSpyRecord * const me, uint8_t size) {
    int32_t ret = (int32_t)0;

    if (me->len >= size) {
        if (size == (uint8_t)1) {
            ret = (uint32_t)me->pos[0];
            ret <<= 24;
            ret >>= 24; /* sign-extend */
        }
        else if (size == (uint8_t)2) {
            ret = ((uint32_t)me->pos[1] << 8)
                        | (uint32_t)me->pos[0];
            ret <<= 16;
            ret >>= 16; /* sign-extend */
        }
        else if (size == (uint8_t)4) {
            ret = ((((((int32_t)me->pos[3] << 8)
                        | (uint32_t)me->pos[2]) << 8)
                          | (uint32_t)me->pos[1]) << 8)
                            | (uint32_t)me->pos[0];
        }
        else {
            Q_ASSERT(0);
        }
        me->pos += size;
        me->len -= size;
    }
    else {
        me->len = -1;
        SNPRINF_LINE("Uint overrun");
        QSPY_onPrintLn();
    }
    return ret;
}
/*..........................................................................*/
uint64_t QSpyRecord_getUint64(QSpyRecord * const me, uint8_t size) {
    uint64_t ret = (uint64_t)0;

    if (me->len >= size) {
        if (size == (uint8_t)1) {
            ret = (uint64_t)me->pos[0];
        }
        else if (size == (uint8_t)2) {
            ret = (((uint64_t)me->pos[1] << 8)
                       | (uint32_t)me->pos[0]);
        }
        else if (size == (uint8_t)4) {
            ret = ((((((uint64_t)me->pos[3] << 8)
                        | (uint64_t)me->pos[2]) << 8)
                          | (uint64_t)me->pos[1]) << 8)
                            | (uint64_t)me->pos[0];
        }
        else if (size == (uint8_t)8) {
            ret = ((((((((((((((uint64_t)me->pos[7] << 8)
                        | (uint64_t)me->pos[6]) << 8)
                          | (uint64_t)me->pos[5]) << 8)
                            | (uint64_t)me->pos[4]) << 8)
                              | (uint64_t)me->pos[3]) << 8)
                                | (uint64_t)me->pos[2]) << 8)
                                  | (uint64_t)me->pos[1]) << 8)
                                    | (uint64_t)me->pos[0];
        }
        else {
            Q_ASSERT(0);
        }
        me->pos += size;
        me->len -= size;
    }
    else {
        me->len = -1;
        SNPRINF_LINE("Uint overrun");
        QSPY_onPrintLn();
    }
    return ret;
}
/*..........................................................................*/
int64_t QSpyRecord_getInt64(QSpyRecord * const me, uint8_t size) {
    int64_t ret = (int64_t)0;

    if (me->len >= size) {
        if (size == (uint8_t)1) {
            ret = (uint64_t)me->pos[0];
            ret <<= 56;
            ret >>= 56; /* sign-extend */
        }
        else if (size == (uint8_t)2) {
            ret = (((uint64_t)me->pos[1] << 8)
                       | (uint32_t)me->pos[0]);
            ret <<= 48;
            ret >>= 48; /* sign-extend */
        }
        else if (size == (uint8_t)4) {
            ret = ((((((uint64_t)me->pos[3] << 8)
                        | (uint64_t)me->pos[2]) << 8)
                          | (uint64_t)me->pos[1]) << 8)
                            | (uint64_t)me->pos[0];
            ret <<= 32;
            ret >>= 32; /* sign-extend */
        }
        else if (size == (uint8_t)8) {
            ret = ((((((((((((((uint64_t)me->pos[7] << 8)
                        | (uint64_t)me->pos[6]) << 8)
                          | (uint64_t)me->pos[5]) << 8)
                            | (uint64_t)me->pos[4]) << 8)
                              | (uint64_t)me->pos[3]) << 8)
                                | (uint64_t)me->pos[2]) << 8)
                                  | (uint64_t)me->pos[1]) << 8)
                                    | (uint64_t)me->pos[0];
        }
        else {
            Q_ASSERT(0);
        }
        me->pos += size;
        me->len -= size;
    }
    else {
        me->len = -1;
        SNPRINF_LINE("Int overrun");
        QSPY_onPrintLn();
    }
    return ret;
}
/*..........................................................................*/
char const *QSpyRecord_getStr(QSpyRecord * const me) {
    uint8_t const *p;
    int32_t l;

    for (l = me->len, p = me->pos; l > 0; --l, ++p) {
        if (*p == (uint8_t)0) {
            char const *s = (char const *)me->pos;
            me->len = l - 1;
            me->pos = p + 1;
            return s;
        }
    }
    me->len = -1;
    SNPRINF_LINE("String overrun");
    QSPY_onPrintLn();
    return "";
}
/*..........................................................................*/
uint8_t const *QSpyRecord_getMem(QSpyRecord * const me, uint32_t *pLen) {
    if ((me->len >= 1) && ((*me->pos) <= me->len)) {
        uint8_t const *mem = me->pos + 1;
        *pLen = *me->pos;
        me->len -= 1 + *me->pos;
        me->pos += 1 + *me->pos;
        return mem;
    }

    me->len = -1;
    *pLen = (uint8_t)0;
    SNPRINF_LINE("Mem overrun");
    QSPY_onPrintLn();

    return (uint8_t *)0;
}
/*..........................................................................*/
static void QSpyRecord_processUser(QSpyRecord * const me) {
    uint8_t  fmt;
    int64_t  i64;
    uint64_t u64;
    int32_t  i32;
    uint32_t u32;
    static char const *ifmt[] = {
        "%li ",   "%1li ",  "%2li ",  "%3li ",
        "%4li ",  "%5li ",  "%6li ",  "%7li ",
        "%8li ",  "%9li ",  "%10li ", "%11li ",
        "%12li ", "%13li ", "%14li ", "%15li "
    };
    static char const *ufmt[] = {
        "%lu ",   "%1lu ",  "%2lu ",  "%3lu ",
        "%4lu ",  "%5lu ",  "%6lu ",  "%7lu ",
        "%8lu ",  "%9lu ",  "%10lu ", "%11lu ",
        "%12lu ", "%13lu ", "%14lu ", "%15lu "
    };
    static char const *uhfmt[] = {
        "0x%0lX ",   "0x%01lX ",  "0x%02lX ",  "0x%03lX ",
        "0x%04lX ",  "0x%05lX ",  "0x%06lX ",  "0x%07lX ",
        "0x%08lX ",  "0x%09lX ",  "0x%010lX ", "0x%011lX ",
        "0x%012lX ", "0x%013lX ", "0x%014lX ", "0x%015lX "
    };
    static char const *ilfmt[] = {
        "%2"PRIi64" ",  "%4"PRIi64" ",  "%6"PRIi64" ",  "%8"PRIi64" ",
        "%10"PRIi64" ", "%12"PRIi64" ", "%14"PRIi64" ", "%16"PRIi64" ",
        "%18"PRIi64" ", "%20"PRIi64" ", "%22"PRIi64" ", "%24"PRIi64" ",
        "%26"PRIi64" ", "%28"PRIi64" ", "%30"PRIi64" ", "%32"PRIi64" "
    };
    static char const *ulfmt[] = {
        "%2"PRIu64" ",  "%4"PRIu64" ",  "%6"PRIu64" ",  "%8"PRIu64" ",
        "%10"PRIu64" ", "%12"PRIu64" ", "%14"PRIu64" ", "%16"PRIu64" ",
        "%18"PRIu64" ", "%20"PRIu64" ", "%22"PRIu64" ", "%24"PRIu64" ",
        "%26"PRIu64" ", "%28"PRIu64" ", "%30"PRIu64" ", "%32"PRIu64" "
    };
    static char const *efmt[] = {
        "%7.0Le ",   "%9.1Le ",   "%10.2Le ",  "%11.3Le ",
        "%12.4Le ",  "%13.5Le ",  "%14.6Le ",  "%15.7Le ",
        "%16.8Le ",  "%17.9Le ",  "%18.10Le ", "%19.11Le ",
        "%20.12Le ", "%21.13Le ", "%22.14Le ", "%23.15Le ",
    };

    u32 = QSpyRecord_getUint32(me, l_config.tstampSize);
    l_linePos = 0;
    i32 = Dictionary_find(&l_usrDict, me->rec);
    if (i32 >= 0) {
        SNPRINF_AT("%010u %s: ", u32, Dictionary_at(&l_usrDict, i32));
    }
    else {
        SNPRINF_AT("%010u User%02d: ", u32, (int)me->rec);
    }

    if (l_matFile != (FILE *)0) {
        fprintf(l_matFile, "%3d %10u ", (int)me->rec, u32);
    }

    while (me->len > 0) {
        char const *s;
        fmt = (uint8_t)QSpyRecord_getUint32(me, 1);  /* get the format byte */

        switch (fmt & 0x0F) {
            case QS_I8_T: {
                i32 = QSpyRecord_getInt32(me, 1);
                SNPRINF_AT(ifmt[fmt >> 4], (int)i32);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, ifmt[fmt >> 4], (int)i32);
                }
                break;
            }
            case QS_U8_T: {
                u32 = QSpyRecord_getUint32(me, 1);
                SNPRINF_AT(ufmt[fmt >> 4], (unsigned)u32);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, ufmt[fmt >> 4], (unsigned)u32);
                }
                break;
            }
            case QS_I16_T: {
                i32 = QSpyRecord_getInt32(me, 2);
                SNPRINF_AT(ifmt[fmt >> 4], (int)i32);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, ifmt[fmt >> 4], (int)i32);
                }
                break;
            }
            case QS_U16_T: {
                u32 = QSpyRecord_getUint32(me, 2);
                SNPRINF_AT(ufmt[fmt >> 4], (unsigned)u32);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, ufmt[fmt >> 4], (unsigned)u32);
                }
                break;
            }
            case QS_I32_T: {
                i32 = QSpyRecord_getInt32(me, 4);
                SNPRINF_AT(ifmt[fmt >> 4], (int)i32);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, ifmt[fmt >> 4], (int)i32);
                }
                break;
            }
            case QS_U32_T: {
                u32 = QSpyRecord_getUint32(me, 4);
                SNPRINF_AT(ufmt[fmt >> 4], (unsigned)u32);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, ufmt[fmt >> 4], (unsigned)u32);
                }
                break;
            }
            case QS_U32_HEX_T: {
                u32 = QSpyRecord_getUint32(me, 4);
                SNPRINF_AT(uhfmt[fmt >> 4], (unsigned)u32);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, uhfmt[fmt >> 4], (unsigned)u32);
                }
                break;
            }
            case QS_F32_T: {
                union {
                   uint32_t u;
                   float    f;
                } x;
                x.u = QSpyRecord_getUint32(me, 4);
                SNPRINF_AT(efmt[fmt >> 4], (double)x.f);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, efmt[fmt >> 4], (double)x.f);
                }
                break;
            }
            case QS_F64_T: {
                union F64Rep {
                    uint64_t u;
                    double   d;
                } data;
                data.u = QSpyRecord_getUint64(me, 8);
                SNPRINF_AT(efmt[fmt >> 4], data.d);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, efmt[fmt >> 4], data.d);
                }
                break;
            }
            case QS_STR_T: {
                s = QSpyRecord_getStr(me);
                SNPRINF_AT("%s ", s);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%s ", s);
                }
                break;
            }
            case QS_MEM_T: {
                uint8_t const *mem = QSpyRecord_getMem(me, &u32);
                for (; u32 > (uint32_t)0; --u32, ++mem) {
                    SNPRINF_AT("%02X ", (int)*mem);
                    if (l_matFile != (FILE *)0) {
                        fprintf(l_matFile, "%03d ", (int)*mem);
                    }
                }
                break;
            }
            case QS_SIG_T: {
                u32 = QSpyRecord_getUint32(me, l_config.sigSize);
                u64 = QSpyRecord_getUint64(me, l_config.objPtrSize);
                SNPRINF_AT("%s,Obj=%s ",
                    SigDictionary_get(&l_sigDict, u32, u64, (char *)0),
                    Dictionary_get(&l_objDict, u64, (char *)0));

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%04u %20"PRId64" ", u32, u64);
                }
                break;
            }
            case QS_OBJ_T: {
                u64 = QSpyRecord_getUint64(me, l_config.objPtrSize);
                SNPRINF_AT("%s ",
                    Dictionary_get(&l_objDict, u64, (char *)0));

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%20"PRId64" ", u64);
                }
                break;
            }
            case QS_FUN_T: {
                u64 = QSpyRecord_getUint64(me, l_config.funPtrSize);
                SNPRINF_AT("%s ",
                    Dictionary_get(&l_funDict, u64, (char *)0));

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%20"PRId64" ", u64);
                }
                break;
            }
            case QS_I64_T: {
                i64 = QSpyRecord_getInt64(me, 8);
                SNPRINF_AT(ilfmt[fmt >> 4], i64);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, ilfmt[fmt >> 4], i64);
                }
                break;
            }
            case QS_U64_T: {
                u64 = QSpyRecord_getUint64(me, 8);
                SNPRINF_AT(ulfmt[fmt >> 4], u64);

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, ulfmt[fmt >> 4], u64);
                }
                break;
            }
            default: {
                SNPRINF_AT("Unknown format");
                me->len = -1;
                break;
            }
        }
    }
    QSPY_onPrintLn();
    if (l_matFile != (FILE *)0) {
        fprintf(l_matFile, "\n");
    }
}
/*..........................................................................*/
static void QSpyRecord_process(QSpyRecord * const me) {
    uint32_t t, a, b, c, d, e;
    uint64_t p, q, r;
    char buf[256];
    char const *s = 0;
    char const *w = 0;

    switch (me->rec) {
        /* Session start ...................................................*/
        case QS_EMPTY: {
            if (l_config.version < 550U) {
                if (QSpyRecord_OK(me)) {
                    SNPRINF_LINE("********** RESET %d", (int)l_config.version);
                    QSPY_onPrintLn();

                    Dictionary_reset(&l_funDict);
                    Dictionary_reset(&l_objDict);
                    Dictionary_reset(&l_mscDict);
                    Dictionary_reset(&l_usrDict);
                    SigDictionary_reset(&l_sigDict);
                }
            }
            else {
                /* silently ignore */
            }
            break;
        }

        /* QEP records .....................................................*/
        case QS_QEP_STATE_ENTRY:
            s = "ENTRY";
        case QS_QEP_STATE_EXIT: {
            if (s == 0) s = "EXIT ";
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("           %s: "
                       "Obj=%s State=%s",
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %20"PRId64" %20"PRId64"\n",
                            (int)me->rec, p, q);
                }
            }
            break;
        }
        case QS_QEP_STATE_INIT:
            s = "INIT ";
        case QS_QEP_TRAN_HIST:
            if (s == 0) s = "HIST ";
        case QS_QEP_TRAN_EP:
            if (s == 0) s = "SUB-E";
        case QS_QEP_TRAN_XP: {
            if (s == 0) s = "SUB-X";
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            r = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("           %s: "
                       "Obj=%s Source=%s Target=%s",
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0),
                       Dictionary_get(&l_funDict, r, buf));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %20"PRId64" %20"PRId64" %20"
                                       PRId64"\n",
                            (int)me->rec, p, q, r);
                }
            }
            break;
        }
        case QS_QEP_INIT_TRAN: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u ==>Init: "
                       "Obj=%s New=%s",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %20"PRId64" %20"PRId64"\n",
                            (int)me->rec, t, p, q);
                }
            }
            break;
        }
        case QS_QEP_INTERN_TRAN: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u Intern : "
                       "Obj=%s Sig=%s Source=%s",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %20"PRId64
                            " %20"PRId64"\n",
                            (int)me->rec, t, a, p, q);
                }
            }
            break;
        }
        case QS_QEP_TRAN: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            r = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u ==>Tran: "
                       "Obj=%s Sig=%s Source=%s New=%s",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0),
                       Dictionary_get(&l_funDict, r, buf));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %20"PRId64" %20"PRId64
                                       " %20"PRId64"\n",
                            (int)me->rec, t, a, p, q, r);
                }
                if (l_mscFile != (FILE *)0) {
                    if (Dictionary_find(&l_mscDict, p) >= 0) { /* found? */
                        fprintf(l_mscFile,
                                "\"%s\" rbox \"%s\" [label=\"%s\"];\n",
                                Dictionary_get(&l_mscDict, p, (char *)0),
                                Dictionary_get(&l_mscDict, p, buf),
                                Dictionary_get(&l_funDict, r, (char *)0));
                    }
                }
            }
            break;
        }
        case QS_QEP_IGNORED: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u Ignored: "
                       "Obj=%s Sig=%s Active=%s",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %20"PRId64" %20"PRId64"\n",
                            (int)me->rec, t, a, p, q);
                }
            }
            break;
        }
        case QS_QEP_DISPATCH: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u Disp==>: "
                       "Obj=%s Sig=%s Active=%s",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %20"PRId64" %20"PRId64"\n",
                            (int)me->rec, t, a, p, q);
                }
            }
            break;
        }
        case QS_QEP_UNHANDLED: {
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("           ==>UnHd: "
                       "Obj=%s Sig=%s Active=%s",
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       Dictionary_get(&l_funDict, q, (char *)0));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %4u %20"PRId64" %20"PRId64"\n",
                            (int)me->rec, a, p, q);
                }
            }
            break;
        }

        /* QF records ......................................................*/
        case QS_QF_ACTIVE_ADD:
            s = "ADD";
        case QS_QF_ACTIVE_REMOVE: {
            if (s == 0) s = "REM";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            a = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u AO.%s : "
                       "Active=%s Prio=%2u",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       a);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %20"PRId64" %2u\n",
                            (int)me->rec, t, p, a);
                }
            }
            break;
        }
        case QS_QF_ACTIVE_SUBSCRIBE:
            s = "SUB";
        case QS_QF_ACTIVE_UNSUBSCRIBE: {
            if (s == 0) s = "USUB";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u AO.%s: "
                       "Active=%s Sig=%s",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %20"PRId64"\n",
                            (int)me->rec, t, a, p);
                }
            }
            break;
        }
        case QS_QF_ACTIVE_POST_FIFO:
            s = "FIFO";
        case QS_QF_ACTIVE_POST_ATTEMPT: {
            if (s == 0) s = "ATT!";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            if (l_config.version < 420U) {
                q = (uint64_t)0;
            }
            else {
                q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            }
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            e = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u AO.%s: Sndr=%s Obj=%s "
                       "Evt(Sig=%s, Pool=%2u, Ref=%2u) "
                       "Queue(nFree=%3u, %s=%3u)",
                       t,
                       s,
                       Dictionary_get(&l_objDict, q, (char *)0),
                       Dictionary_get(&l_objDict, p, buf),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       b, c, d,
                       (me->rec == QS_QF_ACTIVE_POST_FIFO ? "nMin" : "mrgn"),
                       e);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile,
                            "%3d %10u %20"PRId64" %4u %20"PRId64
                            " %3u %3u %4u %4u\n",
                            (int)me->rec, t, q, a, p, b, c, d, e);
                }
                if (l_mscFile != (FILE *)0) {
                    if (Dictionary_find(&l_mscDict, q) < 0) { /* not found? */
                        Dictionary_put(&l_mscDict, q,
                            Dictionary_get(&l_objDict, q, (char *)0));
                    }
                    if (Dictionary_find(&l_mscDict, p) < 0) { /* not found? */
                        Dictionary_put(&l_mscDict, p,
                            Dictionary_get(&l_objDict, p, (char *)0));
                    }
                    fprintf(l_mscFile,
                            "\"%s\"->\"%s\" [label=\"%u:%s\"];\n",
                            Dictionary_get(&l_mscDict, q, (char *)0),
                            Dictionary_get(&l_mscDict, p, buf),
                            t,
                            SigDictionary_get(&l_sigDict, a, p, (char *)0));
                }
            }
            break;
        }
        case QS_QF_ACTIVE_POST_LIFO: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            e = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u AO.LIFO: Obj=%s "
                       "Evt(Sig=%s, Pool=%2u, Ref=%2u) "
                       "Queue(nFree=%3u, nMin=%3u)",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       b, c, d, e);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile,
                            "%3d %10u %4u %20"PRId64" %3u %3u %4u %4u\n",
                            (int)me->rec, t, a, p, b, c, d, e);
                }
                if (l_mscFile != (FILE *)0) {
                    if (Dictionary_find(&l_mscDict, p) < 0) { /* not found? */
                        Dictionary_put(&l_mscDict, p,
                            Dictionary_get(&l_objDict, p, (char *)0));
                    }
                    fprintf(l_mscFile,
                            "\"%s\"->\"%s\" [label=\"%u:%s-LIFO\"];\n",
                            Dictionary_get(&l_mscDict, p, (char *)0),
                            Dictionary_get(&l_mscDict, p, buf),
                            t,
                            SigDictionary_get(&l_sigDict, a, p, (char *)0));
                }
            }
            break;
        }
        case QS_QF_ACTIVE_GET:
            s = "AO.GET : Active=";
        case QS_QF_EQUEUE_GET: {
            if (s == 0) s = "EQ.GET : EQueue=";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u %s "
                       "%s Evt(Sig=%s, Pool=%2u, Ref=%2u) "
                       "Queue(nUsed=%3u)",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       b, c, d);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %20"PRId64" %3u %3u %4u\n",
                            (int)me->rec, t, a, p, b, c, d);
                }
            }
            break;
        }
        case QS_QF_ACTIVE_GET_LAST:
            s = "AO.GETL: Active=";
        case QS_QF_EQUEUE_GET_LAST: {
            if (s == 0) s = "EQ.GETL: EQueue=";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u %s "
                       "%s Evt(Sig=%s, Pool=%2u, Ref=%2u)",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       b, c);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %20"PRId64" %3u %3u\n",
                            (int)me->rec, t, a, p, b, c);
                }
            }
            break;
        }
        case QS_QF_EQUEUE_INIT: {
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            b = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("           EQ.INIT: Obj=%s Len=%2u",
                       Dictionary_get(&l_objDict, p, (char *)0),
                       b);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %20"PRId64" %4u\n",
                            (int)me->rec, p, b);
                }
            }
            break;
        }
        case QS_QF_EQUEUE_POST_FIFO:
            s = "FIFO";
            w = "nMin";
        case QS_QF_EQUEUE_POST_ATTEMPT:
            if (s == 0) s = "ATT!";
            if (w == 0) w = "mrgn";
        case QS_QF_EQUEUE_POST_LIFO: {
            if (s == 0) s = "LIFO";
            if (w == 0) w = "nMin";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            e = QSpyRecord_getUint32(me, l_config.queueCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u EQ.%s: Obj=%s "
                       "Evt(Sig=%s, Pool=%2u, Ref=%2u) "
                       "Queue(nFree=%3u, %s=%3u)",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, p, (char *)0),
                       b, c, d,
                       w,
                       e);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %20"PRId64
                                       " %3u %3u %4u %4u\n",
                            (int)me->rec, t, a, p,
                            b, c, d, e);
                }
            }
            break;
        }
        case QS_QF_MPOOL_INIT: {
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            b = QSpyRecord_getUint32(me, l_config.poolCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("           MP.INIT: Obj=%s nBlocks=%4u",
                       Dictionary_get(&l_objDict, p, (char *)0),
                       b);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %20"PRId64" %4u\n",
                            (int)me->rec, p, b);
                }
            }
            break;
        }
        case QS_QF_MPOOL_GET:
            s = "GET ";
            w = "nMin";
        case QS_QF_MPOOL_GET_ATTEMPT: {
            if (s == 0) s = "ATT!";
            if (w == 0) w = "mrgn";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            b = QSpyRecord_getUint32(me, l_config.poolCtrSize);
            c = QSpyRecord_getUint32(me, l_config.poolCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u MP.%s: Obj=%s "
                       "nFree=%4u %s=%4u",
                       t,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       b,
                       w,
                       c);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %20"PRId64" %4u %4u\n",
                            (int)me->rec, t, p, b, c);
                }
            }
            break;
        }
        case QS_QF_MPOOL_PUT: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            b = QSpyRecord_getUint32(me, l_config.poolCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u MP.PUT : Obj=%s "
                       "nFree=%4u",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       b);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %20"PRId64" %4u\n",
                            (int)me->rec, t, p, b);
                }
            }
            break;
        }
        case QS_QF_PUBLISH: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            if (l_config.version < 420U) {
                p = (uint64_t)0;
                a = QSpyRecord_getUint32(me, l_config.sigSize);
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                p = QSpyRecord_getUint64(me, l_config.objPtrSize);
                a = QSpyRecord_getUint32(me, l_config.sigSize);
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u PUBLISH: Sndr=%s "
                       "Evt(Sig=%s, Pool=%2u, Ref=%2u)",
                       t,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, 0, (char *)0),
                       b, c);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %20"PRId64" %4u %10u\n",
                            (int)me->rec, t, p, a, b);
                }
                if (l_mscFile != (FILE *)0) {
                    if (Dictionary_find(&l_mscDict, p) < 0) { /* not found? */
                        Dictionary_put(&l_mscDict, p,
                            Dictionary_get(&l_objDict, p, (char *)0));
                    }
                    fprintf(l_mscFile,
                            "\"%s\"->* [label=\"%u:%s\""
                            ",textcolour=\"#0000ff\""
                            ",linecolour=\"#0000ff\"];\n",
                            Dictionary_get(&l_mscDict, p, (char *)0),
                            t,
                            SigDictionary_get(&l_sigDict, a, p, (char *)0));
                }
            }
            break;
        }
        case QS_QF_NEW: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.evtSize);
            c = QSpyRecord_getUint32(me, l_config.sigSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u NEW    : Evt(Sig=%s, size=%5u)",
                       t,
                       SigDictionary_get(&l_sigDict, c, 0, (char *)0),
                       a);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %4u\n",
                            (int)me->rec, t, a, c);
                }
            }
            break;
        }
        case QS_QF_GC_ATTEMPT:
            s = "GC-ATT";
        case QS_QF_GC: {
            if (s == 0) s = "GC    ";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            if (l_config.version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u %s : Evt(Sig=%s, Pool=%1d, Ref=%2d)",
                       t,
                       s,
                       SigDictionary_get(&l_sigDict, a, 0, (char *)0),
                       b, c);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %3u %3u\n",
                            (int)me->rec, t, a, b, c);
                }
            }
            break;
        }
        case QS_QF_TICK: {
            a = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("           TICK[%1u]: Ctr=%10u",
                        b,
                        a);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u\n", (int)me->rec, a);
                }
                if (l_mscFile != (FILE *)0) {
                    fprintf(l_mscFile,
                            "--- [label=\"tick %u\""
                            ",textcolour=\"#ff0000\""
                            ",linecolour=\"#ff0000\"];\n",
                            a);
                }
            }
            break;
        }
        case QS_QF_TIMEEVT_ARM:
            s = "ARM ";
        case QS_QF_TIMEEVT_DISARM: {
            if (s == 0) s = "DARM";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            c = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            d = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u TE[%1u].%s: Obj=%s Act=%s "
                       "nTicks=%4u Interval=%4u",
                       t,
                       b,
                       s,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_objDict, q, buf),
                       c, d);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %20"PRId64" %20"PRId64
                            " %10u %10u\n",
                            (int)me->rec, t, p, q, c, d);
                }
            }
            break;
        }
        case QS_QF_TIMEEVT_AUTO_DISARM: {
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("           TE[%1u].ADRM: Obj=%s Act=%s",
                       b,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_objDict, q, buf));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %20"PRId64" %20"PRId64"\n",
                            (int)me->rec, p, q);
                }
           }
            break;
        }
        case QS_QF_TIMEEVT_DISARM_ATTEMPT: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u TE[%1u].DATT: Obj=%s Act=%s",
                       t,
                       b,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_objDict, q, buf));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %20"PRId64" %20"PRId64"\n",
                            (int)me->rec, t, p, q);
                }
            }
            break;
        }
        case QS_QF_TIMEEVT_REARM: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            c = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            d = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            e = QSpyRecord_getUint32(me, 1);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u TE[%1u].RARM: Obj=%s Act=%s "
                       "nTicks=%4u Interval=%4u WasArmed=%1u",
                       t,
                       b,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_objDict, q, buf),
                       c, d, e);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile,
                            "%3d %10u %20"PRId64" %20"PRId64" %10u %10u %1u\n",
                            (int)me->rec, t, p, q, c, d, e);
                }
            }
            break;
        }
        case QS_QF_TIMEEVT_POST: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u TE[%1u].POST: Obj=%s Sig=%s Act=%s",
                       t,
                       b,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       SigDictionary_get(&l_sigDict, a, q, (char *)0),
                       Dictionary_get(&l_objDict, q, buf));
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %20"PRId64" %4u %20"PRId64"\n",
                            (int)me->rec, t, p, a, q);
                }
            }
            break;
        }
        case QS_QF_TIMEEVT_CTR: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            q = QSpyRecord_getUint64(me, l_config.objPtrSize);
            c = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            d = QSpyRecord_getUint32(me, l_config.tevtCtrSize);
            if (l_config.version < 500U) {
                b = 0U;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
            }
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u TE[%1u].CTR: Obj=%s Act=%s "
                       "nTicks=%4u Interval=%4u",
                       t,
                       b,
                       Dictionary_get(&l_objDict, p, (char *)0),
                       Dictionary_get(&l_objDict, q, buf),
                       c, d);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %20"PRId64" %20"PRId64
                            " %10u %10u\n",
                            (int)me->rec, t, p, q, c, d);
                }
            }
            break;
        }
        case QS_QF_CRIT_ENTRY:
            s = "QF_critE";
        case QS_QF_CRIT_EXIT: {
            if (s == 0) s = "QF_critX";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u %s: "
                       "CritNest=%2d",
                       t,
                       s,
                       a);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %3u\n",
                            (int)me->rec, t, a);
                }
           }
            break;
        }
        case QS_QF_ISR_ENTRY:
            s = "QF_isrE";
        case QS_QF_ISR_EXIT: {
            if (s == 0) s = "QF_isrX";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u %s: "
                       "IsrNest=%2u, CurrPrio=%3u",
                       t,
                       s,
                       a, b);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %3u %3u\n",
                            (int)me->rec, t, a, b);
                }
            }
            break;
        }

        /* built-in scheduler records ......................................*/
        case QS_SCHED_LOCK:
            if (s == 0) s = "MuxLOCK";
        case QS_SCHED_UNLOCK: {
            if (s == 0) s = "MuxUNLK";
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u %s: "
                       "OrgPrio=%2u, CurrPrio=%3u",
                       t,
                       s,
                       a, b);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %3u %3u\n",
                            (int)me->rec, t, a, b);
                }
            }
            break;
        }
        case QS_SCHED_NEXT: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u ScheNext: "
                       "prio=%2u, pprev=%3u",
                       t, a, b);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %3u %3u\n",
                            (int)me->rec, t, a, b);
                }
            }
            break;
        }
        case QS_SCHED_IDLE: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u SchedIDLE: "
                       "pprev=%2u",
                       t, a);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %3u\n",
                            (int)me->rec, t, a);
                }
            }
            break;
        }
        case QS_SCHED_RESUME: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u SchedRES: "
                       "prio=%2u, pprev=%3u",
                       t, a, b);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %3u %3u\n",
                            (int)me->rec, t, a, b);
                }
            }
            break;
        }

        /* Miscallaneous QS records ........................................*/
        case QS_SIG_DICT: {
            a = QSpyRecord_getUint32(me, l_config.sigSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            s = QSpyRecord_getStr(me);
            if (s[0] == '&') {
                ++s;
            }
            if (QSpyRecord_OK(me)) {
                SigDictionary_put(&l_sigDict, (SigType)a, p, s);
                SNPRINF_LINE("           Sig-Dic: %08X,Obj=%016"PRIX64"->%s",
                        a, p, s);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %s=[%4u %20"PRId64"];\n",
                        (int)me->rec, getMatDict(s), a, p);
                }
            }
            break;
        }
        case QS_OBJ_DICT: {
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            s = QSpyRecord_getStr(me);
            if (s[0] == '&') {
                ++s;
            }
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&l_objDict, p, s);
                SNPRINF_LINE("           Obj-Dic: %016"PRIX64"->%s",
                        p, s);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %s=%20"PRId64";\n",
                            (int)me->rec, getMatDict(s), p);
                }
            }
            break;
        }
        case QS_FUN_DICT: {
            p = QSpyRecord_getUint64(me, l_config.funPtrSize);
            s = QSpyRecord_getStr(me);
            if (s[0] == '&') {
                ++s;
            }
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&l_funDict, p, s);
                SNPRINF_LINE("           Fun-Dic: %016"PRIX64"->%s",
                        p, s);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %s=%20"PRId64";\n",
                            (int)me->rec, getMatDict(s), p);
                }
            }
            break;
        }

        case QS_USR_DICT: {
            a = QSpyRecord_getUint32(me, 1);
            s = QSpyRecord_getStr(me);
            if (s[0] == '&') {
                ++s;
            }
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&l_usrDict, a, s);
                SNPRINF_LINE("           Usr-Dic: %08X        ->%s",
                        a, s);
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_TARGET_INFO: {
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 2);

            /* extract the data bytes... */
            for (e = 0U; e < 13U; ++e) {
                buf[e] = (char)QSpyRecord_getUint32(me, 1);
            }

            if (QSpyRecord_OK(me)) {

                /* apply the target info... */
                l_config.version      = (uint16_t)(b & 0xFFFFU);
                l_config.objPtrSize   = (uint8_t)(buf[3] & 0x0FU);
                l_config.funPtrSize   = (uint8_t)((buf[3] >> 4) & 0x0FU);
                l_config.tstampSize   = (uint8_t)(buf[4] & 0x0FU);
                l_config.sigSize      = (uint8_t)(buf[0] & 0x0FU);
                l_config.evtSize      = (uint8_t)((buf[0] >> 4) & 0x0FU);
                l_config.queueCtrSize = (uint8_t)(buf[1] & 0x0FU);
                l_config.poolCtrSize  = (uint8_t)((buf[2] >> 4) & 0x0FU);
                l_config.poolBlkSize  = (uint8_t)(buf[2] & 0x0FU);
                l_config.tevtCtrSize  = (uint8_t)((buf[1] >> 4) & 0x0FU);

                for (e = 0U; e < sizeof(l_config.tstamp); ++e) {
                    l_config.tstamp[e] = (uint8_t)buf[7U + e];
                }

                /* set the dictionary file name from the time-stamp... */
                SNPRINTF_S(l_dictName, sizeof(l_dictName),
                       "qspy%02d%02d%02d_%02d%02d%02d.dic",
                       (int)l_config.tstamp[5], (int)l_config.tstamp[4],
                       (int)l_config.tstamp[3], (int)l_config.tstamp[2],
                       (int)l_config.tstamp[1], (int)l_config.tstamp[0]);

                SNPRINF_LINE("********** TARGET: QP-Ver:%4d, "
                       "build tstamp:%02d%02d%02d_%02d%02d%02d",
                       b,
                       (int)l_config.tstamp[5], (int)l_config.tstamp[4],
                       (int)l_config.tstamp[3], (int)l_config.tstamp[2],
                       (int)l_config.tstamp[1], (int)l_config.tstamp[0]);
                QSPY_onPrintLn();

                if (a != 0U) {  /* is this also Target RESET? */
                    l_txTargetSeq = (uint8_t)0;
                    Dictionary_reset(&l_funDict);
                    Dictionary_reset(&l_objDict);
                    Dictionary_reset(&l_mscDict);
                    Dictionary_reset(&l_usrDict);
                    SigDictionary_reset(&l_sigDict);
                    /*TBD: close and re-open MATLAB file, MSC file, etc. */
                }
                else {
                    FILE *dictFile = (FILE *)0;

                    /* do we have dictionary name yet? */
                    if (l_dictName[0] != '\0') {
                        FOPEN_S(dictFile, l_dictName, "r");
                        if (dictFile != (FILE *)0) {
                            if (QSPY_readDict(dictFile) != QSPY_ERROR) {
                                SNPRINF_LINE(
                                    "           Dictionaries read from %s",
                                    l_dictName);
                                QSPY_onPrintLn();
                            }
                            fclose(dictFile);
                        }
                    }
                }
            }
            else {
                l_dictName[0] = '\0';
            }
            break;
        }

        case QS_RX_STATUS: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 1U);
            if (QSpyRecord_OK(me)) {
                if (a < 128U) { /* success? */
                    if (a < sizeof(l_qs_rx_rec)/sizeof(l_qs_rx_rec[0])) {
                        SNPRINF_LINE("%010u RX ack: %s",
                                     t, l_qs_rx_rec[a]);
                    }
                    else {
                        SNPRINF_LINE("%010u RX ack: %d",
                                     t, a);
                    }
                }
                else { /* error */
                    SNPRINF_LINE("%010u RX err: 0x%02X",
                                 t, (a & 0x7F));
                }
                QSPY_onPrintLn();
            }
            break;
        }
        case QS_TEST_STATUS: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            s = QSpyRecord_getStr(me);
            w = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u TEST: File=%s Test=%s",
                       t, s, w);
                QSPY_onPrintLn();
            }
            break;
        }
        case QS_PEEK_DATA: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            p = QSpyRecord_getUint64(me, l_config.objPtrSize);
            w = (char const *)QSpyRecord_getMem(me, &a);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u PEEK: Addr=%016"PRIX64" len=%d",
                             t, p, a);
                QSPY_onPrintLn();
                l_linePos = 0U;
                SNPRINF_AT("          ");
                for (b = 15U; a > (uint32_t)0; --a, --b, ++w) {
                    SNPRINF_AT(" %02X", (int)(*w & 0xFFU));
                    if (b == 0U) {
                        b = 16U;
                        QSPY_onPrintLn();
                        l_linePos = 0U;
                        SNPRINF_AT("          ");
                    }
                }
                QSPY_onPrintLn();
            }
            break;
        }
        case QS_ASSERT_FAIL: {
            t = QSpyRecord_getUint32(me, l_config.tstampSize);
            a = QSpyRecord_getUint32(me, 2);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u !ASSERT: Module=%s Location=%4hu",
                       t, s, a);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %s\n",
                            (int)me->rec, (unsigned)t, (unsigned)a, s);
                }
            }
            break;
        }

        /* User records ....................................................*/
        default: {
            if (me->rec >= QS_USER) {
                QSpyRecord_processUser(me);
            }
            else {
                SNPRINF_LINE("           ???    : Rec=%3d, Len=%3d",
                       (int)me->rec, (int)me->len);
                QSPY_onPrintLn();
            }
            break;
        }
    }
}
/*..........................................................................*/
static char const *getMatDict(char const *s) {
    static char dict[65];
    char *pc = dict;
    while ((*s != '\0') && (pc < &dict[sizeof(dict)] - 1)) {
        if ((*s == '[') || (*s == ']')
            || (*s == '.') || (*s == ':'))
        {
            *pc++ = '_';
            ++s;
        }
        else {
            *pc++ = *s++;
        }
    }
    *pc = '\0';
    return (dict[0] == '&' ? dict+1 : dict);
}

/*..........................................................................*/
void QSPY_stop(void) {
    QSPY_configMatFile((void *)0);
    QSPY_configMscFile((void *)0);
}

/*..........................................................................*/
#define QS_FRAME            ((uint8_t)0x7E)
#define QS_ESC              ((uint8_t)0x7D)
#define QS_GOOD_CHKSUM      ((uint8_t)0xFF)
#define QS_ESC_XOR          ((uint8_t)0x20)

void QSPY_parse(uint8_t const *buf, uint32_t nBytes) {
    static uint8_t record[QS_MAX_RECORD_SIZE];
    static uint8_t *pos   = record; /* position within the record */
    static uint8_t chksum = (uint8_t)0;
    static uint8_t esc    = (uint8_t)0;
    static uint8_t seq    = (uint8_t)0;

    for (; nBytes != 0U; --nBytes) {
        uint8_t b = *buf++;

        if (esc) {                /* escaped byte arrived? */
            esc = (uint8_t)0;
            b ^= QS_ESC_XOR;

            chksum = (uint8_t)(chksum + b);
            if (pos < &record[sizeof(record)]) {
                *pos++ = b;
            }
            else {
                SNPRINF_LINE("********** Error, record too long");
                QSPY_onPrintLn();
                chksum = (uint8_t)0;
                pos = record;
                esc = (uint8_t)0;
            }
        }
        else if (b == QS_ESC) {   /* transparent byte? */
            esc = (uint8_t)1;
        }
        else if (b == QS_FRAME) { /* frame byte? */
            if (chksum != QS_GOOD_CHKSUM) {
                SNPRINF_LINE("********** Bad checksum at seq=%3u, id=%3u",
                       (unsigned)seq, (unsigned)record[1]);
                QSPY_onPrintLn();
                if (l_mscFile != (FILE *)0) {
                    fprintf(l_mscFile, "...;\n"
                        "--- [label=\"Bad checksum at seq=%3u, id=%3u\""
                        ",textbgcolour=\"#ffff00\""
                        ",linecolour=\"#ff0000\"];\n",
                    (unsigned)seq, (unsigned)record[1]);
                }
            }
            else if (pos < &record[3]) {
                SNPRINF_LINE("********** Record too short at seq=%3u, id=%3u",
                       (unsigned)seq, (unsigned)record[1]);
                QSPY_onPrintLn();
                if (l_mscFile != (FILE *)0) {
                    fprintf(l_mscFile, "...;\n"
                    "--- [label=\"Record too short at seq=%3u, id=%3u\""
                        ",textbgcolour=\"#ffff00\""
                        ",linecolour=\"#ff0000\"];\n",
                    (unsigned)seq, (unsigned)record[1]);
                }
            }
            else { /* a healty record received */
                QSpyRecord qrec;
                int parse = 1;
                ++seq;
                if (seq != record[0]) {
                    SNPRINF_LINE("********** Data discontinuity: "
                                 "seq=%u -> seq=%u",
                           (unsigned)seq, (unsigned)record[0]);
                    QSPY_onPrintLn();
                    if (l_mscFile != (FILE *)0) {
                        fprintf(l_mscFile,
                            "--- [label=\""
                            "Data discontinuity: seq=%u -> seq=%u\""
                            ",textbgcolour=\"#ffff00\""
                            ",linecolour=\"#ff0000\"];\n"
                            "...;\n",
                           (unsigned)seq, (unsigned)record[0]);
                    }
                }
                seq = record[0];

                QSpyRecord_init(&qrec, record, (int32_t)(pos - record));

                if (l_custParseFun != (QSPY_CustParseFun)0) {
                    parse = (*l_custParseFun)(&qrec);
                    if (parse) {
                        /* re-initialize the record for parsing again */
                        QSpyRecord_init(&qrec,
                                        record, (int32_t)(pos - record));
                    }
                }
                if (parse) {
                    QSpyRecord_process(&qrec);
                }
            }

            /* get ready for the next record ... */
            chksum = (uint8_t)0;
            pos = record;
            esc = (uint8_t)0;
        }
        else {  /* a regular un-escaped byte */
            chksum = (uint8_t)(chksum + b);
            if (pos < &record[sizeof(record)]) {
                *pos++ = b;
            }
            else {
                SNPRINF_LINE("********** Error, record too long");
                QSPY_onPrintLn();
                chksum = (uint8_t)0;
                pos = record;
                esc = (uint8_t)0;
            }
        }
    }
}

/*..........................................................................*/
char const *QSPY_writeDict(void) {
    FILE *dictFile = (FILE *)0;

    /* do we have dictionary name yet? */
    if (l_dictName[0] == '\0') {
        return (char *)0;  /* no name yet, can't save dictionaries */
    }

    FOPEN_S(dictFile, l_dictName, "w");
    if (dictFile == (FILE *)0) {
        return (char *)0;
    }

    fprintf(dictFile, "-v%03d\n", l_config.version);
    fprintf(dictFile, "-T%01d\n", l_config.tstampSize);
    fprintf(dictFile, "-O%01d\n", l_config.objPtrSize);
    fprintf(dictFile, "-F%01d\n", l_config.funPtrSize);
    fprintf(dictFile, "-S%01d\n", l_config.sigSize);
    fprintf(dictFile, "-E%01d\n", l_config.evtSize);
    fprintf(dictFile, "-Q%01d\n", l_config.queueCtrSize);
    fprintf(dictFile, "-P%01d\n", l_config.poolCtrSize);
    fprintf(dictFile, "-B%01d\n", l_config.poolBlkSize);
    fprintf(dictFile, "-C%01d\n", l_config.tevtCtrSize);

    fprintf(dictFile, "Obj-Dic:\n");
    Dictionary_write(&l_objDict, dictFile);

    fprintf(dictFile, "Fun-Dic:\n");
    Dictionary_write(&l_funDict, dictFile);

    fprintf(dictFile, "Usr-Dic:\n");
    Dictionary_write(&l_usrDict, dictFile);

    fprintf(dictFile, "Sig-Dic:\n");
    SigDictionary_write(&l_sigDict, dictFile);

    fprintf(dictFile, "Msc-Dic:\n");
    Dictionary_write(&l_mscDict, dictFile);

    fclose(dictFile);
    return l_dictName;
}
/*..........................................................................*/
QSpyStatus QSPY_readDict(void *dictFile) {
    char type[10];

    while (fgets(type, sizeof(type), (FILE *)dictFile) != (char *)0) {
        switch (type[0]) {
            case '-':
                switch (type[1]) {
                    case 'v':
                        l_config.version = 100U*(type[2] - '0')
                                           + 10U*(type[3] - '0')
                                           + (type[4] - '0');
                        printf("-v%03d\n", l_config.version);
                        break;
                    case 'T':
                        l_config.tstampSize = (type[2] - '0');
                        printf("-T%1d\n", l_config.tstampSize);
                        break;
                    case 'O':
                        l_config.objPtrSize = (type[2] - '0');
                        printf("-O%1d\n", l_config.tstampSize);
                        break;
                    case 'F':
                        l_config.funPtrSize = (type[2] - '0');
                        printf("-F%1d\n", l_config.objPtrSize);
                        break;
                    case 'S':
                        l_config.sigSize = (type[2] - '0');
                        printf("-S%1d\n", l_config.sigSize);
                        break;
                    case 'E':
                        l_config.evtSize = (type[2] - '0');
                        printf("-E%1d\n", l_config.evtSize);
                        break;
                    case 'Q':
                        l_config.queueCtrSize = (type[2] - '0');
                        printf("-Q%1d\n", l_config.queueCtrSize);
                        break;
                    case 'P':
                        l_config.poolCtrSize = (type[2] - '0');
                        printf("-P%1d\n", l_config.poolCtrSize);
                        break;
                    case 'B':
                        l_config.poolBlkSize = (type[2] - '0');
                        printf("-B%1d\n", l_config.poolBlkSize);
                        break;
                    case 'C':
                        l_config.tevtCtrSize = (type[2] - '0');
                        printf("-C%1d\n", l_config.tevtCtrSize);
                        break;
                    default:
                        return QSPY_ERROR;
                        break;
                }
                break;
            case 'O':
                if (!Dictionary_read(&l_objDict, (FILE *)dictFile)) {
                    return QSPY_ERROR;
                }
                break;
            case 'F':
                if (!Dictionary_read(&l_funDict, (FILE *)dictFile)) {
                    return QSPY_ERROR;
                }
                break;
            case 'U':
                if (!Dictionary_read(&l_usrDict, (FILE *)dictFile)) {
                    return QSPY_ERROR;
                }
                break;
            case 'S':
                if (!SigDictionary_read(&l_sigDict, (FILE *)dictFile)) {
                    return QSPY_ERROR;
                }
                break;
            case 'M':
                if (!Dictionary_read(&l_mscDict, (FILE *)dictFile)) {
                    return QSPY_ERROR;
                }
                break;
            default:
                return QSPY_ERROR;
                break;
        }
    }

    return QSPY_SUCCESS;
}

/****************************************************************************/
/*! helper macro to insert an un-escaped byte into the QS buffer */
#define QS_INSERT_BYTE(b_) \
    *dst++ = (b_); \
    if ((uint8_t)(dst - &dstBuf[0]) >= dstSize) { \
        return 0U; \
    }

/*! helper macro to insert an escaped byte into the QS buffer */
#define QS_INSERT_ESC_BYTE(b_) \
    chksum = (uint8_t)(chksum + (b_)); \
    if (((b_) != QS_FRAME) && ((b_) != QS_ESC)) { \
        QS_INSERT_BYTE(b_) \
    } \
    else { \
        QS_INSERT_BYTE(QS_ESC) \
        QS_INSERT_BYTE((uint8_t)((b_) ^ QS_ESC_XOR)) \
    }

/*..........................................................................*/
uint32_t QSPY_encode(uint8_t *dstBuf, uint32_t dstSize,
                     uint8_t const *srcBuf, uint32_t srcBytes)
{
    uint8_t chksum = 0U;
    uint8_t b;

    uint8_t *dst = &dstBuf[0];
    uint8_t const *src = &srcBuf[1]; /* skip the sequence from the soruce */

    --srcBytes; /* account for skipping the sequence number in the source */

    /* supply the sequence number */
    ++l_txTargetSeq;
    b = l_txTargetSeq;
    QS_INSERT_ESC_BYTE(b); /* insert esceped sequence into destination */

    for (; srcBytes > 0; ++src, --srcBytes) {
        b = *src;
        QS_INSERT_ESC_BYTE(b) /* insert all escaped bytes into destination */
    }

    b = chksum;
    b ^= 0xFFU;               /* invert the bits of the checksum */
    QS_INSERT_ESC_BYTE(b)     /* insert the escaped checksum */
    QS_INSERT_BYTE(QS_FRAME)  /* insert un-escaped frame */

    return dst - &dstBuf[0];  /* number of bytes in the destination */
}
/*..........................................................................*/
uint32_t QSPY_encodeResetCmd(uint8_t *dstBuf, uint32_t dstSize) {
    static uint8_t const s_QS_RX_RESET[] = { 0x00U, QS_RX_RESET };

    return QSPY_encode(dstBuf, dstSize,
                       s_QS_RX_RESET, sizeof(s_QS_RX_RESET));
}
/*..........................................................................*/
uint32_t QSPY_encodeInfoCmd(uint8_t *dstBuf, uint32_t dstSize) {
    static uint8_t const s_QS_RX_INFO[] = { 0x00U, QS_RX_INFO };

    return QSPY_encode(dstBuf, dstSize,
                       s_QS_RX_INFO, sizeof(s_QS_RX_INFO));
}
/*..........................................................................*/
uint32_t QSPY_encodeTickCmd (uint8_t *dstBuf, uint32_t dstSize, uint8_t rate){
    static uint8_t s_QS_RX_TICK[] = { 0x00U, QS_RX_TICK, 0U };
    s_QS_RX_TICK[2] = rate;
    return QSPY_encode(dstBuf, dstSize,
                       s_QS_RX_TICK, sizeof(s_QS_RX_TICK));
}

/****************************************************************************/
static int Dictionary_comp(void const *arg1, void const *arg2) {
    KeyType key1 = ((DictEntry const *)arg1)->key;
    KeyType key2 = ((DictEntry const *)arg2)->key;
    if (key1 > key2) {
        return 1;
    }
    else if (key1 < key2) {
        return -1;
    }
    else {
        return 0;
    }
}

/*..........................................................................*/
static void Dictionary_ctor(Dictionary * const me,
                            DictEntry *sto, uint32_t capacity)
{
    me->sto      = sto;
    me->capacity = capacity;
    me->entries  = 0;
    me->keySize  = 4;
}
/*..........................................................................*/
static void Dictionary_config(Dictionary * const me, int keySize) {
    me->keySize = keySize;
}
/*..........................................................................*/
static char const *Dictionary_at(Dictionary * const me, unsigned idx) {
    if (idx < (unsigned)me->entries) {
        return me->sto[idx].name;
    }
    else {
        return "";
    }
}
/*..........................................................................*/
static void Dictionary_put(Dictionary * const me,
                           KeyType key, char const *name)
{
    int idx = Dictionary_find(me, key);
    int n = me->entries;
    char *dst;
    if (idx >= 0) { /* the key found? */
        Q_ASSERT((idx <= n) || (n == 0));
        dst = me->sto[idx].name;
        STRNCPY_S(dst, name, sizeof(me->sto[idx].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
    }
    else if (n < me->capacity - 1) {
        me->sto[n].key = key;
        dst = me->sto[n].name;
        STRNCPY_S(dst, name, sizeof(me->sto[n].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
        ++me->entries;
        /* keep the entries sorted by the key */
        qsort(me->sto, (size_t)me->entries, sizeof(me->sto[0]),
              &Dictionary_comp);
    }
}
/*..........................................................................*/
static char const *Dictionary_get(Dictionary * const me,
                                  KeyType key, char *buf)
{
    int idx;
    if (key == 0) {
        return "NULL";
    }
    idx = Dictionary_find(me, key);
    if (idx >= 0) { /* key found? */
        return me->sto[idx].name;
    }
    else { /* key not found */
        if (buf == 0) { /* extra buffer not provided? */
            buf = me->notFound.name; /* use the internal location */
        }
        /* otherwise use the provided buffer... */
        if (me->keySize <= 4) {
            SNPRINTF_S(buf, NAME_LEN, "%08"PRIX64"", key);
        }
        else {
            SNPRINTF_S(buf, NAME_LEN, "%016"PRIX64"", key);
        }
        //Dictionary_put(me, key, buf); /* put into the dictionary */
        return buf;
    }
}
/*..........................................................................*/
static int Dictionary_find(Dictionary * const me, KeyType key) {
    /* binary search algorithm... */
    int mid;
    int first = 0;
    int last = me->entries;
    while (first <= last) {
        mid = (first + last) / 2;
        if (me->sto[mid].key == key) {
            return mid;
        }
        if (me->sto[mid].key > key) {
            last = mid - 1;
        }
        else {
            first = mid + 1;
        }
    }

    return -1; /* entry not found */
}
/*..........................................................................*/
static void Dictionary_reset(Dictionary * const me) {
    me->entries = 0;
}
/*..........................................................................*/
static void Dictionary_write(Dictionary const * const me, FILE *stream) {
    int i;

    fprintf(stream, "%d %d\n", me->entries, me->keySize);
    for (i = 0; i < me->entries; ++i) {
        DictEntry const *e = &me->sto[i];
        fprintf(stream, "%016"PRIX64" %s\n", e->key, e->name);
    }
}
/*..........................................................................*/
static bool Dictionary_read(Dictionary * const me, FILE *stream) {
    char dictLine[80];
    int i;

    if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
        goto error;
    }
    if (SSCANF_S(dictLine, "%d %d\n", &me->entries, &me->keySize) != 2) {
        goto error;
    }
    if ((me->entries > me->capacity) || (me->keySize > 8)) {
        goto error;
    }
    for (i = 0; i < me->entries; ++i) {
        DictEntry *e = &me->sto[i];
        if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
            goto error;
        }
        if (SSCANF_S(dictLine, "%016"PRIX64" %s\n", &e->key, e->name) != 2) {
            goto error;
        }
    }
    return true;

error:
    Dictionary_reset(me);
    return false;
}

/*--------------------------------------------------------------------------*/
static int SigDictionary_comp(void const *arg1, void const *arg2)
{
    SigType sig1 = ((SigDictEntry const *)arg1)->sig;
    SigType sig2 = ((SigDictEntry const *)arg2)->sig;
    if (sig1 > sig2) {
        return 1;
    }
    else if (sig1 < sig2) {
        return -1;
    }
    else { /* sig1 == sig2 */
        return 0;
    }
}
/*..........................................................................*/
static void SigDictionary_ctor(SigDictionary * const me,
                        SigDictEntry *sto, uint32_t capacity)
{
    me->sto      = sto;
    me->capacity = capacity;
    me->entries  = 0;
    me->ptrSize  = 4;
}
/*..........................................................................*/
static void SigDictionary_config(SigDictionary * const me, int ptrSize) {
    me->ptrSize = ptrSize;
}
/*..........................................................................*/
static void SigDictionary_put(SigDictionary * const me,
                              SigType sig, ObjType obj, char const *name)
{
    int idx = SigDictionary_find(me, sig, obj);
    int n = me->entries;
    char *dst;
    if (idx >= 0) { /* the key found? */
        Q_ASSERT((idx <= n) || (n == 0));
        me->sto[idx].obj = obj;
        dst = me->sto[idx].name;
        STRNCPY_S(dst, name, sizeof(me->sto[idx].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
    }
    else if (n < me->capacity - 1) {
        me->sto[n].sig = sig;
        me->sto[n].obj = obj;
        dst = me->sto[n].name;
        STRNCPY_S(dst, name, sizeof(me->sto[n].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0'; /* zero-terminate */
        ++me->entries;
        /* keep the entries sorted by the sig */
        qsort(me->sto, (size_t)me->entries, sizeof(me->sto[0]),
              &SigDictionary_comp);
    }
}
/*..........................................................................*/
static char const *SigDictionary_get(SigDictionary * const me,
                                     SigType sig, ObjType obj, char *buf)
{
    int idx;
    if (sig == 0) {
        return "NULL";
    }
    idx = SigDictionary_find(me, sig, obj);
    if (idx >= 0) {
        return me->sto[idx].name;
    }
    else { /* key not found */
        if (buf == 0) { /* extra buffer not provided? */
            buf = me->notFound.name; /* use the internal location */
        }
        /* otherwise use the provided buffer... */
        if (me->ptrSize <= 4) {
            SNPRINTF_S(buf, NAME_LEN, "%08X,Obj=%08"PRIX64"", (int)sig, obj);
        }
        else {
            SNPRINTF_S(buf, NAME_LEN, "%08X,Obj=%016"PRIX64"", (int)sig, obj);
        }
        //SigDictionary_put(me, sig, obj, buf); /* put into the dictionary */
        return buf;
    }
}
/*..........................................................................*/
static int SigDictionary_find(SigDictionary * const me,
                              SigType sig, ObjType obj)
{
    /* binary search algorithm ... */
    int mid;
    int first = 0;
    int last = me->entries;
    while (first <= last) {
        mid = (first + last) / 2;
        if (me->sto[mid].sig == sig) {
            if (obj == 0) { /* global/generic signal? */
                return mid;
            }
            else {
                first = mid;
                do {
                    if ((me->sto[first].obj == 0)
                        || (me->sto[first].obj == obj))
                    {
                        return first;
                    }
                    --first;
                } while ((first >= 0) && (me->sto[first].sig == sig));
                last = mid + 1;
                while ((last < me->entries) && (me->sto[last].sig == sig)) {
                    if ((me->sto[last].obj == 0)
                        || (me->sto[last].obj == obj))
                    {
                        return last;
                    }
                    ++last;
                }
                return -1;  /* entry not found */
            }
        }
        if (me->sto[mid].sig > sig) {
            last = mid - 1;
        }
        else {
            first = mid + 1;
        }
    }

    return -1; /* entry not found */
}
/*..........................................................................*/
static void SigDictionary_reset(SigDictionary * const me) {
    me->entries = 0;
}
/*..........................................................................*/
static void SigDictionary_write(SigDictionary const * const me,
                                FILE *stream)
{
    int i;

    fprintf(stream, "%d %d\n", me->entries, me->ptrSize);
    for (i = 0; i < me->entries; ++i) {
        SigDictEntry const *e = &me->sto[i];
        fprintf(stream, "%08X %016"PRIX64" %s\n", e->sig, e->obj, e->name);
    }
}
/*..........................................................................*/
static bool SigDictionary_read(SigDictionary * const me, FILE *stream) {
    char dictLine[80];
    int i;

    if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
        goto error;
    }
    if (SSCANF_S(dictLine, "%d %d\n", &me->entries, &me->ptrSize) != 2) {
        goto error;
    }
    if ((me->entries > me->capacity) || (me->ptrSize > 8)) {
        goto error;
    }
    for (i = 0; i < me->entries; ++i) {
        SigDictEntry *e = &me->sto[i];
        if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
            goto error;
        }
        if (SSCANF_S(dictLine, "%08X %016"PRIX64" %s\n",
            &e->sig, &e->obj, e->name) != 3)
        {
            goto error;
        }
    }
    return true;

error:
    SigDictionary_reset(me);
    return false;
}
