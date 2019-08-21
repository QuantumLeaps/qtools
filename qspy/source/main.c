/**
* @file
* @brief main for QSPY host utility
* @ingroup qpspy
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
* www.state-machine.com
* info@state-machine.com
******************************************************************************
* @endcond
*/
#include <stdint.h>
#include <stddef.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include "qspy.h"   /* QSPY data parser */
#include "be.h"     /* Back-End interface */
#include "pal.h"    /* Platform Abstraction Layer */
#include "getopt.h" /* command-line option processor */

/*..........................................................................*/
enum { FNAME_SIZE = 256 };

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
static FILE *l_outFile = (FILE *)0;
static FILE *l_savFile = (FILE *)0;
static FILE *l_matFile = (FILE *)0;
static FILE *l_mscFile = (FILE *)0;

static char  l_comPort    [FNAME_SIZE];
static char  l_inpFileName[FNAME_SIZE];
static char  l_outFileName[FNAME_SIZE];
static char  l_savFileName[FNAME_SIZE];
static char  l_matFileName[FNAME_SIZE];
static char  l_mscFileName[FNAME_SIZE];
static char  l_dicFileName[FNAME_SIZE];

static char  l_tstampStr  [16];

static int   l_bePort   = 7701;   /* default UDP port  */
static int   l_tcpPort  = 6601;   /* default TCP port */
static int   l_baudRate = 115200; /* default serial baudrate */

static char const l_introStr[] =
    "QSPY %s Copyright (c) 2005-2019 Quantum Leaps\n"
    "Documentation: https://www.state-machine.com/qtools/qspy.html\n"
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
    "-v <QS_version>   6.2     compatibility with QS version\n"
    "-o                (key-o) save screen output to a file\n"
    "-s                (key-s) save binary QS data to a file\n"
    "-m                        produce Matlab output to a file\n"
    "-g                        produce MscGen output to a file\n"
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
    "-C <counter_size> 2       QTimeEvt counter size (bytes)\n";

static char const l_kbdHelpStr[] =
    "Keyboard shortcuts:\n"
    "KEY(s)            ACTION\n"
    "-----------------------------------------------------------------\n"
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
    "  g               toggle MscGen file output (close/re-open)\n";

/*..........................................................................*/
static QSpyStatus configure(int argc, char *argv[]);
static void cleanup(void);
static char const *tstampStr(void);
static uint8_t l_buf[8*1024]; /* process input in 8K chunks */

/*..........................................................................*/
int main(int argc, char *argv[]) {
    int status = 0;

    /* parse the command-line options and configure QSPY ...................*/
    if (configure(argc, argv) != QSPY_SUCCESS) {
        status = -1;
    }
    else {
        size_t nBytes;
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
                    isRunning = QSPY_command(l_buf[0]);
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
    printf("\nQSPY Done\n");
    return status;
}

/*..........................................................................*/
static void cleanup(void) {
    if (l_savFile != (FILE *)0) {
        fclose(l_savFile);
    }
    if (l_outFile != (FILE *)0) {
        fclose(l_outFile);
    }

    QSPY_stop();  /* update and close all other open files */

    if (l_bePort != 0) {
        PAL_closeBE();          /* close the Back-End connection */
    }

    if (PAL_vtbl.cleanup != 0) {
        (*PAL_vtbl.cleanup)();  /* close the target connection */
    }
}

/*..........................................................................*/
void Q_onAssert(char const * const module, int loc) {
    printf("\n   <ERROR> QSPY ASSERTION failed in Module=%s:%d\n",
           module, loc);
    cleanup();
    exit(-1);
}

/*..........................................................................*/
void QSPY_onPrintLn(void) {
    if (l_quiet < 0) {
        fputs(QSPY_line, stdout);
        fputc('\n', stdout);
    }
    else if (l_quiet > 0) {
        if ((l_quiet_ctr == 0U) || (QSPY_output.type != REG_OUT)) {
            if ((l_quiet < 99) || (QSPY_output.type != REG_OUT)) {
                if (l_quiet_ctr != l_quiet - 1) {
                    fputc('\n', stdout);
                }
                fputs(&QSPY_output.buf[QS_LINE_OFFSET], stdout);
                fputc('\n', stdout);
                l_quiet_ctr = l_quiet;
            }
        }
        else {
            fputc('.', stdout);
        }
        --l_quiet_ctr;
    }

    if (l_outFile != (FILE *)0) {
        /* the output file receives all trace records, regardles of -q mode */
        fputs(&QSPY_output.buf[QS_LINE_OFFSET], l_outFile);
        fputc('\n', l_outFile);
    }

    if (QSPY_output.type != INF_OUT) { /* just an internal info? */
        BE_sendLine(); /* forward to the back-end */
    }

    QSPY_output.type = REG_OUT; /* reset for the next time */
}
/*..........................................................................*/
void QSPY_printInfo(void) {
    QSPY_output.type = INF_OUT; /* this is an internal info message */
    QSPY_onPrintLn();
}

/*..........................................................................*/
static QSpyStatus configure(int argc, char *argv[]) {
    static char const getoptStr[] =
        "hq::u::v:osmgc:b:t::p:f:d::T:O:F:S:E:Q:P:B:C:";

    /* default configuration options... */
    uint16_t version     = 620U;
    uint8_t tstampSize   = 4U;
    uint8_t objPtrSize   = 4U;
    uint8_t funPtrSize   = 4U;
    uint8_t sigSize      = 2U;
    uint8_t evtSize      = 2U;
    uint8_t queueCtrSize = 1U;
    uint8_t poolCtrSize  = 2U;
    uint8_t poolBlkSize  = 2U;
    uint8_t tevtCtrSize  = 2U;
    int     optChar;

    STRNCPY_S(l_outFileName, "OFF", sizeof(l_outFileName) - 1U);
    STRNCPY_S(l_savFileName, "OFF", sizeof(l_savFileName) - 1U);
    STRNCPY_S(l_matFileName, "OFF", sizeof(l_matFileName) - 1U);
    STRNCPY_S(l_mscFileName, "OFF", sizeof(l_mscFileName) - 1U);
    STRNCPY_S(l_dicFileName, "OFF", sizeof(l_dicFileName) - 1U);

    (void)tstampStr();
    printf(l_introStr, QSPY_VER, l_tstampStr);

    STRNCPY_S(l_inpFileName, "qs.bin", sizeof(l_inpFileName) - 1U);
#ifdef _WIN32
    STRNCPY_S(l_comPort, "COM1", sizeof(l_comPort) - 1U);
#elif (defined __linux) || (defined __linux__) || (defined __posix)
    STRNCPY_S(l_comPort, "/dev/ttyS0", sizeof(l_comPort) - 1U);
#endif

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
                    version = (((optarg[0] - '0') * 10)
                              + (optarg[2] - '0')) * 10;
                    printf("-v %c.%c\n", optarg[0], optarg[2]);
                }
                else {
                    fprintf(stderr, "Incorrect version number: %s", optarg);
                    return QSPY_ERROR;
                }
                break;
            }
            case 'o': { /* save screen output to a file */
                SNPRINTF_S(l_outFileName, sizeof(l_outFileName) - 1U,
                           "qspy%s.txt", l_tstampStr);
                printf("-o (%s)\n", l_outFileName);
                break;
            }
            case 's': { /* save binary data to a file */
                SNPRINTF_S(l_savFileName, sizeof(l_savFileName) - 1U,
                           "qspy%s.bin", l_tstampStr);
                printf("-s (%s)\n", l_savFileName);
                break;
            }
            case 'm': { /* Matlab/Octave file output */
                SNPRINTF_S(l_matFileName, sizeof(l_matFileName) - 1U,
                           "qspy%s.mat", l_tstampStr);
                printf("-m (%s)\n", l_matFileName);
                break;
            }
            case 'g': { /* MscGen file output */
                SNPRINTF_S(l_mscFileName, sizeof(l_mscFileName) - 1U,
                           "qspy%s.msc", l_tstampStr);
                printf("-g (%s)\n", l_mscFileName);
                break;
            }
            case 'c': { /* COM port */
                if ((l_link != NO_LINK) && (l_link != SERIAL_LINK)) {
                    fprintf(stderr,
                            "The -c option is incompatible with -t/-f\n");
                    return QSPY_ERROR;
                }
                STRNCPY_S(l_comPort, optarg, sizeof(l_comPort) - 1U);
                printf("-c %s\n", l_comPort);
                l_link = SERIAL_LINK;
                break;
            }
            case 'b': { /* baud rate */
                if ((l_link != NO_LINK) && (l_link != SERIAL_LINK)) {
                    fprintf(stderr,
                        "The -b option is incompatible with -t/-f\n");
                    return QSPY_ERROR;
                }
                if (SSCANF_S(optarg, "%d", &l_baudRate) != 1) {
                    fprintf(stderr, "incorrect baud rate: %s\n", optarg);
                    return QSPY_ERROR;
                }
                printf("-b %d\n", l_baudRate);
                l_link = SERIAL_LINK;
                break;
            }
            case 'f': { /* File input */
                if (l_link != NO_LINK) {
                    fprintf(stderr,
                            "The -f option is incompatible with -c/-b/-t\n");
                    return QSPY_ERROR;
                }
                STRNCPY_S(l_inpFileName, optarg, sizeof(l_inpFileName) - 1U);
                printf("-f %s\n", l_inpFileName);
                l_link = FILE_LINK;
                break;
            }
            case 'd': { /* Dictionary file */
                if (optarg != NULL) { /* is optional argument provided? */
                    STRNCPY_S(l_dicFileName, optarg,
                              sizeof(l_dicFileName) - 1U);
                    printf("-d %s\n", l_dicFileName);
                }
                else { /* apply the default */
                    l_dicFileName[0] = '?';
                    l_dicFileName[1] = '\0';
                    printf("-d\n");
                }
                break;
            }
            case 't': { /* TCP/IP input */
                if ((l_link != NO_LINK) && (l_link != TCP_LINK)) {
                    fprintf(stderr,
                            "The -t option is incompatible with -c/-b/-f\n");
                    return QSPY_ERROR;
                }
                if (optarg != NULL) { /* is optional argument provided? */
                    l_tcpPort = (int)strtoul(optarg, NULL, 10);
                }
                printf("-t %d\n", l_tcpPort);
                l_link = TCP_LINK;
                break;
            }
            case 'p': { /* TCP/IP port number */
                fprintf(stderr,
                        "The -p option is obsolete, use -t[port]\n");
                return QSPY_ERROR;
                break;
            }
            case 'T': { /* timestamp size */
                tstampSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'F': { /* function pointer size */
                funPtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'O': { /* object pointer size */
                objPtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'S': { /* signal size */
                sigSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
                }
            case 'E': { /* event size */
                evtSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'Q': { /* Queue counter size */
                queueCtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'P': { /* Memory-pool counter size */
                poolCtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'B': { /* Memory-pool blocksize size */
                poolBlkSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'C': { /* Time event counter size */
                tevtCtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'h': { /* help */
                printf("\n%s\n%s", l_helpStr, l_kbdHelpStr);
                return QSPY_ERROR;
            }
            default: { /* unknown option */
                fprintf(stderr, "Unknown option -%c\n", (char)optChar);
                printf("\n%s\n%s", l_helpStr, l_kbdHelpStr);
                return QSPY_ERROR;
            }
        }
    }
    if (argc != optind) {
        fprintf(stderr,
            "%d command-line options were not processed\n", (argc - optind));
        printf("\n%s\n%s", l_helpStr, l_kbdHelpStr);
        return QSPY_ERROR;
    }

    /* configure QSPY ......................................................*/
    /* open Back-End link. NOTE: must happen *before* opening Target link */
    if (l_bePort != 0) {
        printf("-u %d\n", l_bePort);
        if (PAL_openBE(l_bePort) == QSPY_ERROR) {
            return QSPY_ERROR;
        }
    }

    /* open Target link... */
    switch (l_link) {
        case NO_LINK:
            printf("-t %d\n", l_tcpPort); /* -t is the default link */
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
            fprintf(l_outFile, l_introStr, QSPY_VER, l_tstampStr);
        }
        else {
            printf("   <QSPY-> Cannot open File=%s\n", l_outFileName);
            return QSPY_ERROR;
        }
    }
    if (l_savFileName[0] != 'O') { /* "OFF" ? */
        FOPEN_S(l_savFile, l_savFileName, "wb"); /* open for writing binary */
        if (l_savFile == (FILE *)0) {
            printf("   <QSPY-> Cannot open File=%s\n", l_savFileName);
            return QSPY_ERROR;
        }
    }
    if (l_matFileName[0] != 'O') { /* "OFF" ? */
        FOPEN_S(l_matFile, l_matFileName, "w");
        if (l_matFile == (FILE *)0) {
            printf("   <QSPY-> Cannot open File=%s\n", l_matFileName);
            return QSPY_ERROR;
        }
    }
    if (l_mscFileName[0] != 'O') { /* "OFF" ? */
        FOPEN_S(l_mscFile, l_mscFileName, "w");
        if (l_mscFile == (FILE *)0) {
            printf("   <QSPY-> Cannot open File=%s\n", l_mscFileName);
            return QSPY_ERROR;
        }
    }
    QSPY_config(version,
                objPtrSize,
                funPtrSize,
                tstampSize,
                sigSize,
                evtSize,
                queueCtrSize,
                poolCtrSize,
                poolBlkSize,
                tevtCtrSize,
                l_matFile,
                l_mscFile,
                (l_bePort != 0)
                    ? &BE_parseRecFromTarget
                    : (QSPY_CustParseFun)0);
    QSPY_configTxReset(&QSPY_txReset);

    /* NOTE: dictionary file must be set and read AFTER configuring QSPY */
    if (l_dicFileName[0] != 'O') { /* not "OFF" ? */
        QSPY_setExternDict(l_dicFileName);
        QSPY_readDict();
    }

    return QSPY_SUCCESS;
}
/*..........................................................................*/
bool QSPY_command(uint8_t cmdId) {
    size_t nBytes;
    bool isRunning = true;
    QSpyStatus stat;

    switch (cmdId) {
        default:
            printf("   <QSPY-> Unrecognized keyboard Command=%c",
                   (char)cmdId);
            /* intentionally fall-through... */

        case 'h':  /* keyboard help */
            printf("\n%s\n", l_kbdHelpStr);
            if (l_quiet < 0) {
                printf("Quiet Mode    [q]: OFF\n");
            }
            else {
                printf("Quiet Mode    [q]: %d\n", l_quiet);
            }
            printf("Screen Output [o]: %s\n", l_outFileName);
            printf("Binary Output [s]: %s\n", l_savFileName);
            printf("Matlab Output [m]: %s\n", l_matFileName);
            printf("MscGen Output [g]: %s\n", l_mscFileName);
            break;

        case 'r':  /* send RESET command to the Target */
            nBytes = QSPY_encodeResetCmd(l_buf, sizeof(l_buf));
            stat = (*PAL_vtbl.send2Target)(l_buf, nBytes);
            printf("   <USER-> Sending RESET to the Target Stat=%s\n",
                          (stat == QSPY_ERROR) ? "ERROR" : "OK");
            break;

        case 't':  /* send TICK[0] command to the Target */
            nBytes = QSPY_encodeTickCmd(l_buf, sizeof(l_buf), 0U);
            stat = (*PAL_vtbl.send2Target)(l_buf, nBytes);
            printf("   <USER-> Sending TICK-0 to the Target Stat=%s\n",
                          (stat == QSPY_ERROR) ? "ERROR" : "OK");
            break;

        case 'u':  /* send TICK[1] command to the Target */
            nBytes = QSPY_encodeTickCmd(l_buf, sizeof(l_buf), 1U);
            stat = (*PAL_vtbl.send2Target)(l_buf, nBytes);
            printf("   <USER-> Sending TICK-1 to the Target Stat=%s\n",
                   (stat == QSPY_ERROR) ? "ERROR" : "OK");
            break;

        case 'i':  /* send INFO request command to the Target */
            nBytes = QSPY_encodeInfoCmd(l_buf, sizeof(l_buf));
            stat = (*PAL_vtbl.send2Target)(l_buf, nBytes);
            printf("   <USER-> Sending INFO to the Target Stat=%s\n",
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
                printf("   <USER-> Quiet Mode [q] Mode=%d\n", l_quiet);
            }
            else {
                l_quiet_ctr = l_quiet;
                l_quiet = -1;
                printf("   <USER-> Quiet Mode [q] Mode=OFF\n");
            }
            break;

        case 'o':  /* output file open/close toggle */
            if (l_outFile != (FILE *)0) {
                fclose(l_outFile);
                l_outFile = (FILE *)0;
                STRNCPY_S(l_outFileName, "OFF", sizeof(l_outFileName));
            }
            else {
                SNPRINTF_S(l_outFileName, sizeof(l_outFileName),
                           "qspy%s.txt", tstampStr());
                FOPEN_S(l_outFile, l_outFileName, "w");
                if (l_outFile != (FILE *)0) {
                    fprintf(l_outFile, l_introStr, QSPY_VER,
                            l_tstampStr);
                }
                else {
                    printf("   <QSPY-> Cannot open File=%s for writing\n",
                           l_outFileName);
                    STRNCPY_S(l_outFileName, "OFF", sizeof(l_outFileName));
                }
            }
            printf("   <USER-> Screen Output [o] File=%s\n", l_outFileName);
            break;

        case 'b':
        case 's':  /* save binary file open/close toggle*/
            if (l_savFile != (FILE *)0) {
                fclose(l_savFile);
                l_savFile = (FILE *)0;
                STRNCPY_S(l_savFileName, "OFF", sizeof(l_savFileName));
            }
            else {
                SNPRINTF_S(l_savFileName, sizeof(l_savFileName),
                           "qspy%s.bin", tstampStr());
                FOPEN_S(l_savFile, l_savFileName, "wb");
                if (l_savFile == (FILE *)0) {
                    printf("   <QSPY-> Cannot open File=%s for writing\n",
                           l_savFileName);
                    STRNCPY_S(l_savFileName, "OFF", sizeof(l_savFileName));
                }
            }
            printf("   <USER-> Binary Output [s] File=%s\n",
                   l_savFileName);
            break;

        case 'm':  /* save Matlab file open/close toggle */
            if (l_matFile != (FILE *)0) {
                QSPY_configMatFile((void *)0); /* close the Matlab file */
                l_matFile = (FILE *)0;
                STRNCPY_S(l_matFileName, "OFF", sizeof(l_matFileName));
            }
            else {
                SNPRINTF_S(l_matFileName, sizeof(l_matFileName),
                           "qspy%s.mat", tstampStr());
                FOPEN_S(l_matFile, l_matFileName, "w");
                if (l_matFile != (FILE *)0) {
                    QSPY_configMatFile(l_matFile);
                }
                else {
                    printf("   <QSPY-> Cannot open File=%s for writing\n",
                           l_matFileName);
                    STRNCPY_S(l_matFileName, "OFF", sizeof(l_matFileName));
                }
            }
            printf("   <USER-> Matlab Output [m] File=%s\n",
                   l_matFileName);
            break;

        case 'g':  /* save MscGen file open/close toggle*/
            if (l_mscFile != (FILE *)0) {
                QSPY_configMscFile((void *)0); /* close the MscGen file */
                l_mscFile = (FILE *)0;
                STRNCPY_S(l_mscFileName, "OFF", sizeof(l_mscFileName));
            }
            else {
                SNPRINTF_S(l_mscFileName, sizeof(l_mscFileName),
                           "qspy%s.msc", tstampStr());
                FOPEN_S(l_mscFile, l_mscFileName, "w");
                if (l_mscFile != (FILE *)0) {
                    QSPY_configMscFile(l_mscFile);
                }
                else {
                    printf("   <QSPY-> Cannot open File=%s for writing\n",
                           l_mscFileName);
                    STRNCPY_S(l_mscFileName, "OFF", sizeof(l_mscFileName));
                }
            }
            printf("   <USER-> MscGen Output [g] File=%s\n",
                   l_mscFileName);
            break;

        case 'x':
        case 'X':
        case '\033': /* Esc */
            isRunning = false; /* terminate the event loop */
            break;
    }

    return isRunning;
}
/*..........................................................................*/
static char const *tstampStr(void) {
    time_t rawtime = time(NULL);
    struct tm tstamp;

    LOCALTIME_S(&tstamp, &rawtime);

    SNPRINTF_S(l_tstampStr, sizeof(l_tstampStr), "%02d%02d%02d_%02d%02d%02d",
               (tstamp.tm_year + 1900)%100,
               (tstamp.tm_mon + 1),
               tstamp.tm_mday,
               tstamp.tm_hour,
               tstamp.tm_min,
               tstamp.tm_sec);

    return &l_tstampStr[0];
}
