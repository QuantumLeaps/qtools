/*****************************************************************************
* Product: Quantum Spy -- Host resident component, main entry point
* Last updated for version 5.3.1
* Last updated on  2014-09-02
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
#include <stddef.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "qspy.h"
#include "hal.h"
#include "getopt.h"

/*..........................................................................*/
#define FNAME_SIZE 256
#define BUF_SIZE   1024

enum TargetLink {
    NO_LINK,
    FILE_LINK,
    SERIAL_LINK,
    TCP_LINK
};

/*..........................................................................*/
static uint8_t l_quiet   = 0;
static FILE   *l_outFile = (FILE *)0;

/*..........................................................................*/
int main(int argc, char *argv[]) {
    static unsigned char buf[BUF_SIZE];
    static char const help[] =
        "Syntax is: qspy [options]          * = default\n\n"
        "OPTION                    DEFAULT  COMMENT\n"
        "----------------------------------------------------------------\n"
        "-h                                 help (this message)\n"
        "-q                                 quiet mode (no stdout output)\n"
        "-v<Version_number>        5.0      compatibility with QS version\n"
        "-o<File_name>                      produce output to a file\n"
        "-s<File_name>                      save the binary data to a file\n"
        "-m<File_name>                      produce a Matlab/Octave file\n"
        "-g<File_name>                      produce a MscGen file\n"
        "-c<COM_port>  *           COM1     com port input\n"
        "-b<Baud_rate>             115200   baud rate selection\n"
        "-f<File_name>             qs.bin   file input\n"
        "-t                                 TCP/IP input\n"
        "-p<TCP_Port_number>       6601     TCP/IP server port\n"
        "-T<tstamp_size>           4        QS timestamp size (bytes)\n"
        "-O<pointer_size>          4        object pointer size (bytes)\n"
        "-F<pointer_size>          4        function pointer size (bytes)\n"
        "-S<signal_size>           2        signal size (bytes)\n"
        "-E<event_size>            2        event size size (bytes)\n"
        "-Q<queue_counter_size>    1        queue counter size (bytes)\n"
        "-P<pool_counter_size>     2        pool counter size (bytes)\n"
        "-B<pool_blocksize_size>   2        pool blocksize size (bytes)\n"
        "-C<QTimeEvt_counter_size> 2        QTimeEvt counter size\n";

    int optChar;
    char comPort[FNAME_SIZE];
    char inpFileName[FNAME_SIZE];
    char outFileName[FNAME_SIZE];
    char savFileName[FNAME_SIZE];
    char matFileName[FNAME_SIZE];
    char mscFileName[FNAME_SIZE];
    enum TargetLink link = NO_LINK;
    int n;
                                           /* default configuration options */
    int tcpPort          = 6601;
    int baudRate         = 115200;
    uint16_t version     = 500U;
    uint8_t tstampSize   = 4U;
    uint8_t objPtrSize   = 4U;
    uint8_t funPtrSize   = 4U;
    uint8_t sigSize      = 2U;
    uint8_t evtSize      = 2U;
    uint8_t queueCtrSize = 1U;
    uint8_t poolCtrSize  = 2U;
    uint8_t poolBlkSize  = 2U;
    uint8_t tevtCtrSize  = 2U;
    FILE *savFile = (FILE *)0;
    FILE *matFile = (FILE *)0;
    FILE *mscFile = (FILE *)0;
    time_t now = time(NULL);

    outFileName[0]       = '\0';               /* Output file not specified */
    savFileName[0]       = '\0';                 /* Save file not specified */
    matFileName[0]       = '\0';               /* Matlab file not specified */
    mscFileName[0]       = '\0';               /* MscGen file not specified */

    strcpy(inpFileName, "qs.bin");
    strcpy(comPort, "COM1");

    printf("QSPY host application %s\n"
           "Copyright (c) Quantum Leaps, state-machine.com\n"
           "%s\n", QSPY_VER, ctime(&now));

    while ((optChar = getopt(argc, argv,
                      "hqv:o:s:m:g:c:b:tp:f:T:O:F:S:E:Q:P:B:C:")) != -1)
    {
        switch (optChar) {
            case 'q': {                                       /* quiet mode */
                l_quiet = 1;
                break;
            }
            case 'v': {                    /* compatibility with QS version */
                if (('0' <= optarg[0] && optarg[0] <= '9')
                    && (optarg[1] == '.')
                    && ('0' <= optarg[2] && optarg[2] <= '9'))
                {
                    version = (((optarg[0] - '0') * 10) + (optarg[2] - '0')) * 10;
                    printf("-v %c.%c\n", optarg[0], optarg[2]);
                }
                else {
                    printf("Incorrect version number: %s", optarg);
                    return -1;                              /* error return */
                }
                break;
            }
            case 'o': {                                      /* file output */
                strncpy(outFileName, optarg, sizeof(outFileName));
                printf("-o %s\n", outFileName);
                break;
            }
            case 's': {                       /* save binary data to a file */
                strncpy(savFileName, optarg, sizeof(savFileName));
                printf("-s %s\n", savFileName);
                break;
            }
            case 'm': {                        /* Matlab/Octave file output */
                strncpy(matFileName, optarg, sizeof(matFileName));
                printf("-m %s\n", matFileName);
                break;
            }
            case 'g': {                               /* MscGen file output */
                strncpy(mscFileName, optarg, sizeof(mscFileName));
                printf("-g %s\n", mscFileName);
                break;
            }
            case 'c': {                                         /* COM port */
                if ((link != NO_LINK) && (link != SERIAL_LINK)) {
                    printf("The -c option is incompatible with -p/-f\n");
                    return -1;                                   /* failure */
                }
                strncpy(comPort, optarg, sizeof(comPort));
                printf("-c %s\n", comPort);
                link = SERIAL_LINK;
                break;
            }
            case 'b': {                                        /* baud rate */
                if ((link != NO_LINK) && (link != SERIAL_LINK)) {
                    printf("The -b option is incompatible with -p/-f\n");
                    return -1;                                   /* failure */
                }
                if (sscanf(optarg, "%d", &baudRate) != 1) {
                    printf("incorrect baud rate: %s\n", optarg);
                    return -1;                                   /* failure */
                }
                printf("-b %d\n", baudRate);
                link = SERIAL_LINK;
                break;
            }
            case 'f': {                                       /* File input */
                if (link != NO_LINK) {
                    printf("The -f option is incompatible with -c/-b/-p\n");
                    return -1;                                   /* failure */
                }
                strncpy(inpFileName, optarg, sizeof(inpFileName));
                printf("-f %s\n", inpFileName);
                link = FILE_LINK;
                break;
            }
            case 't': {                                     /* TCP/IP input */
                if ((link != NO_LINK) && (link != TCP_LINK)) {
                    printf("The -t option is incompatible with -c/-b/-f\n");
                    return -1;
                }
                printf("-t\n");
                link = TCP_LINK;
                break;
            }
            case 'p': {                                      /* TCP/IP port */
                if ((link != NO_LINK) && (link != TCP_LINK)) {
                    printf("The -p option is incompatible with -c/-b/-f\n");
                    return -1;
                }
                tcpPort = (int)strtoul(optarg, NULL, 10);
                printf("-p %d\n", tcpPort);
                link = TCP_LINK;
                break;
            }
            case 'T': {                                   /* timestamp size */
                tstampSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'F': {                            /* function pointer size */
                funPtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'O': {                              /* object pointer size */
                objPtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'S': {                                      /* signal size */
                sigSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'E': {                                       /* event size */
                evtSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'Q': {                               /* Queue counter size */
                queueCtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'P': {                         /* Memory-pool counter size */
                poolCtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'B': {                       /* Memory-pool blocksize size */
                poolBlkSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'C': {                          /* Time event counter size */
                tevtCtrSize = (uint8_t)strtoul(optarg, 0, 10);
                break;
            }
            case 'h':                                               /* help */
            default: {                                    /* unknown option */
                printf("%s", help);
                return -1;                                  /* error return */
            }
        }
    }
    if (argc != optind) {
        printf("%s", help);
        return -1;
    }

    /* configure the Quantum Spy... */
    if (outFileName[0] != '\0') {
        l_outFile = fopen(outFileName, "w");
        if (l_outFile != (void *)0) {
            time_t now = time(NULL);
            fprintf(l_outFile, "QSPY host application %s\n"
                    "Copyright (c) Quantum Leaps, state-machine.com\n"
                    "%s\n", QSPY_VER, ctime(&now));
        }
    }

    if (savFileName[0] != '\0') {
        savFile = fopen(savFileName, "wb");      /* open for writing binary */
    }
    if (matFileName[0] != '\0') {
        matFile = fopen(matFileName, "w");
    }
    if (mscFileName[0] != '\0') {
        mscFile = fopen(mscFileName, "w");
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
                matFile,
                mscFile,
                (QSPY_CustParseFun)0);

    /* process the QS data from the selected intput... */
    switch (link) {
        case NO_LINK:                         /* intentionally fall through */
        case SERIAL_LINK: {        /* input trace data from the serial port */
            if (!HAL_comOpen(comPort, baudRate)) {
                return -1;
            }
            else {
                printf("\nSerial port %s opened, hit any key to quit...\n\n",
                       comPort);
            }
            while ((n = HAL_comRead(buf, sizeof(buf))) != -1) {
                if (n > 0) {
                    if (savFile != (FILE *)0) {
                        fwrite(buf, 1, n, savFile);
                    }
                    QSPY_parse(buf, n);
                }
            }
            HAL_comClose();
            break;
        }
        case FILE_LINK: {                   /* input trace data from a file */
            FILE *f = fopen(inpFileName, "rb");  /* open for reading binary */
            if (f == (FILE *)0) {
                printf("file %s not found\n", inpFileName);
                return -1;
            }
            do {
                n = fread(buf, 1, (int)sizeof(buf), f);
                if (savFile != (FILE *)0) {
                    fwrite(buf, 1, n, savFile);
                }
                QSPY_parse(buf, n);
            } while (n == sizeof(buf));

            fclose(f);
            break;
        }
        case TCP_LINK: {              /* input trace data from the TCP port */
            if (!HAL_tcpOpen(tcpPort)) {
                return -1;
            }
            else {
                printf("\nTCP/IP port %d opened, "
                       "hit any key to quit...\n"
                       "(the target must be stopped)\n",
                       tcpPort);
            }
            while ((n = HAL_tcpRead(buf, sizeof(buf))) != -1) {
                if (n > 0) {
                    if (savFile != (FILE *)0) {
                        fwrite(buf, 1, n, savFile);
                    }
                    QSPY_parse(buf, n);
                }
            }
            HAL_tcpClose();
            break;
        }
    }

    if (savFile != (FILE *)0) {
        fclose(savFile);
    }
    if (l_outFile != (FILE *)0) {
        fclose(l_outFile);
    }
    QSPY_stop();                         /* update and close all open files */

    printf("\nDone.\n");
    return 0;                                                    /* success */
}
/*..........................................................................*/
void QSPY_onPrintLn(void) {
    if (!l_quiet) {
        fputs(QSPY_line, stdout);
        fputc('\n', stdout);
    }
    if (l_outFile != (FILE *)0) {
        fputs(QSPY_line, l_outFile);
        fputc('\n', l_outFile);
    }
}
