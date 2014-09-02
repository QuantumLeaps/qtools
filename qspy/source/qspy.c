/*****************************************************************************
* Product: Quantum Spy -- Host resident component
* Last updated for version 5.3.1
* Last updated on  2014-04-21
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) Quantum Leaps, www.state-machine.com.
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
* Web:   www.state-machine.com
* Email: info@state-machine.com
*****************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <inttypes.h>

#include "qspy.h"

typedef char     char_t;
typedef float    float32_t;
typedef double   float64_t;
typedef int      enum_t;
typedef unsigned uint_t;

#ifndef Q_SPY
#define Q_SPY 1
#endif
#include "qs_copy.h"            /* copy of the target-resident QS interface */

/* global objects ..........................................................*/
char QSPY_line[512];                /* last formatted line ready for output */

/****************************************************************************/
                                 /* name string length for internal buffers */
#define NAME_LEN 64

/*..........................................................................*/
typedef uint64_t KeyType;
typedef uint32_t SigType;
typedef uint64_t ObjType;

typedef struct DictEntryTag {
    KeyType key;
    char    name[NAME_LEN];
} DictEntry;

typedef struct DictionaryTag {
    DictEntry  notFound;
    DictEntry *sto;
    int        capacity;
    int        entries;
    int        keySize;
} Dictionary;

static void Dictionary_ctor(Dictionary * const me,
                            DictEntry *sto, uint32_t capacity);
static void Dictionary_config(Dictionary * const me, int keySize);
static char const *Dictionary_entry(Dictionary * const me, unsigned idx);
static void Dictionary_put(Dictionary * const me,
                           KeyType key, char const *name);
static char const *Dictionary_get(Dictionary * const me,
                                  KeyType key, char *buf);
static int Dictionary_find(Dictionary * const me, KeyType key);
static void Dictionary_reset(Dictionary * const me);

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

/*..........................................................................*/
static DictEntry     l_funSto[512];
static DictEntry     l_objSto[256];
static DictEntry     l_mscSto[64];
static DictEntry     l_usrSto[256 + 1 - QS_USER];
static SigDictEntry  l_sigSto[512];
static Dictionary    l_funDict;
static Dictionary    l_objDict;
static Dictionary    l_mscDict;
static Dictionary    l_usrDict;
static SigDictionary l_sigDict;

/*..........................................................................*/
static uint16_t      l_version;
static uint8_t       l_objPtrSize;
static uint8_t       l_funPtrSize;
static uint8_t       l_tstampSize;
static uint8_t       l_sigSize;
static uint8_t       l_evtSize;
static uint8_t       l_queueCtrSize;
static uint8_t       l_poolCtrSize;
static uint8_t       l_poolBlkSize;
static uint8_t       l_tevtCtrSize;
static FILE         *l_savFile;
static FILE         *l_matFile;
static FILE         *l_mscFile;
static QSPY_CustParseFun l_custParseFun;
static int           l_linePos;

#define SNPRINF_LINE(...) \
    snprintf(QSPY_line, sizeof(QSPY_line) - 1, ##__VA_ARGS__)

#define SNPRINF_AT(...) do { \
    int n = snprintf(&QSPY_line[l_linePos], \
                     sizeof(QSPY_line) - 1U - l_linePos, \
                     ##__VA_ARGS__); \
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
    l_version      = version;
    l_objPtrSize   = objPtrSize;
    l_funPtrSize   = funPtrSize;
    l_tstampSize   = tstampSize;
    l_sigSize      = sigSize;
    l_evtSize      = evtSize;
    l_queueCtrSize = queueCtrSize;
    l_poolCtrSize  = poolCtrSize;
    l_poolBlkSize  = poolBlkSize;
    l_tevtCtrSize  = tevtCtrSize;
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
    Dictionary_config(&l_funDict, l_funPtrSize);
    Dictionary_config(&l_objDict, l_objPtrSize);
    Dictionary_config(&l_usrDict, 1);
    SigDictionary_config(&l_sigDict, l_objPtrSize);

    SNPRINF_LINE("-T %d", (unsigned)tstampSize);   QSPY_onPrintLn();
    SNPRINF_LINE("-O %d", (unsigned)objPtrSize);   QSPY_onPrintLn();
    SNPRINF_LINE("-F %d", (unsigned)funPtrSize);   QSPY_onPrintLn();
    SNPRINF_LINE("-S %d", (unsigned)sigSize);      QSPY_onPrintLn();
    SNPRINF_LINE("-E %d", (unsigned)evtSize);      QSPY_onPrintLn();
    SNPRINF_LINE("-Q %d", (unsigned)queueCtrSize); QSPY_onPrintLn();
    SNPRINF_LINE("-P %d", (unsigned)poolCtrSize);  QSPY_onPrintLn();
    SNPRINF_LINE("-B %d", (unsigned)poolBlkSize);  QSPY_onPrintLn();
    SNPRINF_LINE("-C %d", (unsigned)tevtCtrSize);  QSPY_onPrintLn();
    QSPY_line[0] = '\0'; QSPY_onPrintLn();
}
/*..........................................................................*/
void QSpyRecord_ctor(QSpyRecord * const me,
                            uint8_t rec, uint8_t const *pos, int32_t len)
{
    me->rec = rec;
    me->pos = pos;
    me->len = len;
}
/*..........................................................................*/
int QSpyRecord_OK(QSpyRecord * const me) {
    if (me->len != (uint8_t)0) {
        SNPRINF_LINE("********** %03d: Error %d bytes unparsed",
               (int)me->rec, (int)me->len);
        QSPY_onPrintLn();
        return 0;
    }
    return 1;
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
            assert(0);
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
            ret >>= 24;                                      /* sign-extend */
        }
        else if (size == (uint8_t)2) {
            ret = ((uint32_t)me->pos[1] << 8)
                        | (uint32_t)me->pos[0];
            ret <<= 16;
            ret >>= 16;                                      /* sign-extend */
        }
        else if (size == (uint8_t)4) {
            ret = ((((((int32_t)me->pos[3] << 8)
                        | (uint32_t)me->pos[2]) << 8)
                          | (uint32_t)me->pos[1]) << 8)
                            | (uint32_t)me->pos[0];
        }
        else {
            assert(0);
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
            assert(0);
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
            ret >>= 56;                                      /* sign-extend */
        }
        else if (size == (uint8_t)2) {
            ret = (((uint64_t)me->pos[1] << 8)
                       | (uint32_t)me->pos[0]);
            ret <<= 48;
            ret >>= 48;                                      /* sign-extend */
        }
        else if (size == (uint8_t)4) {
            ret = ((((((uint64_t)me->pos[3] << 8)
                        | (uint64_t)me->pos[2]) << 8)
                          | (uint64_t)me->pos[1]) << 8)
                            | (uint64_t)me->pos[0];
            ret <<= 32;
            ret >>= 32;                                      /* sign-extend */
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
            assert(0);
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
uint8_t const *QSpyRecord_getMem(QSpyRecord * const me, uint8_t *pl) {

    if ((me->len >= 1) && ((*me->pos) <= me->len - 1)) {
        uint8_t const *mem = me->pos + 1;
        *pl = *me->pos;
        me->len -= 1 + *me->pos;
        me->pos += 1 + *me->pos;
        return mem;
    }

    me->len = -1;
    *pl = (uint8_t)0;
    SNPRINF_LINE("Mem overrun");
    QSPY_onPrintLn();

    return (uint8_t *)0;
}
/*..........................................................................*/
static void QSpyRecord_parseUser(QSpyRecord * const me) {
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

    u32 = QSpyRecord_getUint32(me, l_tstampSize);
    l_linePos = 0;
    i32 = Dictionary_find(&l_usrDict, (me->rec + 1 - QS_USER));
    if (i32 >= 0) {
        SNPRINF_AT("%010u %s: ", u32, Dictionary_entry(&l_usrDict, i32));
    }
    else {
        SNPRINF_AT("%010u User%03d: ", u32, (int)(me->rec - QS_USER));
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
                QSPY_onPrintLn();

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
                uint8_t l;
                uint8_t const *mem = QSpyRecord_getMem(me, &l);
                while (l-- != (uint8_t)0) {
                    SNPRINF_AT("%02X ", (int)*mem);
                    if (l_matFile != (FILE *)0) {
                        fprintf(l_matFile, "%03d ", (int)*mem);
                    }
                    ++mem;
                }
                break;
            }
            case QS_SIG_T: {
                u32 = QSpyRecord_getUint32(me, l_sigSize);
                u64 = QSpyRecord_getUint64(me, l_objPtrSize);
                SNPRINF_AT("%s,Obj=%s ",
                    SigDictionary_get(&l_sigDict, u32, u64, (char *)0),
                    Dictionary_get(&l_objDict, u64, (char *)0));

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%04u %20"PRId64" ", u32, u64);
                }
                break;
            }
            case QS_OBJ_T: {
                u64 = QSpyRecord_getUint64(me, l_objPtrSize);
                SNPRINF_AT("%s ",
                    Dictionary_get(&l_objDict, u64, (char *)0));

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%20"PRId64" ", u64);
                }
                break;
            }
            case QS_FUN_T: {
                u64 = QSpyRecord_getUint64(me, l_funPtrSize);
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
static void QSpyRecord_parse(QSpyRecord * const me) {
    uint32_t t, a, b, c, d, e;
    uint64_t p, q, r;
    char buf[NAME_LEN];
    char const *s = 0;
    char const *w = 0;

    switch (me->rec) {
        /* Reset record ....................................................*/
        case QS_QP_RESET: {
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("********** RESET");
                QSPY_onPrintLn();
                Dictionary_reset(&l_funDict);
                Dictionary_reset(&l_objDict);
                Dictionary_reset(&l_mscDict);
                Dictionary_reset(&l_usrDict);
                SigDictionary_reset(&l_sigDict);
                /*TBD: close and re-open the MATLAB file, MSC file, etc. */
            }
            break;
        }

        /* QEP records .....................................................*/
        case QS_QEP_STATE_ENTRY:
            s = "ENTRY";
        case QS_QEP_STATE_EXIT: {
            if (s == 0) s = "EXIT ";
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_funPtrSize);
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
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_funPtrSize);
            r = QSpyRecord_getUint64(me, l_funPtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_funPtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_funPtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u Intern : "
                       "Obj=%s Sig=%s Source=%s",
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
        case QS_QEP_TRAN: {
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_funPtrSize);
            r = QSpyRecord_getUint64(me, l_funPtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_funPtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_funPtrSize);
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
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_funPtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            if (l_version < 420U) {
                q = (uint64_t)0;
            }
            else {
                q = QSpyRecord_getUint64(me, l_objPtrSize);
            }
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            if (l_version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_queueCtrSize);
            e = QSpyRecord_getUint32(me, l_queueCtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            if (l_version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_queueCtrSize);
            e = QSpyRecord_getUint32(me, l_queueCtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            if (l_version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_queueCtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            if (l_version < 420U) {
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
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            b = QSpyRecord_getUint32(me, l_queueCtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            if (l_version < 420U) {
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                b = QSpyRecord_getUint32(me, 1);
                c = QSpyRecord_getUint32(me, 1);
            }
            d = QSpyRecord_getUint32(me, l_queueCtrSize);
            e = QSpyRecord_getUint32(me, l_queueCtrSize);
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
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            b = QSpyRecord_getUint32(me, l_poolCtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            b = QSpyRecord_getUint32(me, l_poolCtrSize);
            c = QSpyRecord_getUint32(me, l_poolCtrSize);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u MP.%s : Obj=%s "
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            b = QSpyRecord_getUint32(me, l_poolCtrSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            if (l_version < 420U) {
                p = (uint64_t)0;
                a = QSpyRecord_getUint32(me, l_sigSize);
                b = QSpyRecord_getUint32(me, 1);
                c = b & 0x3F;
                b >>= 6;
            }
            else {
                p = QSpyRecord_getUint64(me, l_objPtrSize);
                a = QSpyRecord_getUint32(me, l_sigSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_evtSize);
            c = QSpyRecord_getUint32(me, l_sigSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            if (l_version < 420U) {
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
            a = QSpyRecord_getUint32(me, l_tevtCtrSize);
            if (l_version < 500U) {
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_objPtrSize);
            c = QSpyRecord_getUint32(me, l_tevtCtrSize);
            d = QSpyRecord_getUint32(me, l_tevtCtrSize);
            if (l_version < 500U) {
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
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_objPtrSize);
            if (l_version < 500U) {
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_objPtrSize);
            if (l_version < 500U) {
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_objPtrSize);
            c = QSpyRecord_getUint32(me, l_tevtCtrSize);
            d = QSpyRecord_getUint32(me, l_tevtCtrSize);
            e = QSpyRecord_getUint32(me, 1);
            if (l_version < 500U) {
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            a = QSpyRecord_getUint32(me, l_sigSize);
            q = QSpyRecord_getUint64(me, l_objPtrSize);
            if (l_version < 500U) {
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            q = QSpyRecord_getUint64(me, l_objPtrSize);
            c = QSpyRecord_getUint32(me, l_tevtCtrSize);
            d = QSpyRecord_getUint32(me, l_tevtCtrSize);
            if (l_version < 500U) {
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
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
            t = QSpyRecord_getUint32(me, l_tstampSize);
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

        /* QK records ......................................................*/
        case QS_QK_MUTEX_LOCK:
            if (s == 0) s = "QK_muxL";
        case QS_QK_MUTEX_UNLOCK: {
            if (s == 0) s = "QK_muxU";
            t = QSpyRecord_getUint32(me, l_tstampSize);
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
        case QS_QK_SCHEDULE: {
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, 1);
            b = QSpyRecord_getUint32(me, 1);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u QK_sche: "
                       "prio=%2u, pin=%3u",
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
            a = QSpyRecord_getUint32(me, l_sigSize);
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                SigDictionary_put(&l_sigDict, (SigType)a, p, s);
                SNPRINF_LINE("           Sig Dic: %08X,Obj=%016"PRIX64" ->%s",
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
            p = QSpyRecord_getUint64(me, l_objPtrSize);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&l_objDict, p, ((s[0] == '&') ? s + 1 : s));
                SNPRINF_LINE("           Obj Dic: %016"PRIX64"->%s",
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
            p = QSpyRecord_getUint64(me, l_funPtrSize);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&l_funDict, p, ((s[0] == '&') ? s + 1 : s));
                SNPRINF_LINE("           Fun Dic: %016"PRIX64"->%s",
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
            if (QSpyRecord_OK(me)) {
                Dictionary_put(&l_usrDict, (a + 1 - QS_USER), s);
                SNPRINF_LINE("           Usr Dic: %08X        ->%s",
                        a, s);
                QSPY_onPrintLn();
            }
            break;
        }

        case QS_EMPTY: {
            /* silently ignore */
            break;
        }
        case QS_TEST_RUN: {
            t = QSpyRecord_getUint32(me, l_tstampSize);
            s = QSpyRecord_getStr(me);
            w = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u !TEST: File=%s Test=%s",
                       t, s, w);
                QSPY_onPrintLn();
            }
            break;
        }
        case QS_TEST_FAIL: {
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, 2);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u !FAIL: File=%s Line=%4hu",
                       t, s, a);
                QSPY_onPrintLn();

                if (l_matFile != (FILE *)0) {
                    fprintf(l_matFile, "%3d %10u %4u %s\n",
                            (int)me->rec, (unsigned)t, (unsigned)a, s);
                }
            }
            break;
        }
        case QS_ASSERT_FAIL: {
            t = QSpyRecord_getUint32(me, l_tstampSize);
            a = QSpyRecord_getUint32(me, 2);
            s = QSpyRecord_getStr(me);
            if (QSpyRecord_OK(me)) {
                SNPRINF_LINE("%010u !ASSERT: File=%s Line=%4hu",
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
                QSpyRecord_parseUser(me);
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
    int i;
    if (l_matFile != (FILE *)0) {
        fclose(l_matFile);
    }
    if (l_mscFile != (FILE *)0) {
        fprintf(l_mscFile, "}\n");
        rewind(l_mscFile);
        fprintf(l_mscFile, "msc {\n");
        for (i = 0; ; ++i) {
            char const *entry = Dictionary_entry(&l_mscDict, i);
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
}

/*..........................................................................*/
#define QS_FRAME            ((uint8_t)0x7EU)
#define QS_ESC              ((uint8_t)0x7DU)
#define QS_GOOD_CHKSUM      ((uint8_t)0xFFU)
#define QS_ESC_XOR          0x20U

#define QS_MAX_RECORD_SIZE  256

void QSPY_parse(uint8_t const *buf, uint32_t nBytes) {
    static uint8_t esc;
    static uint8_t seq;
    static uint8_t chksum;
    static uint8_t record[QS_MAX_RECORD_SIZE];
    static uint8_t *pos = record;             /* position within the record */

    if (l_savFile != (FILE *)0) {
        fwrite(buf, 1, nBytes, l_savFile);
    }

    while (nBytes-- != 0) {
        uint8_t b = *buf++;

        if (esc) {
            esc = (uint8_t)0;
            b ^= QS_ESC_XOR;
        }
        else {
            if (b == QS_FRAME) {
                if (chksum != QS_GOOD_CHKSUM) {
                    SNPRINF_LINE("*** Bad checksum at seq=%3u, id=%3u",
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
                    SNPRINF_LINE("*** Record too short at seq=%3u, id=%3u",
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
                else {
                    QSpyRecord qrec;
                    int parse = 1;

                    if ((uint8_t)(seq + 1) != record[0]) {
                        SNPRINF_LINE("*** Dropped %3u records",
                               (unsigned)((record[0] - seq - 1) & 0xFF));
                        QSPY_onPrintLn();
                        if (l_mscFile != (FILE *)0) {
                            fprintf(l_mscFile,
                                "--- [label=\"Dropped %3u records\""
                                ",textbgcolour=\"#ffff00\""
                                ",linecolour=\"#ff0000\"];\n"
                                "...;\n",
                                (unsigned)((record[0] - seq - 1) & 0xFF));
                        }
                    }
                    seq = record[0];

                    QSpyRecord_ctor(&qrec,
                                    record[1],
                                    &record[2],
                                    (uint32_t)(pos - record - 3));

                    if (l_custParseFun != (QSPY_CustParseFun)0) {
                        parse = (*l_custParseFun)(&qrec);
                        if (parse) {
                            /* re-initialize the record for parsing again */
                            QSpyRecord_ctor(&qrec,
                                            record[1],
                                            &record[2],
                                            (uint32_t)(pos - record - 3));
                        }
                    }
                    if (parse) {
                        QSpyRecord_parse(&qrec);
                    }
                }
                                       /* get ready for the next record ... */
                chksum = (uint8_t)0;
                pos = record;
                esc = (uint8_t)0;
                record[0] = (uint8_t)0;
                record[1] = (uint8_t)0;
                record[2] = (uint8_t)0;
                record[3] = (uint8_t)0;
                continue;
            }
            else if (b == QS_ESC) {
                esc = (uint8_t)1;
                continue;
            }
        }

        chksum = (uint8_t)(chksum + b);
        if (pos < &record[sizeof(record)]) {
            *pos++ = b;
        }
        else {
            SNPRINF_LINE("Error, record too long");
            QSPY_onPrintLn();
            chksum = (uint8_t)0;
            pos = record;
            esc = (uint8_t)0;
            record[0] = (uint8_t)0;
            record[1] = (uint8_t)0;
            record[2] = (uint8_t)0;
            record[3] = (uint8_t)0;
        }
    }
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
static char const *Dictionary_entry(Dictionary * const me, unsigned idx) {
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
    if (idx >= 0) {                                       /* the key found? */
        assert((idx <= n) || (n == 0));
        dst = me->sto[idx].name;
        strncpy(dst, name, sizeof(me->sto[idx].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0';        /* zero-terminate */
    }
    else if (n < me->capacity - 1) {
        me->sto[n].key = key;
        dst = me->sto[n].name;
        strncpy(dst, name, sizeof(me->sto[n].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0';        /* zero-terminate */
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
    if (idx >= 0) {
        return me->sto[idx].name;
    }
    else {                                                 /* key not found */
        if (buf == 0) {                       /* extra buffer not provided? */
            buf = me->notFound.name;           /* use the internal location */
        }
        /* otherwise use the provided buffer... */
        if (me->keySize <= 4) {
            snprintf(buf, NAME_LEN, "%08"PRIX64"", key);
        }
        else {
            snprintf(buf, NAME_LEN, "%016"PRIX64"", key);
        }
        //Dictionary_put(me, key, buf);            /* put into the dictionary */
        return buf;
    }
}
/*..........................................................................*/
static int Dictionary_find(Dictionary * const me, KeyType key) {
                                             /* binary search algorithm ... */
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

    return -1;                                           /* entry not found */
}
/*..........................................................................*/
void Dictionary_reset(Dictionary * const me) {
    me->entries = 0;
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
    else {                                                  /* sig1 == sig2 */
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
    if (idx >= 0) {                                       /* the key found? */
        assert((idx <= n) || (n == 0));
        me->sto[idx].obj = obj;
        dst = me->sto[idx].name;
        strncpy(dst, name, sizeof(me->sto[idx].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0';        /* zero-terminate */
    }
    else if (n < me->capacity - 1) {
        me->sto[n].sig = sig;
        me->sto[n].obj = obj;
        dst = me->sto[n].name;
        strncpy(dst, name, sizeof(me->sto[n].name) - 1);
        dst[sizeof(me->sto[idx].name) - 1] = '\0';        /* zero-terminate */
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
    else {                                                 /* key not found */
        if (buf == 0) {                       /* extra buffer not provided? */
            buf = me->notFound.name;           /* use the internal location */
        }
        /* otherwise use the provided buffer... */
        if (me->ptrSize <= 4) {
            snprintf(buf, NAME_LEN, "%08X,Obj=%08"PRIX64"", (int)sig, obj);
        }
        else {
            snprintf(buf, NAME_LEN, "%08X,Obj=%016"PRIX64"", (int)sig, obj);
        }
        //SigDictionary_put(me, sig, obj, buf);    /* put into the dictionary */
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
            if (obj == 0) {                       /* global/generic signal? */
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
                return -1;                               /* entry not found */
            }
        }
        if (me->sto[mid].sig > sig) {
            last = mid - 1;
        }
        else {
            first = mid + 1;
        }
    }

    return -1;                                           /* entry not found */
}
/*..........................................................................*/
void SigDictionary_reset(SigDictionary * const me) {
    me->entries = 0;
}
