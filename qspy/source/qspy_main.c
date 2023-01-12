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
* @date Last updated on: 2023-01-11
* @version Last updated for version: 7.2.0
*
* @file
* @brief main for QSPY host utility
*/
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "safe_std.h"   /* "safe" <stdio.h> and <string.h> facilities */
#include "qspy.h"       /* QSPY data parser */
#include "be.h"         /* Back-End interface */
#include "pal.h"        /* Platform Abstraction Layer */
#include "getopt.h"     /* command-line option processor */

#define Q_SPY   1       /* this is QP implementation */
#define QP_IMPL 1       /* this is QP implementation */
#include "qpc_qs.h"     /* QS target-resident interface */
#include "qpc_qs_pkg.h" /* QS package-scope interface */

/*..........................................................................*/
typedef enum {
    NO_LINK,
    FILE_LINK,
    SERIAL_LINK,
    TCP_LINK
} TargetLink;

static TargetLink l_link = NO_LINK;
static int   l_quiet     = -1;
static int   l_quiet_ctr = 0;
static bool  l_kbd_inp   = true; /* keyboard input supported */
static FILE *l_outFile = (FILE *)0;
static FILE *l_savFile = (FILE *)0;
static FILE *l_matFile = (FILE *)0;
static FILE *l_seqFile = (FILE *)0;

static char  l_comPort    [QS_FNAME_LEN_MAX];
static char  l_inpFileName[QS_FNAME_LEN_MAX];
static char  l_outFileName[QS_FNAME_LEN_MAX];
static char  l_savFileName[QS_FNAME_LEN_MAX];
static char  l_matFileName[QS_FNAME_LEN_MAX];
static char  l_seqFileName[QS_FNAME_LEN_MAX];
static char  l_dicFileName[QS_FNAME_LEN_MAX];

static char  l_tstampStr  [16];
static char  l_seqList[QS_SEQ_LIST_LEN_MAX];

static int   l_bePort   = 7701;   /* default UDP port  */
static int   l_tcpPort  = 6601;   /* default TCP port */
static int   l_baudRate = 115200; /* default serial baud rate */

/* color rendering */
extern char const * const l_darkPalette[];
extern char const * const l_lightPalette[];
static char const * const *l_colorPalette = l_darkPalette;

static char const l_introStr[] = \
    "QSPY %s Copyright (c) 2005-2023 Quantum Leaps\n" \
    "Documentation: https://www.state-machine.com/qtools/qspy.html\n" \
    "Current timestamp: %s\n";

static char const l_helpStr[] =
    "Usage: qspy [options]     <arg> = required, [arg] = optional\n"
    "\n"
    "OPTION            DEFAULT COMMENT\n"
    "                  (key)\n"
    "---------------------------------------------------------------\n"
    "-h                        help (show this message)\n"
    "-q [num]          (key-q) quiet mode (no QS data output)\n"
    "-u [UDP_port|0]   7701    UDP socket with optional port, 0-no UDP\n"
    "-v <QS_version>   7.0     compatibility with QS version\n"
    "-r <c0|c1|c2>     c1      rendering (c0=no-color, c1-color1, )\n"
    "-k                        suppress keyboard input\n"
    "-o                (key-o) save screen output to a file\n"
    "-s                (key-s) save binary QS data to a file\n"
    "-m                        produce Matlab output to a file\n"
    "-g <obj_list>             produce Sequence diagram to a file\n"
    "-t [TCP_port]     6601    TCP/IP input with optional port\n"
#ifdef _WIN32
    "-c <COM_port>     COM1    com port input (default)\n"
#elif (defined __linux) || (defined __linux__) || (defined __posix)
    "-c <serial_port>  /dev/ttyS0 serial port input (default)\n"
#endif
    "-b <baud_rate>    115200  baud rate for the com port\n"
    "-f <file_name>            file input (postprocessing)\n"
    "-d [file_name]            dictionary files\n"
    "-T <tstamp_size>  4       QS timestamp size     (bytes)\n"
    "-O <pointer_size> 4       object pointer size   (bytes)\n"
    "-F <pointer_size> 4       function pointer size (bytes)\n"
    "-S <signal_size>  2       event signal size     (bytes)\n"
    "-E <event_size>   2       event size size       (bytes)\n"
    "-Q <counter_size> 1       queue counter size    (bytes)\n"
    "-P <counter_size> 2       pool counter size     (bytes)\n"
    "-B <block_size>   2       pool block-size size  (bytes)\n"
    "-C <counter_size> 4       QTimeEvt counter size (bytes)\n";

static char const l_kbdHelpStr[] =
    "Keyboard shortcuts (valid when -k option is absent):\n"
    "KEY(s)            ACTION\n"
    "----------------------------------------------------------------\n"
    "<Esc>/x/X         Exit QSPY\n"
    "  h               display keyboard help and QSPY status\n"
    "  c               clear the screen\n"
    "  q               toggle quiet mode (no Target data from QS)\n"
    "  r               send RESET  command to the target\n"
    "  i               send INFO request to the target\n"
    "  t               send TICK[0] command to the target\n"
    "  u               send TICK[1] command to the target\n"
    "  d               trigger saving dictionaries to a file\n"
    "  o               toggle screen file output (close/re-open)\n"
    "  s/b             toggle binary file output (close/re-open)\n"
    "  m               toggle Matlab file output (close/re-open)\n"
    "  g               toggle Message sequence output (close/re-open)\n";

/*..........................................................................*/
static QSpyStatus configure(int argc, char *argv[]);
static void cleanup(void);
static void colorPrintLn(void);
static uint8_t l_buf[8*1024]; /* process input in 8K chunks */

/*..........................................................................*/
int main(int argc, char *argv[]) {
    int status = 0;

    /* parse the command-line options and configure QSPY ...................*/
    if (configure(argc, argv) != QSPY_SUCCESS) {
        status = -1;
    }
    else {
        uint32_t nBytes;
        bool isRunning = true;

        status = 0; /* assume success */
        while (isRunning) {   /* QSPY event loop... */

            /* get the event from the PAL... */
            nBytes = sizeof(l_buf);
            QSPYEvtType evt = (*PAL_vtbl.getEvt)(l_buf, &nBytes);

            switch (evt) {
                case QSPY_NO_EVT: /* all intputs timed-out this time around */
                    break;

                case QSPY_TARGET_INPUT_EVT: /* the Target sent some data... */
                    if (nBytes > 0) {
                        QSPY_parse(l_buf, (uint32_t)nBytes);
                        if (l_savFile != (FILE *)0) {
                            fwrite(l_buf, 1, nBytes, l_savFile);
                        }
                    }
                    break;

                case QSPY_FE_INPUT_EVT: /* the Front-End sent some data... */
                    Q_ASSERT(l_bePort != 0);
                    if (nBytes > 0) {
                        BE_parse(l_buf, nBytes);
                    }
                    break;

                case QSPY_KEYBOARD_EVT: /* the User pressed a key... */
                    isRunning = QSPY_command(l_buf[0], CMD_OPT_TOGGLE);
                    break;

                case QSPY_DONE_EVT:     /* done (e.g., file processed) */
                    isRunning = false;  /* terminate the event loop */
                    break;

                case QSPY_ERROR_EVT:    /* unrecoverable error */
                    isRunning = false;  /* terminate the event loop */
                    status = -1;        /* error return */
                    break;
            }
        }
    }

    /* cleanup .............................................................*/
    cleanup();
    return status;
}

/*..........................................................................*/
static void cleanup(void) {
    QSPY_cleanup();

    PAL_closeKbd();  /* close the keyboard input (if open) */

    if (l_savFile != (FILE *)0) {
        fclose(l_savFile);
    }
    if (l_outFile != (FILE *)0) {
        fclose(l_outFile);
    }

    QSEQ_configFile((void*)0);

    if (l_bePort != 0) {
        PAL_closeBE();  /* close the Back-End connection */
    }

    if (PAL_vtbl.cleanup != 0) {
        (*PAL_vtbl.cleanup)();  /* close the target connection */
    }

    PRINTF_S("\n%s\n", "QSPY Done");
}

/*..........................................................................*/
void Q_onAssert(char const * const module, int loc) {
    PRINTF_S("\n   <ERROR> QSPY ASSERTION failed in Module=%s:%d\n",
           module, loc);
    cleanup();
    exit(-1);
}

/*..........................................................................*/
void QSPY_onPrintLn(void) {
    if (l_outFile != (FILE *)0) {
        /* output file receives all trace records, regardles of -q mode */
        fputs(&QSPY_output.buf[QS_LINE_OFFSET], l_outFile);
        fputc('\n', l_outFile);
    }

    if (QSPY_output.type < BE_OUT) { /* message to be forwarded to BE? */
        BE_sendLine(); /* forward to the back-end */
    }

    if (l_quiet < 0) {
        if (l_colorPalette) {
            colorPrintLn();
        }
        else {
            fputs(&QSPY_output.buf[QS_LINE_OFFSET], stdout);
            fputc('\n', stdout);
        }
    }
    else if (l_quiet > 0) {
        if ((l_quiet_ctr == 0U) || (QSPY_output.type != REG_OUT)) {
            if ((l_quiet < 99) || (QSPY_output.type != REG_OUT)) {
                if (l_quiet_ctr != l_quiet - 1) {
                    fputc('\n', stdout);
                }
                if (l_colorPalette) {
                    colorPrintLn();
                }
                else {
                    fputs(&QSPY_output.buf[QS_LINE_OFFSET], stdout);
                    fputc('\n', stdout);
                }
                l_quiet_ctr = l_quiet;
            }
        }
        else {
            fputc('.', stdout);
        }
        --l_quiet_ctr;
    }

    QSPY_output.type = REG_OUT; /* reset for the next time */
}

/*..........................................................................*/
static QSpyStatus configure(int argc, char *argv[]) {
    static char const getoptStr[] =
        "hq::u::v:r:kosmg:c:b:t::p:f:d::T:O:F:S:E:Q:P:B:C:";

    /* default configuration options... */
    QSpyConfig config = {
        .version      = 700U,
        .endianness   = 0U,
        .objPtrSize   = 4U,
        .funPtrSize   = 4U,
        .tstampSize   = 4U,
        .sigSize      = 2U,
        .evtSize      = 2U,
        .queueCtrSize = 1U,
        .poolCtrSize  = 2U,
        .poolBlkSize  = 2U,
        .tevtCtrSize  = 4U,
    };
    int  optChar;

    STRNCPY_S(l_outFileName, sizeof(l_outFileName), "OFF");
    STRNCPY_S(l_savFileName, sizeof(l_savFileName), "OFF");
    STRNCPY_S(l_matFileName, sizeof(l_matFileName), "OFF");
    STRNCPY_S(l_seqFileName, sizeof(l_seqFileName), "OFF");
    STRNCPY_S(l_dicFileName, sizeof(l_dicFileName), "OFF");

    STRNCPY_S(l_tstampStr, sizeof(l_tstampStr), QSPY_tstampStr());
    PRINTF_S(l_introStr, QSPY_VER, l_tstampStr);

    STRNCPY_S(l_inpFileName, sizeof(l_inpFileName), "qs.bin");
#ifdef _WIN32
    STRNCPY_S(l_comPort, sizeof(l_comPort), "COM1");
#elif (defined __linux) || (defined __linux__) || (defined __posix)
    STRNCPY_S(l_comPort, sizeof(l_comPort), "/dev/ttyS0");
#endif
    l_seqList[0] = '\0';

    /* parse the command-line parameters ...................................*/
    while ((optChar = getopt(argc, argv, getoptStr)) != -1) {
        switch (optChar) {
            case 'q': { /* quiet mode */
                if (optarg != NULL) { /* is optional argument provided? */
                    l_quiet = (int)strtoul(optarg, NULL, 10);
                }
                else { /* apply the default */
                    l_quiet = 0;
                }
                break;
            }
            case 'u': { /* UDP control port */
                if (optarg != NULL) { /* is optional argument provided? */
                    l_bePort = (int)strtoul(optarg, NULL, 10);
                }
                else { /* apply the default */
                    l_bePort = 7701;
                }
                break;
            }
            case 'v': { /* compatibility with QS version */
                if (('0' <= optarg[0] && optarg[0] <= '9')
                    && (optarg[1] == '.')
                    && ('0' <= optarg[2] && optarg[2] <= '9'))
                {
                    config.version = (((optarg[0] - '0') * 10)
                                     + (optarg[2] - '0')) * 10;
                    PRINTF_S("-v %c.%c\n", optarg[0], optarg[2]);
                }
                else {
                    FPRINTF_S(stderr, "Incorrect version number: %s", optarg);
                    return QSPY_ERROR;
                }
                break;
            }
            case 'r': { /* rendering options */
                if (optarg != NULL) { /* is optional argument provided? */
                    PRINTF_S("-r %s\n", optarg);
                    if (optarg[0] == 'c') {
                        if (optarg[1] == '1') {
                            l_colorPalette = l_darkPalette;
                        }
                        else if (optarg[1] == '2') {
                            l_colorPalette = l_lightPalette;
                        }
                        else {
                            l_colorPalette = (char const * const *)0;
                        }
                    }
                }
                break;
            }
            case 'k': { /* suppress keyboard input */
                l_kbd_inp = false;
                break;
            }
            case 'o': { /* save screen output to a file */
                SNPRINTF_S(l_outFileName, sizeof(l_outFileName) - 1U,
                           "qspy%s.txt", l_tstampStr);
                PRINTF_S("-o (%s)\n", l_outFileName);
                break;
            }
            case 's': { /* save binary data to a file */
                SNPRINTF_S(l_savFileName, sizeof(l_savFileName) - 1U,
                           "qspy%s.bin", l_tstampStr);
                PRINTF_S("-s (%s)\n", l_savFileName);
                break;
            }
            case 'm': { /* Matlab/Octave file output */
                SNPRINTF_S(l_matFileName, sizeof(l_matFileName) - 1U,
                           "qspy%s.mat", l_tstampStr);
                PRINTF_S("-m (%s)\n", l_matFileName);
                break;
            }
            case 'g': { /* Sequence file output */
                if (optarg != NULL) { /* is optional argument provided? */
                    STRNCPY_S(l_seqList, sizeof(l_seqList), optarg);
                }
                else {
                    FPRINTF_S(stderr, "%s",
                         "empty object-list for sequence diagram");
                    return QSPY_ERROR;
                }
                SNPRINTF_S(l_seqFileName, sizeof(l_seqFileName) - 1U,
                           "qspy%s.seq", l_tstampStr);
                PRINTF_S("-g %s (%s)\n", l_seqList, l_seqFileName);
                break;
            }
            case 'c': { /* COM port */
                if ((l_link != NO_LINK) && (l_link != SERIAL_LINK)) {
                    FPRINTF_S(stderr, "%s\n",
                            "The -c option is incompatible with -t/-f");
                    return QSPY_ERROR;
                }
                STRNCPY_S(l_comPort, sizeof(l_comPort), optarg);
                PRINTF_S("-c %s\n", l_comPort);
                l_link = SERIAL_LINK;
                break;
            }
            case 'b': { /* baud rate */
                if ((l_link != NO_LINK) && (l_link != SERIAL_LINK)) {
                    FPRINTF_S(stderr, "%s\n",
                        "The -b option is incompatible with -t/-f");
                    return QSPY_ERROR;
                }
                l_baudRate = (int)strtol(optarg, NULL, 10);
                if (l_baudRate == 0) {
                    FPRINTF_S(stderr, "incorrect baud rate: %s\n", optarg);
                    return QSPY_ERROR;
                }
                PRINTF_S("-b %d\n", l_baudRate);
                l_link = SERIAL_LINK;
                break;
            }
            case 'f': { /* File input */
                if (l_link != NO_LINK) {
                    FPRINTF_S(stderr, "%s\n",
                            "The -f option is incompatible with -c/-b/-t");
                    return QSPY_ERROR;
                }
                STRNCPY_S(l_inpFileName, sizeof(l_inpFileName), optarg);
                PRINTF_S("-f %s\n", l_inpFileName);
                l_link = FILE_LINK;
                break;
            }
            case 'd': { /* Dictionary file */
                if (optarg != NULL) { /* is optional argument provided? */
                    STRNCPY_S(l_dicFileName, sizeof(l_dicFileName), optarg);
                    PRINTF_S("-d %s\n", l_dicFileName);
                }
                else { /* apply the default */
                    l_dicFileName[0] = '?';
                    l_dicFileName[1] = '\0';
                    PRINTF_S("%s\n", "-d");
                }
                break;
            }
            case 't': { /* TCP/IP input */
                if ((l_link != NO_LINK) && (l_link != TCP_LINK)) {
                    FPRINTF_S(stderr, "%s\n",
                            "The -t option is incompatible with -c/-b/-f");
                    return QSPY_ERROR;
                }
                if (optarg != NULL) { /* is optional argument provided? */
                    l_tcpPort = (int)strtoul(optarg, NULL, 10);
                }
                PRINTF_S("-t %d\n", l_tcpPort);
                l_link = TCP_LINK;
                break;
            }
            case 'p': { /* TCP/IP port number */
                FPRINTF_S(stderr, "%s\n",
                        "The -p option is obsolete, use -t[port]");
                return QSPY_ERROR;
                break;
            }
            case 'T': { /* timestamp size */
                config.tstampSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'F': { /* function pointer size */
                config.funPtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'O': { /* object pointer size */
                config.objPtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'S': { /* signal size */
                config.sigSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
                }
            case 'E': { /* event size */
                config.evtSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'Q': { /* Queue counter size */
                config.queueCtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'P': { /* Memory-pool counter size */
                config.poolCtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'B': { /* Memory-pool blocksize size */
                config.poolBlkSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'C': { /* Time event counter size */
                config.tevtCtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'h': { /* help */
                PRINTF_S("\n%s\n%s", l_helpStr, l_kbdHelpStr);
                return QSPY_ERROR;
            }
            case '?': /* intentionally fall through */
            case '!': /* intentionally fall through */
            case '$': {
                PRINTF_S("\n%s\n%s", l_helpStr, l_kbdHelpStr);
                return QSPY_ERROR;
            }
            default: {
                Q_ASSERT(0);
            }
        }
    }
    if (argc != optind) {
        FPRINTF_S(stderr,
            "%d command-line options were not processed\n", (argc - optind));
        PRINTF_S("\n%s\n%s", l_helpStr, l_kbdHelpStr);
        return QSPY_ERROR;
    }

    /* open the keyboard input... */
    if (PAL_openKbd(l_kbd_inp, (l_colorPalette != (char const * const*)0))
        != QSPY_SUCCESS)
    {
        return QSPY_ERROR;
    }

    /* configure QSPY ......................................................*/
    /* open Back-End link. NOTE: must happen *before* opening Target link */
    if (l_bePort != 0) {
        PRINTF_S("-u %d\n", l_bePort);
        if (PAL_openBE(l_bePort) == QSPY_ERROR) {
            return QSPY_ERROR;
        }
    }

    /* open Target link... */
    switch (l_link) {
        case NO_LINK:
            PRINTF_S("-t %d\n", l_tcpPort); /* -t is the default link */
            /* fall through */
        case TCP_LINK: {    /* connect to the Target via TCP socket */
            if (PAL_openTargetTcp(l_tcpPort) != QSPY_SUCCESS) {
                return QSPY_ERROR;
            }
            break;
        }
        case SERIAL_LINK: { /* connect to the Target via serial port */
            if (PAL_openTargetSer(l_comPort, l_baudRate) != QSPY_SUCCESS) {
                return QSPY_ERROR;
            }
            break;
        }
        case FILE_LINK: {   /* input QS data from a file */
            if (PAL_openTargetFile(l_inpFileName) != QSPY_SUCCESS) {
                return QSPY_ERROR;
            }
            break;
        }
    }

    /* open files specified on the command line... */
    if (l_outFileName[0] != 'O') { /* "OFF" ? */
        FOPEN_S(l_outFile, l_outFileName, "w");
        if (l_outFile != (FILE *)0) {
            FPRINTF_S(l_outFile, l_introStr, QSPY_VER, l_tstampStr);
        }
        else {
            PRINTF_S("   <QSPY-> Cannot open File=%s\n", l_outFileName);
            return QSPY_ERROR;
        }
    }
    if (l_savFileName[0] != 'O') { /* "OFF" ? */
        FOPEN_S(l_savFile, l_savFileName, "wb"); /* open for writing binary */
        if (l_savFile == (FILE *)0) {
            PRINTF_S("   <QSPY-> Cannot open File=%s\n", l_savFileName);
            return QSPY_ERROR;
        }
    }
    if (l_matFileName[0] != 'O') { /* "OFF" ? */
        FOPEN_S(l_matFile, l_matFileName, "w");
        if (l_matFile == (FILE *)0) {
            PRINTF_S("   <QSPY-> Cannot open File=%s\n", l_matFileName);
            return QSPY_ERROR;
        }
    }
    if (l_seqFileName[0] != 'O') { /* "OFF" ? */
        FOPEN_S(l_seqFile, l_seqFileName, "w");
        if (l_seqFile == (FILE *)0) {
            PRINTF_S("   <QSPY-> Cannot open File=%s\n", l_seqFileName);
            return QSPY_ERROR;
        }
    }
    QSPY_config(&config,
                (l_bePort != 0)
                    ? &BE_parseRecFromTarget
                    : (QSPY_CustParseFun)0);
    QSPY_configMatFile(l_matFile);
    QSEQ_config(l_seqFile, l_seqList);
    QSPY_configTxReset(&QSPY_txReset);

    /* NOTE: dictionary file must be set and read AFTER configuring QSPY */
    if (l_dicFileName[0] != 'O') { /* not "OFF" ? */
        QSPY_setExternDict(l_dicFileName);
        QSPY_readDict();
    }

    return QSPY_SUCCESS;
}
/*..........................................................................*/
bool QSPY_command(uint8_t cmdId, uint8_t opt) {
    uint32_t nBytes;
    bool isRunning = true;
    QSpyStatus stat;

    switch (cmdId) {
        default:
            PRINTF_S("   <QSPY-> Unrecognized keyboard Command=%c",
                   (char)cmdId);
            /* intentionally fall-through... */

        case 'h':  /* keyboard help */
            PRINTF_S("\n%s\n", l_kbdHelpStr);
            if (l_quiet < 0) {
                PRINTF_S("Quiet Mode    [q]: %s\n", "OFF");
            }
            else {
                PRINTF_S("Quiet Mode    [q]: %d\n", l_quiet);
            }
            PRINTF_S("Screen   Output [o]: %s\n", l_outFileName);
            PRINTF_S("Binary   Output [s]: %s\n", l_savFileName);
            PRINTF_S("Matlab   Output [m]: %s\n", l_matFileName);
            PRINTF_S("Sequence Output [g]: %s\n", l_seqFileName);
            break;

        case 'r':  /* send RESET command to the Target */
            nBytes = QSPY_encodeResetCmd(l_buf, sizeof(l_buf));
            stat = (*PAL_vtbl.send2Target)(l_buf, nBytes);
            PRINTF_S("   <USER-> Sending RESET to the Target Stat=%s\n",
                          (stat == QSPY_ERROR) ? "ERROR" : "OK");
            break;

        case 't':  /* send TICK[0] command to the Target */
            nBytes = QSPY_encodeTickCmd(l_buf, sizeof(l_buf), 0U);
            stat = (*PAL_vtbl.send2Target)(l_buf, nBytes);
            PRINTF_S("   <USER-> Sending TICK-0 to the Target Stat=%s\n",
                          (stat == QSPY_ERROR) ? "ERROR" : "OK");
            break;

        case 'u':  /* send TICK[1] command to the Target */
            nBytes = QSPY_encodeTickCmd(l_buf, sizeof(l_buf), 1U);
            stat = (*PAL_vtbl.send2Target)(l_buf, nBytes);
            PRINTF_S("   <USER-> Sending TICK-1 to the Target Stat=%s\n",
                   (stat == QSPY_ERROR) ? "ERROR" : "OK");
            break;

        case 'i':  /* send INFO request command to the Target */
            nBytes = QSPY_encodeInfoCmd(l_buf, sizeof(l_buf));
            stat = (*PAL_vtbl.send2Target)(l_buf, nBytes);
            PRINTF_S("   <USER-> Sending INFO to the Target Stat=%s\n",
                   (stat == QSPY_ERROR) ? "ERROR" : "OK");
            break;

        case 'd':  /* save Dictionaries to a file */
            QSPY_writeDict();
            break;

        case 'c':  /* clear the screen */
            PAL_clearScreen();
            break;

        case 'q':  /* quiet */
            if (l_quiet < 0) {
                l_quiet = l_quiet_ctr;
                PRINTF_S("   <USER-> Quiet Mode [q] Mode=%d\n", l_quiet);
            }
            else {
                l_quiet_ctr = l_quiet;
                l_quiet = -1;
                PRINTF_S("   <USER-> Quiet Mode [q] Mode=%s\n", "OFF");
            }
            break;

        case 'o':  /* text output file open/close/toggle */
            if (l_outFile != (FILE *)0) {
                fclose(l_outFile);
                l_outFile = (FILE *)0;
                STRNCPY_S(l_outFileName, sizeof(l_outFileName), "OFF");
                if (opt == CMD_OPT_TOGGLE) {
                    opt = CMD_OPT_OFF;
                }
            }
            if ((opt == CMD_OPT_ON) || (opt == CMD_OPT_TOGGLE)) {
                STRNCPY_S(l_tstampStr, sizeof(l_tstampStr), QSPY_tstampStr());
                SNPRINTF_S(l_outFileName, sizeof(l_outFileName),
                           "qspy%s.txt", l_tstampStr);
                FOPEN_S(l_outFile, l_outFileName, "w");
                if (l_outFile != (FILE *)0) {
                    FPRINTF_S(l_outFile, l_introStr, QSPY_VER,
                              l_tstampStr);
                }
                else {
                    PRINTF_S("   <QSPY-> Cannot open File=%s for writing\n",
                             l_outFileName);
                    STRNCPY_S(l_outFileName, sizeof(l_outFileName), "OFF");
                }
            }
            PRINTF_S("   <USER-> Screen Output [o] File=%s\n", l_outFileName);
            break;

        case 'b':
        case 's':  /* binary output file open/close/toggle */
            if (l_savFile != (FILE *)0) {
                fclose(l_savFile);
                l_savFile = (FILE *)0;
                STRNCPY_S(l_savFileName, sizeof(l_savFileName), "OFF");
                if (opt == CMD_OPT_TOGGLE) {
                    opt = CMD_OPT_OFF;
                }
            }
            if ((opt == CMD_OPT_ON) || (opt == CMD_OPT_TOGGLE)) {
                SNPRINTF_S(l_savFileName, sizeof(l_savFileName),
                           "qspy%s.bin", QSPY_tstampStr());
                FOPEN_S(l_savFile, l_savFileName, "wb");
                if (l_savFile == (FILE *)0) {
                    PRINTF_S("   <QSPY-> Cannot open File=%s for writing\n",
                             l_savFileName);
                    STRNCPY_S(l_savFileName, sizeof(l_savFileName), "OFF");
                }
            }
            PRINTF_S("   <USER-> Binary Output [s] File=%s\n",
                   l_savFileName);
            break;

        case 'm':  /* Matlab output file open/close/toggle */
            if (l_matFile != (FILE *)0) {
                QSPY_configMatFile((void *)0); /* close the Matlab file */
                l_matFile = (FILE *)0;
                STRNCPY_S(l_matFileName, sizeof(l_matFileName), "OFF");
                if (opt == CMD_OPT_TOGGLE) {
                    opt = CMD_OPT_OFF;
                }
            }
            if ((opt == CMD_OPT_ON) || (opt == CMD_OPT_TOGGLE)) {
                SNPRINTF_S(l_matFileName, sizeof(l_matFileName),
                           "qspy%s.mat", QSPY_tstampStr());
                FOPEN_S(l_matFile, l_matFileName, "w");
                if (l_matFile != (FILE *)0) {
                    QSPY_configMatFile(l_matFile);
                }
                else {
                    PRINTF_S("   <QSPY-> Cannot open File=%s for writing\n",
                             l_matFileName);
                    STRNCPY_S(l_matFileName, sizeof(l_matFileName), "OFF");
                }
            }
            PRINTF_S("   <USER-> Matlab Output [m] File=%s\n",
                   l_matFileName);
            break;

        case 'g':  /* Sequence output file open/close/toggle */
            if (l_seqList[0] == '\0') {
                SNPRINTF_LINE("   <QSPY-> Sequence list NOT provided %s ",
                              "(no -g option)");
                QSPY_printError();
                break;
            }
            if (l_seqFile != (FILE *)0) {
                QSEQ_configFile((void *)0); /* close the Sequence file */
                l_seqFile = (FILE *)0;
                STRNCPY_S(l_seqFileName, sizeof(l_seqFileName), "OFF");
                if (opt == CMD_OPT_TOGGLE) {
                    opt = CMD_OPT_OFF;
                }
            }
            if ((opt == CMD_OPT_ON) || (opt == CMD_OPT_TOGGLE)) {
                SNPRINTF_S(l_seqFileName, sizeof(l_seqFileName),
                           "qspy%s.seq", QSPY_tstampStr());
                FOPEN_S(l_seqFile, l_seqFileName, "w");
                if (l_seqFile != (FILE *)0) {
                    QSEQ_configFile(l_seqFile);
                }
                else {
                    PRINTF_S("   <QSPY-> Cannot open File=%s for writing\n",
                             l_seqFileName);
                    STRNCPY_S(l_seqFileName, sizeof(l_seqFileName), "OFF");
                }
            }
            PRINTF_S("   <USER-> Sequence Output [g] File=%s\n",
                   l_seqFileName);
            break;

        case 'x':
        case 'X':
        case '\x1b': /* Esc */
            isRunning = false; /* terminate the event loop */
            break;
    }

    return isRunning;
}
/*..........................................................................*/
char const* QSPY_tstampStr(void) {
    time_t rawtime = time(NULL);
    struct tm tstamp;
    static char tstampStr[64];

    LOCALTIME_S(&tstamp, &rawtime);

    SNPRINTF_S(tstampStr, sizeof(tstampStr), "%02d%02d%02d_%02d%02d%02d",
        (tstamp.tm_year + 1900) % 100,
        (tstamp.tm_mon + 1),
        tstamp.tm_mday,
        tstamp.tm_hour,
        tstamp.tm_min,
        tstamp.tm_sec);

    return &tstampStr[0];
}

/*--------------------------------------------------------------------------*/
/* color output to the terminal */

/* terminal colors */
#define B_DFLT_EOL "\x1b[K\x1b[0m"
#define B_DFLT     "\x1b[0m"
#define B_BLACK    "\x1b[40m"
#define B_RED      "\x1b[41m"
#define B_GREEN    "\x1b[42m"
#define B_YELLOW   "\x1b[43m"
#define B_BLUE     "\x1b[44m"
#define B_MAGENTA  "\x1b[45m"
#define B_CYAN     "\x1b[46m"
#define B_WHITE    "\x1b[47m"

#define F_BLACK    "\x1b[30m"
#define F_RED      "\x1b[31m"
#define F_GREEN    "\x1b[32m"
#define F_YELLOW   "\x1b[33m"
#define F_BLUE     "\x1b[34m"
#define F_MAGENTA  "\x1b[35m"
#define F_CYAN     "\x1b[36m"
#define F_WHITE    "\x1b[37m"

#define F_GRAY     "\x1b[30;1m"
#define F_BRED     "\x1b[31;1m"
#define F_BGREEN   "\x1b[32;1m"
#define F_BYELLOW  "\x1b[33;1m"
#define F_BBLUE    "\x1b[34;1m"
#define F_BMAGENTA "\x1b[35;1m"
#define F_BCYAN    "\x1b[36;1m"
#define F_BWHITE   "\x1b[37;1m"

/* color palette entries */
enum {
    PALETTE_ERR_OUT,
    PALETTE_INF_OUT,
    PALETTE_TST_OUT,
    PALETTE_USR_OUT,
    PALETTE_TSTAMP,
    PALETTE_DSC_SM,
    PALETTE_DSC_QP,
    PALETTE_DSC_INF,
    PALETTE_DSC_TST,
    PALETTE_SM_TXT,
    PALETTE_QP_TXT,
    PALETTE_INF_TXT,
    PALETTE_DIC_TXT,
    PALETTE_TST_TXT,
    PALETTE_USR_TXT,
};

char const * const l_darkPalette[] = {
/* PALETTE_ERR_OUT */            B_RED     F_BYELLOW,
/* PALETTE_INF_OUT */    B_DFLT  B_GREEN   F_BLACK,
/* PALETTE_TST_OUT */            B_BLACK   F_BYELLOW,
/* PALETTE_USR_OUT */            B_BLACK   F_WHITE,
/* PALETTE_TSTAMP  */            B_BLACK   F_CYAN,
/* PALETTE_DSC_SM  */            B_BLUE    F_WHITE,
/* PALETTE_DSC_QP  */    B_DFLT  B_MAGENTA F_WHITE,
/* PALETTE_DSC_INF */    B_DFLT  B_BLUE    F_BYELLOW,
/* PALETTE_DSC_TST */    B_DFLT  B_CYAN    F_BWHITE,
/* PALETTE_SM_TXT  */            B_BLACK   F_BWHITE,
/* PALETTE_QP_TXT  */            B_BLACK   F_WHITE,
/* PALETTE_INF_TXT */    B_DFLT  B_BLACK   F_BYELLOW,
/* PALETTE_DIC_TXT */            B_BLACK   F_CYAN,
/* PALETTE_TST_TXT */    B_DFLT  B_BLACK   F_YELLOW,
/* PALETTE_USR_TXT */    B_DFLT  B_WHITE   F_BLACK,
};

char const * const l_lightPalette[] = {
/* PALETTE_ERR_OUT */            B_RED     F_BYELLOW,
/* PALETTE_INF_OUT */    B_DFLT  B_GREEN   F_BLACK,
/* PALETTE_TST_OUT */            B_BLACK   F_BYELLOW,
/* PALETTE_USR_OUT */            B_BLACK   F_WHITE,
/* PALETTE_TSTAMP  */            B_WHITE   F_GRAY,
/* PALETTE_DSC_SM  */            B_BLUE    F_WHITE,
/* PALETTE_DSC_QP  */    B_DFLT  B_MAGENTA F_WHITE,
/* PALETTE_DSC_INF */    B_DFLT  B_BLUE    F_BYELLOW,
/* PALETTE_DSC_TST */    B_DFLT  B_CYAN    F_BWHITE,
/* PALETTE_SM_TXT  */            B_WHITE   F_BLUE,
/* PALETTE_QP_TXT  */            B_WHITE   F_MAGENTA,
/* PALETTE_INF_TXT */    B_DFLT  B_WHITE   F_RED,
/* PALETTE_DIC_TXT */            B_WHITE   F_GRAY,
/* PALETTE_TST_TXT */    B_DFLT  B_WHITE   F_BLUE,
/* PALETTE_USR_TXT */    B_DFLT  B_BLACK   F_WHITE,
};

enum {
    COL_TSTAMP = 11,
    COL_DESC   = 19,
};

static void colorPrintLn(void) {
    if (QSPY_output.type == REG_OUT) {
        int group = QSPY_output.rec < QS_USER
                   ? QSPY_rec[QSPY_output.rec].group
                   : GRP_USR;

        /* timestamp */
        char ch = QSPY_output.buf[QS_LINE_OFFSET + COL_TSTAMP];
        QSPY_output.buf[QS_LINE_OFFSET + COL_TSTAMP] = '\0';
        fputs(l_colorPalette[PALETTE_TSTAMP], stdout);
        fputs(&QSPY_output.buf[QS_LINE_OFFSET], stdout);
        QSPY_output.buf[QS_LINE_OFFSET + COL_TSTAMP] = ch;

        switch (group) {
        case GRP_ERR: {
            fputs(l_colorPalette[PALETTE_ERR_OUT], stdout);
            fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_TSTAMP], stdout);
            break;
        }
        case GRP_INF: {
            /* description section */
            ch = QSPY_output.buf[QS_LINE_OFFSET + COL_DESC];
            QSPY_output.buf[QS_LINE_OFFSET + COL_DESC] = '\0';
            fputs(l_colorPalette[PALETTE_DSC_INF], stdout);
            fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_TSTAMP], stdout);
            QSPY_output.buf[QS_LINE_OFFSET + COL_DESC] = ch;
            if (QSPY_output.len > COL_DESC) {
                fputs(l_colorPalette[PALETTE_INF_TXT], stdout);
                fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_DESC], stdout);
            }
            break;
        }
        case GRP_DIC: {
            fputs(l_colorPalette[PALETTE_DIC_TXT], stdout);
            fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_TSTAMP], stdout);
            break;
        }
        case GRP_TST: {
            /* description section */
            ch = QSPY_output.buf[QS_LINE_OFFSET + COL_DESC];
            QSPY_output.buf[QS_LINE_OFFSET + COL_DESC] = '\0';
            fputs(l_colorPalette[PALETTE_DSC_TST], stdout);
            fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_TSTAMP], stdout);
            QSPY_output.buf[QS_LINE_OFFSET + COL_DESC] = ch;
            if (QSPY_output.len > COL_DESC) {
                fputs(l_colorPalette[PALETTE_TST_TXT], stdout);
                fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_DESC], stdout);
            }
            break;
        }
        case GRP_SM: {
            /* description section */
            ch = QSPY_output.buf[QS_LINE_OFFSET + COL_DESC];
            QSPY_output.buf[QS_LINE_OFFSET + COL_DESC] = '\0';
            fputs(l_colorPalette[PALETTE_DSC_SM], stdout);
            fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_TSTAMP], stdout);
            QSPY_output.buf[QS_LINE_OFFSET + COL_DESC] = ch;
            if (QSPY_output.len > COL_DESC) {
                fputs(l_colorPalette[PALETTE_SM_TXT], stdout);
                fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_DESC], stdout);
            }
            break;
        }
        case GRP_AO:
        case GRP_EQ:
        case GRP_MP:
        case GRP_TE:
        case GRP_QF:
        case GRP_SC:
        case GRP_SEM:
        case GRP_MTX: {
            /* description section */
            ch = QSPY_output.buf[QS_LINE_OFFSET + COL_DESC];
            QSPY_output.buf[QS_LINE_OFFSET + COL_DESC] = '\0';
            fputs(l_colorPalette[PALETTE_DSC_QP], stdout);
            fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_TSTAMP], stdout);
            QSPY_output.buf[QS_LINE_OFFSET + COL_DESC] = ch;
            if (QSPY_output.len > COL_DESC) {
                fputs(l_colorPalette[PALETTE_QP_TXT], stdout);
                fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_DESC], stdout);
            }
            break;
        }
        case GRP_USR: /* intentionally fall through */
        default: {
            fputs(l_colorPalette[PALETTE_USR_TXT], stdout);
            fputs(&QSPY_output.buf[QS_LINE_OFFSET + COL_TSTAMP], stdout);
            break;
        }
        }
        fputs(B_DFLT "\n", stdout);
    }
    else if (QSPY_output.type == INF_OUT) {
        fputs(l_colorPalette[PALETTE_INF_OUT], stdout);
        fputs(&QSPY_output.buf[QS_LINE_OFFSET], stdout);
        fputs(B_DFLT_EOL "\n", stdout);
    }
    else if (QSPY_output.type == ERR_OUT) {
        fputs(l_colorPalette[PALETTE_ERR_OUT], stdout);
        fputs(&QSPY_output.buf[QS_LINE_OFFSET], stdout);
        fputs(B_DFLT_EOL "\n", stdout);
    }
    else if (QSPY_output.type == TST_OUT) {
        fputs(l_colorPalette[PALETTE_TST_OUT], stdout);
        fputs(&QSPY_output.buf[QS_LINE_OFFSET], stdout);
        fputs(B_DFLT_EOL "\n", stdout);
    }
    else { /* USR_OUT */
        fputs(l_colorPalette[PALETTE_USR_OUT], stdout);
        fputs(&QSPY_output.buf[QS_LINE_OFFSET], stdout);
        fputs(B_DFLT_EOL "\n", stdout);
    }
}
