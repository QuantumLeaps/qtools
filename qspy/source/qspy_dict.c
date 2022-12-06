/*============================================================================
* QP/C Real-Time Embedded Framework (RTEF)
* Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
*
* SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
*
* This software is dual-licensed under the terms of the open source GNU
* General Public License version 3 (or any later version), or alternatively,
* under the terms of one of the closed source Quantum Leaps commercial
* licenses.
*
* The terms of the open source GNU General Public License version 3
* can be found at: <www.gnu.org/licenses/gpl-3.0>
*
* The terms of the closed source Quantum Leaps commercial licenses
* can be found at: <www.state-machine.com/licensing>
*
* Redistributions in source code must retain this top-level comment block.
* Plagiarizing this software to sidestep the license obligations is illegal.
*
* Contact information:
* <www.state-machine.com>
* <info@state-machine.com>
============================================================================*/
/*!
* @date Last updated on: 2021-12-23
* @version Last updated for version: 7.0.0
*
* @file
* @brief QSPY host uility: dictionary file reading/writing
*/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#include "safe_std.h" /* "safe" <stdio.h> and <string.h> facilities */
#include "qspy.h"     /* QSPY data parser */
#include "pal.h"      /* Platform Abstraction Layer */

/*..........................................................................*/
static char          l_dictFileName[QS_FNAME_LEN_MAX];

/*..........................................................................*/
bool QDIC_isActive(void) {
    return l_dictFileName[0] != '\0';
}

/*..........................................................................*/
void QSPY_setExternDict(char const *dictName) {
    SNPRINTF_S(l_dictFileName, sizeof(l_dictFileName), "%s", dictName);
}
/*..........................................................................*/
QSpyStatus QSPY_writeDict(void) {
    FILE *dictFile = (FILE *)0;
    char buf[QS_FNAME_LEN_MAX];

    /* no external dictionaries configured? */
    if (l_dictFileName[0] == '\0') {
        SNPRINTF_LINE("   <QSPY-> %s",
                      "Dictionaries NOT configured (no -d option)");
        QSPY_printError();
        return QSPY_ERROR;
    }

    /* no external dictionaries configured or no target config yet? */
    if (QSPY_conf.tstamp[5] == 0U) {
        SNPRINTF_LINE("   <QSPY-> %s",
                      "Dictionaries NOT saved (no target info)");
        QSPY_printError();
        return QSPY_ERROR;
    }

    /* synthesize dictionary name from the timestamp */
    SNPRINTF_S(buf, sizeof(buf),
           "qspy%02u%02u%02u_%02u%02u%02u.dic",
           (unsigned)QSPY_conf.tstamp[5],
           (unsigned)QSPY_conf.tstamp[4],
           (unsigned)QSPY_conf.tstamp[3],
           (unsigned)QSPY_conf.tstamp[2],
           (unsigned)QSPY_conf.tstamp[1],
           (unsigned)QSPY_conf.tstamp[0]);

    FOPEN_S(dictFile, buf, "w");
    if (dictFile == (FILE *)0) {
        SNPRINTF_LINE("   <QSPY-> Cannot save dictionaries to File=%s", buf);
        QSPY_printError();
        return QSPY_ERROR;
    }

    FPRINTF_S(dictFile, "-v%03d\n", (int)QSPY_conf.version);
    FPRINTF_S(dictFile, "-T%01d\n", (int)QSPY_conf.tstampSize);
    FPRINTF_S(dictFile, "-O%01d\n", (int)QSPY_conf.objPtrSize);
    FPRINTF_S(dictFile, "-F%01d\n", (int)QSPY_conf.funPtrSize);
    FPRINTF_S(dictFile, "-S%01d\n", (int)QSPY_conf.sigSize);
    FPRINTF_S(dictFile, "-E%01d\n", (int)QSPY_conf.evtSize);
    FPRINTF_S(dictFile, "-Q%01d\n", (int)QSPY_conf.queueCtrSize);
    FPRINTF_S(dictFile, "-P%01d\n", (int)QSPY_conf.poolCtrSize);
    FPRINTF_S(dictFile, "-B%01d\n", (int)QSPY_conf.poolBlkSize);
    FPRINTF_S(dictFile, "-C%01d\n", (int)QSPY_conf.tevtCtrSize);
    FPRINTF_S(dictFile, "-t%02d%02d%02d_%02d%02d%02d\n\n",
        (int)QSPY_conf.tstamp[5],
        (int)QSPY_conf.tstamp[4],
        (int)QSPY_conf.tstamp[3],
        (int)QSPY_conf.tstamp[2],
        (int)QSPY_conf.tstamp[1],
        (int)QSPY_conf.tstamp[0]);

    FPRINTF_S(dictFile, "%s\n", "Obj-Dic:");
    Dictionary_write(&QSPY_objDict, dictFile);

    FPRINTF_S(dictFile, "%s\n", "Fun-Dic:");
    Dictionary_write(&QSPY_funDict, dictFile);

    FPRINTF_S(dictFile, "%s\n", "Usr-Dic:");
    Dictionary_write(&QSPY_usrDict, dictFile);

    FPRINTF_S(dictFile, "%s\n", "Sig-Dic:");
    SigDictionary_write(&QSPY_sigDict, dictFile);

    fclose(dictFile);

    SNPRINTF_LINE("   <QSPY-> Dictionaries saved to File=%s", buf);
    QSPY_printInfo();

    return QSPY_SUCCESS;
}
/*..........................................................................*/
QSpyStatus QSPY_readDict(void) {
    FILE *dictFile;
    char name[QS_FNAME_LEN_MAX];
    char buf[256];
    uint32_t c = QSPY_conf.tstamp[5]; /* save the year-part of the tstamp */
    uint32_t d = 0U; /* assume no difference in the configuration */
    QSpyStatus stat = QSPY_SUCCESS; /* assume success */

    /* no external dictionaries configured? */
    if (l_dictFileName[0] == '\0') {
        SNPRINTF_LINE("   <QSPY-> %s",
                      "Dictionaries NOT configured (no -d option)");
        QSPY_printError();
        return QSPY_ERROR;
    }
    else if (l_dictFileName[0] == '?') { /* automatic dictionaries? */

        /* no target timestamp yet? */
        if (c == 0U) {
            SNPRINTF_LINE("   <QSPY-> %s",
                          "No Target info yet to read dictionaries");
            QSPY_printError();
            return QSPY_ERROR;
        }

        /* synthesize dictionary name from the timestamp */
        SNPRINTF_S(name, sizeof(name),
               "qspy%02u%02u%02u_%02u%02u%02u.dic",
               (unsigned)QSPY_conf.tstamp[5],
               (unsigned)QSPY_conf.tstamp[4],
               (unsigned)QSPY_conf.tstamp[3],
               (unsigned)QSPY_conf.tstamp[2],
               (unsigned)QSPY_conf.tstamp[1],
               (unsigned)QSPY_conf.tstamp[0]);
    }
    else { /* manual dictionaries */
        SNPRINTF_S(name, sizeof(name), "%s", l_dictFileName);
    }

    FOPEN_S(dictFile, name, "r");
    if (dictFile == (FILE *)0) {
        SNPRINTF_LINE("   <QSPY-> Dictionaries not found File=%s", name);
        QSPY_printError();
        return QSPY_ERROR;
    }

    /* output the status to the user */
    SNPRINTF_LINE("   <QSPY-> Reading dictionaries from File=%s", name);
    QSPY_onPrintLn();

    while (fgets(buf, sizeof(buf), (FILE *)dictFile) != (char *)0) {
        switch (buf[0]) {
            case '#':  /* comment beginning */
            case '\r': /* empty line (DOS) */
            case '\n': /* empty line (Unix) */
                /* skip the comment */
                break;
            case '-':
                switch (buf[1]) {
                    case 'v':
                        CONFIG_UPDATE(version,
                                      (100U*(buf[2] - '0')
                                           + 10U*(buf[3] - '0')
                                           + (buf[4] - '0')), d);
                        break;
                    case 'T':
                        CONFIG_UPDATE(tstampSize, (buf[2] - '0'), d);
                        break;
                    case 'O':
                        CONFIG_UPDATE(objPtrSize, (buf[2] - '0'), d);
                        break;
                    case 'F':
                        CONFIG_UPDATE(funPtrSize, (buf[2] - '0'), d);
                        break;
                    case 'S':
                        CONFIG_UPDATE(sigSize,    (buf[2] - '0'), d);
                        break;
                    case 'E':
                        CONFIG_UPDATE(evtSize,    (buf[2] - '0'), d);
                        break;
                    case 'Q':
                        CONFIG_UPDATE(queueCtrSize, (buf[2] - '0'), d);
                        break;
                    case 'P':
                        CONFIG_UPDATE(poolCtrSize, (buf[2] - '0'), d);
                        break;
                    case 'B':
                        CONFIG_UPDATE(poolBlkSize, (buf[2] - '0'), d);
                        break;
                    case 'C':
                        CONFIG_UPDATE(tevtCtrSize, (buf[2] - '0'), d);
                        break;
                    case 't':
                        CONFIG_UPDATE(tstamp[5],
                            ((buf[2 + 0] - '0')*10 + buf[2 + 1] - '0'), d);
                        CONFIG_UPDATE(tstamp[4],
                            ((buf[2 + 2] - '0')*10 + buf[2 + 3] - '0'), d);
                        CONFIG_UPDATE(tstamp[3],
                            ((buf[2 + 4] - '0')*10 + buf[2 + 5] - '0'), d);
                        CONFIG_UPDATE(tstamp[2],
                            ((buf[2 + 7] - '0')*10 + buf[2 + 8] - '0'), d);
                        CONFIG_UPDATE(tstamp[1],
                            ((buf[2 + 9] - '0')*10 + buf[2 +10] - '0'), d);
                        CONFIG_UPDATE(tstamp[0],
                            ((buf[2 +11] - '0')*10 + buf[2 +12] - '0'), d);
                        break;
                    default:
                        SNPRINTF_LINE("   <QSPY-> Unexpected option in "
                            "Dictionary File=%s,Opt=%c", name, buf[1]);
                        QSPY_printError();
                        stat = QSPY_ERROR;
                        goto error;
                }
                break;
            case 'O': /* object dictionary */
                if (!Dictionary_read(&QSPY_objDict, (FILE *)dictFile)) {
                    SNPRINTF_LINE("   <QSPY-> Parsing OBJ dictionaries failed"
                                  " File=%s", name);
                    QSPY_printError();
                    stat = QSPY_ERROR;
                    goto error;
                }
#ifdef QSPY_APP
                QSEQ_dictionaryReset();
                int i;
                for (i = 0; i < QSPY_objDict.entries; ++i) {
                    QSEQ_updateDictionary(QSPY_objDict.sto[i].name,
                                        QSPY_objDict.sto[i].key);
                }
#endif
                break;
            case 'F': /* function dictionary */
                if (!Dictionary_read(&QSPY_funDict, (FILE *)dictFile)) {
                    SNPRINTF_LINE("   <QSPY-> Parsing FUN dictionaries failed"
                                  " File=%s", name);
                    QSPY_printError();
                    stat = QSPY_ERROR;
                    goto error;
                }
                break;
            case 'U': /* user dictionary */
                if (!Dictionary_read(&QSPY_usrDict, (FILE *)dictFile)) {
                    SNPRINTF_LINE("   <QSPY-> Parsing USR dictionaries failed"
                                  " File=%s", name);
                    QSPY_printError();
                    stat = QSPY_ERROR;
                    goto error;
                }
                break;
            case 'S': /* signal dictionary */
                if (!SigDictionary_read(&QSPY_sigDict, (FILE *)dictFile)) {
                    SNPRINTF_LINE("   <QSPY-> Parsing SIG dictionaries failed"
                                  " File=%s", name);
                    QSPY_printError();
                    stat = QSPY_ERROR;
                    goto error;
                }
                break;
            /* don't read the sequence dictionary */
            default:
                SNPRINTF_LINE("   <QSPY-> Unexpected line in "
                              "Dictionary File=%s,Char=%c", name, buf[0]);
                QSPY_printError();
                stat = QSPY_ERROR;
                goto error;
        }
    }

error:
    fclose(dictFile);

    /* any differences in config and not the first time through? */
    if ((stat != QSPY_ERROR) && (d != 0U) && (c != 0U)) {
        SNPRINTF_LINE("   <QSPY-> %s",
                      "Dictionaries mismatch the Target (discarded)");
        QSPY_printInfo();

        QSPY_resetAllDictionaries();
        stat = QSPY_ERROR;
    }

    return stat;
}

/*..........................................................................*/
void Dictionary_write(Dictionary const * const me, FILE *stream) {
    int i;

    FPRINTF_S(stream, "%d\n", me->keySize);
    for (i = 0; i < me->entries; ++i) {
        DictEntry const *e = &me->sto[i];
        if (me->keySize <= 4) {
            FPRINTF_S(stream, "0x%08X %s\n", (unsigned)e->key, e->name);
        }
        else {
            FPRINTF_S(stream, "0x%016"PRIX64" %s\n", e->key, e->name);
        }
    }
    FPRINTF_S(stream, "%s\n", "***"); /* close marker for a dictionary */
}
/*..........................................................................*/
bool Dictionary_read(Dictionary * const me, FILE *stream) {
    char dictLine[80];

    if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
        goto error;
    }
    me->keySize = (int)strtol(dictLine, NULL, 10);
    if ((me->keySize == 0) || (me->keySize > 8)) {
        goto error;
    }

    Dictionary_reset(me);
    while (me->entries < me->capacity) {
        uint64_t key = 0;
        char *name = NULL;
        char *str_end = NULL;

        if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
            break;
        }
        if ((dictLine[0] != '0') || (dictLine[1] != 'x')) {
            break;
        }
        if (me->keySize <= 4) {
            dictLine[10] = '\0';
            name = &dictLine[11];
            key = (uint64_t)strtoul(dictLine, &str_end, 16);
        }
        else {
            dictLine[18] = '\0';
            key = strtoull(dictLine, &str_end, 16);
            name = &dictLine[19];
        }
        if (str_end == dictLine) {
            goto error;
        }
        /* remove the '\n' from the end of the name string */
        name[strlen(name) - 1] = '\0';
        Dictionary_put(me, key, name);
    }
    return true;

error:
    Dictionary_reset(me);
    return false;
}
/*..........................................................................*/
void SigDictionary_write(SigDictionary const* const me, FILE* stream) {
    int i;

    FPRINTF_S(stream, "%d\n", me->ptrSize);
    for (i = 0; i < me->entries; ++i) {
        SigDictEntry const* e = &me->sto[i];
        if (me->ptrSize <= 4) {
            FPRINTF_S(stream, "%08d 0x%08X %s\n",
                e->sig, (unsigned)e->obj, e->name);
        }
        else {
            FPRINTF_S(stream, "%08d 0x%016"PRIX64" %s\n",
                e->sig, e->obj, e->name);
        }
    }
    FPRINTF_S(stream, "%s\n", "***"); /* close marker for a dictionary */
}
/*..........................................................................*/
bool SigDictionary_read(SigDictionary * const me, FILE *stream) {
    char dictLine[80];

    if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
        goto error;
    }
    me->ptrSize = (int)strtol(dictLine, NULL, 10);
    if ((me->ptrSize == 0) || (me->ptrSize > 8)) {
        goto error;
    }

    SigDictionary_reset(me);
    while (me->entries < me->capacity) {
        uint32_t sig = 0;
        uint64_t obj = 0;
        char *name = NULL;
        char *str_end = NULL;

        if (fgets(dictLine, sizeof(dictLine), stream) == (char *)0) {
            break;
        }
        if (dictLine[0] != '0') {
            break;
        }

        /* parse the sig */
        dictLine[8] = '\0';
        sig = strtoul(dictLine, NULL, 10);
        if (sig == 0) {
            goto error;
        }
        /* parse the obj and name */
        if (me->ptrSize <= 4) {
            dictLine[19] = '\0';
            name = &dictLine[20];
            obj = (uint64_t)strtoul(&dictLine[9], &str_end, 16);
        }
        else {
            dictLine[27] = '\0';
            name = &dictLine[28];
            obj = strtoull(&dictLine[9], &str_end, 16);
        }
        if (str_end == &dictLine[9]) {
            goto error;
        }
        /* remove the '\n' from the end of the name string */
        name[strlen(name) - 1] = '\0';
        SigDictionary_put(me, sig, obj, name);
    }
    return true;

error:
    SigDictionary_reset(me);
    return false;
}
/*..........................................................................*/
char const* QSPY_getMatDict(char const* s) {
    static char dict[65];
    char* pc = dict;
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
    return (dict[0] == '&' ? dict + 1 : dict);
}
