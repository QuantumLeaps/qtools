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
* @date Last updated on: 2022-01-25
* @version Last updated for version: 7.0.0
*
* @file
* @brief QSPY PAL implementation for POSIX
*/
#include <stdlib.h>  /* for system() */
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#include "safe_std.h" /* "safe" <stdio.h> and <string.h> facilities */
#include "qspy.h"     /* QSPY data parser */
#include "be.h"       /* Back-End interface */
#include "pal.h"      /* Platform Abstraction Layer */

/*..........................................................................*/
PAL_VtblType PAL_vtbl;   /* global PAL virtual table */

/* specific implementations of the PAL "virutal functions" .................*/
static QSPYEvtType ser_getEvt(unsigned char *buf, uint32_t *pBytes);
static QSpyStatus  ser_send2Target(unsigned char *buf, uint32_t nBytes);
static void ser_cleanup(void);

static QSPYEvtType tcp_getEvt(unsigned char *buf, uint32_t *pBytes);
static QSpyStatus  tcp_send2Target(unsigned char *buf, uint32_t nBytes);
static void tcp_cleanup(void);

static QSPYEvtType file_getEvt(unsigned char *buf, uint32_t *pBytes);
static QSpyStatus  file_send2Target(unsigned char *buf, uint32_t nBytes);
static void file_cleanup(void);

/*..........................................................................*/
enum PAL_Constants { /* local constants... */
    INVALID_SOCKET = -1,
    SOCKET_ERROR   = -1,
    FE_DETACHED    = 0,   /* Front-End detached */
    PAL_TOUT_MS    = 10,  /* determines how long to wait for an event [ms] */
};

/* fron-end address */
typedef union {
    uint64_t data[2];
    struct sockaddr addr;
} fe_addr;

static bool l_kbd_inp = false;

static int l_serFD      = 0;  /* Serial port file descriptor */
static int l_serverSock = INVALID_SOCKET;
static int l_clientSock = INVALID_SOCKET;
static int l_beSock     = INVALID_SOCKET;

static fe_addr   l_feAddr;
static socklen_t l_feAddrSize = FE_DETACHED;

static FILE *l_file = (FILE *)0;

static struct termios l_termios_saved; /* saved terminal attributes */
static fd_set l_readSet; /* descriptor set for reading all input sources */
static int l_maxFd;      /* maximum file descriptor for select() */

/*==========================================================================*/
/* Keyboard input */
static void sigExitHandler(int dummy) {
    (void)dummy; /* unused parameter */
    QSPY_cleanup();
    exit(0);
}

QSpyStatus PAL_openKbd(bool kbd_inp, bool color) {
    struct sigaction sig_act;
    memset(&sig_act, 0, sizeof(sig_act));

    (void)color; /* unused parameter */

    /* install the SIGINT (Ctrl-C) signal handler */
    sig_act.sa_handler = &sigExitHandler;
    sigaction(SIGINT, &sig_act, NULL);

    /* install the SIGTERM (kill) signal handler */
    sig_act.sa_handler = &sigExitHandler;
    sig_act.sa_flags   = SA_SIGINFO;
    sigaction(SIGTERM, &sig_act, NULL);

    if (kbd_inp) {
        struct termios t;

        /* modify the terminal attributes... */
        /* get the original terminal settings */
        if (tcgetattr(0, &l_termios_saved) == -1) {
            SNPRINTF_LINE("    <CONS> ERROR    %s",
                          "getting terminal attributes");
            QSPY_printError();
            return QSPY_ERROR;
        }

        t = l_termios_saved;
        t.c_lflag &= ~(ICANON | ECHO); /* disable canonical mode and echo */
        if (tcsetattr(0, TCSANOW, &t) == -1) {
            SNPRINTF_LINE("    <CONS> ERROR    %s",
                          "setting terminal attributes");
            QSPY_printError();
            return QSPY_ERROR;
        }

        l_kbd_inp = true;
    }
    return QSPY_SUCCESS;
}
/*..........................................................................*/
void PAL_closeKbd(void) {
    if (l_kbd_inp) {
        /* restore the saved terminal settings */
        tcsetattr(0, TCSANOW, &l_termios_saved);
        l_kbd_inp = false;
    }
}

/*==========================================================================*/
/* POSIX serial communication with the Target */
QSpyStatus PAL_openTargetSer(char const *comName, int baudRate) {
    struct termios t;
    speed_t spd;

    /* setup the PAL virtual table for the Serial communication... */
    PAL_vtbl.getEvt      = &ser_getEvt;
    PAL_vtbl.send2Target = &ser_send2Target;
    PAL_vtbl.cleanup     = &ser_cleanup;

    l_serFD = open(comName, O_RDWR | O_NOCTTY | O_NONBLOCK);/* R/W,no-block */
    if (l_serFD == -1) {
        SNPRINTF_LINE("   <COMMS> ERROR    "
                      "Opening serial port Port=%s,Baud=%d",
                      comName, baudRate);
        QSPY_printError();
        return QSPY_ERROR; /* open failed */
    }

    if (tcgetattr(l_serFD, &t) == -1) {
        SNPRINTF_LINE("   <COMMS> ERROR    "
                      "cannot get serial attributes errno=%d",
                      errno);
        QSPY_printError();
        return QSPY_ERROR; /* getting attributes failed */
    }
    t.c_cc[VMIN]  = 0;
    t.c_cc[VTIME] = 1;

    t.c_iflag = 0;
    t.c_iflag &= ~(BRKINT | IGNPAR | PARMRK | INPCK |
                   ISTRIP | INLCR | IGNCR | ICRNL | IXON | IXOFF);
    t.c_iflag |= IGNBRK;

    t.c_oflag = 0;
    t.c_oflag &= ~OPOST;

    t.c_lflag = 0;
    t.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL |
                   ICANON | ISIG | NOFLSH | TOSTOP);

    t.c_cflag = 0;
    t.c_cflag &= ~(CSIZE | HUPCL);
    t.c_cflag |= (CLOCAL | CREAD);

    spd = B115200; /* the default speed of the Serial port */
    switch (baudRate) {
        case    200: spd =    B200; break;
        case    300: spd =    B300; break;
        case    600: spd =    B600; break;
        case   1200: spd =   B1200; break;
        case   1800: spd =   B1800; break;
        case   2400: spd =   B2400; break;
        case   4800: spd =   B4800; break;
        case   9600: spd =   B9600; break;
        case  19200: spd =  B19200; break;
        case  38400: spd =  B38400; break;
        case  57600: spd =  B57600; break;
        case 115200: spd = B115200; break;

        /* high baud rates (might not be supported)... */
#ifdef B230400
        case 230400: spd = B230400; break;
#endif
#ifdef B460800
        case 460800: spd = B460800; break;
#endif
#ifdef B500000
        case 500000: spd = B500000; break;
#endif
#ifdef B576000
        case 576000: spd = B576000; break;
#endif
#ifdef B921600
        case 921600: spd = B921600; break;
#endif
#ifdef B1000000
        case 1000000: spd = B1000000; break;
#endif
#ifdef B1152000
        case 1152000: spd = B1152000; break;
#endif
#ifdef B1500000
        case 1500000: spd = B1500000; break;
#endif
#ifdef B2000000
        case 2000000: spd = B2000000; break;
#endif
#ifdef B2500000
        case 2500000: spd = B2500000; break;
#endif
#ifdef B3000000
        case 3000000: spd = B3000000; break;
#endif
#ifdef B3500000
        case 3500000: spd = B3500000; break;
#endif
#ifdef B4000000
        case 4000000: spd = B4000000; break;
#endif

        default:
            SNPRINTF_LINE("   <COMMS> ERROR    Unsupported rate Baud=%d",
                          baudRate);
            QSPY_printError();
            return QSPY_ERROR;
    }

    t.c_cflag &= ~(PARENB | PARODD); /* no parity */
    t.c_cflag |= (CS8);              /* 8 bits in a byte */
    t.c_cflag &= ~(CSTOPB);          /* 1 stop bit */

    if (cfsetispeed(&t, spd) == -1) {
        SNPRINTF_LINE("   <COMMS> ERROR    %s",
                      "setting input speed failed");
        QSPY_printError();
        return QSPY_ERROR;
    }
    if (cfsetospeed(&t, spd) == -1) {
        SNPRINTF_LINE("   <COMMS> ERROR    %s",
                      "setting output speed failed");
        QSPY_printError();
        return QSPY_ERROR;
    }
    if (tcflush(l_serFD, TCIFLUSH) == -1) {
        SNPRINTF_LINE("   <COMMS> ERROR    %s",
                      "flushing serial port failed");
        QSPY_printError();
        return QSPY_ERROR;
    }
    if (tcsetattr(l_serFD, TCSANOW, &t) == -1) {
        SNPRINTF_LINE("   <COMMS> ERROR    %s",
                      "seting serial attributes failed");
        QSPY_printError();
        return QSPY_ERROR;
    }

    PAL_updateReadySet(l_serFD); /* Serial port to be checked in select() */

    return QSPY_SUCCESS;
}
/*..........................................................................*/
static QSPYEvtType ser_getEvt(unsigned char *buf, uint32_t *pBytes) {
    QSPYEvtType evtType;
    fd_set readSet = l_readSet;

    /* block indefinitely until any input source has input */
    int nrec = select(l_maxFd, &readSet, 0, 0, NULL);

    if (nrec == 0) {
        return QSPY_NO_EVT;
    }
    else if (nrec < 0) {
        SNPRINTF_LINE("   <COMMS> ERROR    select() errno=%d", errno);
        QSPY_printError();
        return QSPY_ERROR_EVT;
    }

    /* any input available from the keyboard? */
    if (l_kbd_inp && FD_ISSET(0, &readSet)) {
        evtType = PAL_receiveKbd(buf, pBytes);
        if (evtType != QSPY_NO_EVT) {
            return evtType;
        }
    }

    /* any input available from the Back-End socket? */
    if ((l_beSock != INVALID_SOCKET) && FD_ISSET(l_beSock, &readSet)) {
        evtType = PAL_receiveBe(buf, pBytes);
        if (evtType != QSPY_NO_EVT) {
            return evtType;
        }
    }

    /* any input available from the Serial port? */
    if (FD_ISSET(l_serFD, &readSet)) {
        uint32_t nBytes = read(l_serFD, buf, *pBytes);
        if (nBytes > 0) {
            *pBytes = (uint32_t)nBytes;
            return QSPY_TARGET_INPUT_EVT;
        }
    }

    return QSPY_NO_EVT;
}
/*..........................................................................*/
static QSpyStatus ser_send2Target(unsigned char *buf, uint32_t nBytes) {
    uint32_t nBytesWritten = write(l_serFD, buf, nBytes);
    if (nBytesWritten == (uint32_t)nBytes) {
        return QSPY_SUCCESS;
    }
    else {
        SNPRINTF_LINE("   <COMMS> ERROR    writing serial port errno=%d",
                      errno);
        QSPY_printError();
        return QSPY_ERROR;
    }
}
/*..........................................................................*/
static void ser_cleanup(void) {
    if (l_serFD != 0) {
        close(l_serFD); /* close the serial port */
        l_serFD = 0;
    }
}

/*==========================================================================*/
/* POSIX TCP/IP communication with the Target */

struct sockaddr_in l_clientAddr;

/*..........................................................................*/
QSpyStatus PAL_openTargetTcp(int portNum) {
    struct sockaddr_in local;

    /* setup the PAL virtual table for the TCP/IP Target connection... */
    PAL_vtbl.getEvt      = &tcp_getEvt;
    PAL_vtbl.send2Target = &tcp_send2Target;
    PAL_vtbl.cleanup     = &tcp_cleanup;

    /* create TCP socket */
    l_serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (l_serverSock == INVALID_SOCKET) {
        SNPRINTF_LINE("   <COMMS> ERROR    server socket open errno=%d",
                      errno);
        QSPY_printError();
        return QSPY_ERROR;
    }

    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons((unsigned short)portNum);

    /* bind() associates a local address and port combination with the
    * socket just created. This is most useful when the application is a
    * server that has a well-known port that clients know about in advance.
    */
    if (bind(l_serverSock, (struct sockaddr *)&local, sizeof(local))
        == SOCKET_ERROR)
    {
        SNPRINTF_LINE("   <COMMS> ERROR    socket binding errno=%d", errno);
        QSPY_printError();
        return QSPY_ERROR;
    }

    if (listen(l_serverSock, 1) == SOCKET_ERROR) {
        SNPRINTF_LINE("   <COMMS> ERROR    socket listen errno=%d", errno);
        QSPY_printError();
        return QSPY_ERROR;
    }

    PAL_updateReadySet(l_serverSock); /* to be checked in select() */

    return QSPY_SUCCESS;
}
/*..........................................................................*/
static void tcp_cleanup(void) {
    if (l_serverSock != INVALID_SOCKET) {
        close(l_serverSock);
    }
}
/*..........................................................................*/
static QSPYEvtType tcp_getEvt(unsigned char *buf, uint32_t *pBytes) {
    QSPYEvtType evtType;
    fd_set readSet = l_readSet;

    /* block indefinitely until any input source has input */
    int nrec = select(l_maxFd, &readSet, 0, 0, NULL);

    if (nrec == 0) {
        return QSPY_NO_EVT;
    }
    else if (nrec < 0) {
        SNPRINTF_LINE("   <COMMS> ERROR    select() errno=%d", errno);
        QSPY_printError();
        return QSPY_ERROR_EVT;
    }

    /* any input available from the keyboard? */
    if (l_kbd_inp && FD_ISSET(0, &readSet)) {
        evtType = PAL_receiveKbd(buf, pBytes);
        if (evtType != QSPY_NO_EVT) {
            return evtType;
        }
    }

    /* any input available from the Back-End socket? */
    if ((l_beSock != INVALID_SOCKET) && FD_ISSET(l_beSock, &readSet)) {
        evtType = PAL_receiveBe(buf, pBytes);
        if (evtType != QSPY_NO_EVT) {
            return evtType;
        }
    }

    /* still waiting for the client? */
    if (l_clientSock == INVALID_SOCKET) {
        if (FD_ISSET(l_serverSock, &readSet)) {
           socklen_t clientAddrLen = (socklen_t)sizeof(l_clientAddr);
           l_clientSock = accept(l_serverSock,
                                  (struct sockaddr *)&l_clientAddr,
                                  &clientAddrLen);
            if (l_clientSock == INVALID_SOCKET) {
                SNPRINTF_LINE("   <COMMS> ERROR    socket accept errno=%d",
                              errno);
                QSPY_printError();
                return QSPY_ERROR_EVT;
            }

            QSPY_reset();   /* reset the QSPY parser to start over cleanly */
            QSPY_txReset(); /* reset the QSPY transmitter */

            SNPRINTF_LINE("   <COMMS> TCP-IP   Connected to Host=%s,Port=%d",
                   inet_ntoa(l_clientAddr.sin_addr),
                   (int)ntohs(l_clientAddr.sin_port));
            QSPY_printInfo();

            /* re-evaluate the ready set and max FD for select() */
            PAL_updateReadySet(l_clientSock);
        }
    }
    else {
        if (FD_ISSET(l_clientSock, &readSet)) {
            nrec = recv(l_clientSock, (char *)buf, *pBytes, 0);

            if (nrec <= 0) { /* the client hang up */
                SNPRINTF_LINE("   <COMMS> TCP-IP   Disconn from "
                              "Host=%s,Port=%d",
                              inet_ntoa(l_clientAddr.sin_addr),
                              (int)ntohs(l_clientAddr.sin_port));
                QSPY_printInfo();

                /* go back to waiting for a client */
                close(l_clientSock);
                l_clientSock = INVALID_SOCKET;

                /* re-evaluate the ready set and max FD for select() */
                PAL_updateReadySet(l_serverSock);
            }
            else {
                *pBytes = (uint32_t)nrec;
                return QSPY_TARGET_INPUT_EVT;
            }
        }
    }

    return QSPY_NO_EVT;
}
/*..........................................................................*/
static QSpyStatus tcp_send2Target(unsigned char *buf, uint32_t nBytes) {
    if (l_clientSock == INVALID_SOCKET) {
        return QSPY_ERROR;
    }
    if (send(l_clientSock, buf, nBytes, 0) == SOCKET_ERROR) {
        SNPRINTF_LINE("   <COMMS> ERROR    Writing to TCP socket errno=%d",
                      errno);
        QSPY_printError();
        return QSPY_ERROR;
    }
    return QSPY_SUCCESS;
}

/*==========================================================================*/
/* File communication with the "Target" */
QSpyStatus PAL_openTargetFile(char const *fName) {
    int fd;

    /* setup the PAL virtual table for the File connection... */
    PAL_vtbl.getEvt      = &file_getEvt;
    PAL_vtbl.send2Target = &file_send2Target;
    PAL_vtbl.cleanup     = &file_cleanup;

    FOPEN_S(l_file, fName, "rb"); /* open for reading binary */
    if (l_file == (FILE *)0) {
        SNPRINTF_LINE("   <COMMS> ERROR    Cannot find File=%s", fName);
        QSPY_printError();
        return QSPY_ERROR;
    }

    QSPY_reset();   /* reset the QSPY parser to start over cleanly */
    QSPY_txReset(); /* reset the QSPY transmitter */

    fd = fileno(l_file); /* FILE* to file-descriptor */
    PAL_updateReadySet(fd);  /* fd to be checked in select() */

    SNPRINTF_LINE("   <COMMS> File     Opened File=%s", fName);
    QSPY_printInfo();

    return QSPY_SUCCESS;
}
/*..........................................................................*/
static QSPYEvtType file_getEvt(unsigned char *buf, uint32_t *pBytes) {
    QSPYEvtType evtType;
    uint32_t nBytes;
    fd_set readSet = l_readSet;

    /* block indefinitely until any input source has input */
    int nrec = select(l_maxFd, &readSet, 0, 0, NULL);

    if (nrec == 0) {
        return QSPY_NO_EVT;
    }
    else if (nrec < 0) {
        SNPRINTF_LINE("   <COMMS> ERROR    select() errno=%d", errno);
        QSPY_printError();
        return QSPY_ERROR_EVT;
    }

    /* any input available from the keyboard? */
    if (l_kbd_inp && FD_ISSET(0, &readSet)) {
        evtType = PAL_receiveKbd(buf, pBytes);
        if (evtType != QSPY_NO_EVT) {
            return evtType;
        }
    }

    /* any input available from the Back-End socket? */
    if ((l_beSock != INVALID_SOCKET) && FD_ISSET(l_beSock, &readSet)) {
        evtType = PAL_receiveBe(buf, pBytes);
        if (evtType != QSPY_NO_EVT) {
            return evtType;
        }
    }

    /* try to receive data from the File... */
    nBytes = FREAD_S(buf, *pBytes, 1U, *pBytes, l_file);
    if (nBytes > 0) {
        *pBytes = nBytes;
        return QSPY_TARGET_INPUT_EVT;
    }

    /* no more input available from the file, QSPY is done */
    return QSPY_DONE_EVT;
}
/*..........................................................................*/
static QSpyStatus file_send2Target(unsigned char *buf, uint32_t nBytes) {
    (void)buf;
    (void)nBytes;
    return QSPY_ERROR;
}
/*..........................................................................*/
static void file_cleanup(void) {
    if (l_file != (FILE *)0) {
        fclose(l_file);
        l_file = (FILE *)0;
    }
}


/*==========================================================================*/
/* Front-End interface  */
QSpyStatus PAL_openBE(int portNum) {
    struct sockaddr_in local;
    int flags;

    l_beSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); /* UDP socket */
    if (l_beSock == INVALID_SOCKET){
        SNPRINTF_LINE("   <F-END> ERROR    UDP socket create errno=%d",
                      errno);
        QSPY_printError();
        return QSPY_ERROR;
    }

    /* bind the socket...
    * bind() associates a local address and port combination with the
    * socket just created. This is most useful when the application is a
    * server that has a well-known port that clients know about in advance.
    */
    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons((unsigned short)portNum);
    if (bind(l_beSock, (struct sockaddr *)&local, sizeof(local))
        == SOCKET_ERROR)
    {
        SNPRINTF_LINE("   <F-END> ERROR    UDP socket binding errno=%d",
                      errno);
        QSPY_printError();
        return QSPY_ERROR;
    }

    /* put the socket into NON-BLOCKING mode... */
    flags = fcntl(l_beSock, F_GETFL, 0);
    if (flags == SOCKET_ERROR) {
        SNPRINTF_LINE("   <F-END> ERROR    UDP socket fcntl() get errno=%d",
                      errno);
        QSPY_printError();
        return QSPY_ERROR;
    }
    flags |= O_NONBLOCK;
    if (fcntl(l_beSock, F_SETFL, flags) != 0) {
        SNPRINTF_LINE("   <F-END> ERROR    "
                      "UDP socket fcntl() set errno=%d", errno);
        QSPY_printError();
        return QSPY_ERROR;
    }

    BE_onStartup();  /* Back-End startup callback */

    /* NOTE:
    * The Back-End socket l_beSock needs to be checked in select() just
    * like all other sources of input. However, the l_beSock socket is
    * added automatically in the PAL_updateReadySet() function *later* in
    * the initialization phase. This assumes that PAL_openBE() is called
    * always *before* opening the specific Target link (see configure() in
    * main.c).
    */

    return QSPY_SUCCESS;
}
/*..........................................................................*/
void PAL_closeBE(void) {
    if (l_beSock != INVALID_SOCKET) {
        if (l_feAddrSize != FE_DETACHED) { /* front-end attached? */
            fd_set writeSet;
            struct timeval delay;

            /* Back-End cleanup callback (send detach packet)... */
            BE_onCleanup();

            /* block until the packet comes out... */
            FD_ZERO(&writeSet);
            FD_SET(l_beSock, &writeSet);
            delay.tv_sec  = 2U; /* delay for up to 2 seconds */
            delay.tv_usec = 0U;
            if (select(l_beSock + 1, (fd_set *)0,
                       &writeSet, (fd_set *)0, &delay)
                == SOCKET_ERROR)
            {
                SNPRINTF_LINE("   <F-END> ERROR    "
                             "UDP socket select errno=%d",
                        errno);
                QSPY_printError();
            }
        }

        close(l_beSock);
        l_beSock = INVALID_SOCKET;
    }
}
/*..........................................................................*/
void PAL_send2FE(unsigned char const *buf, uint32_t nBytes) {
    if (l_feAddrSize != FE_DETACHED) { /* front-end attached? */
        if (sendto(l_beSock, (char *)buf, (int)nBytes, 0,
                   &l_feAddr.addr, l_feAddrSize) == SOCKET_ERROR)
        {
            PAL_detachFE(); /* detach the Front-End */

            SNPRINTF_LINE("   <F-END> ERROR    UDP socket failed errno=%d",
                          errno);
            QSPY_printError();
        }
    }
}
/*..........................................................................*/
void PAL_detachFE(void) {
    l_feAddrSize = FE_DETACHED;
}

/*..........................................................................*/
void PAL_clearScreen(void) {
    int status = system("clear");
    if (status < 0) {
        SNPRINTF_LINE("    <CONS> ERROR    %s",
                      "clearing the screen failed");
        QSPY_printError();
    }
}

/*--------------------------------------------------------------------------*/
QSPYEvtType PAL_receiveBe(unsigned char *buf, uint32_t *pBytes) {
    fe_addr feAddr;
    socklen_t feAddrSize;
    uint32_t nBytes;

    if (l_beSock == INVALID_SOCKET) { /* Back-End socket not initialized? */
        return QSPY_NO_EVT;
    }

    /* receive a packet from the Back-End socket */
    feAddrSize = sizeof(feAddr);
    nBytes = recvfrom(l_beSock, buf, *pBytes, 0,
                      &feAddr.addr, &feAddrSize);

    if (nBytes == 0)  { /* socket error */
        PAL_detachFE(); /* detach from the Front-End */
        SNPRINTF_LINE("   <F-END> ERROR    "
            "UDP socket recvfrom() errno=%d", errno);
        QSPY_printError();
        return QSPY_ERROR_EVT;
    }
    else if (l_feAddrSize == 0) { /* not attached yet? */
        memcpy(&l_feAddr, &feAddr, feAddrSize);
        l_feAddrSize = feAddrSize; /* attach connection */
        *pBytes = nBytes;
        return QSPY_FE_INPUT_EVT;
    }
    else { /* already attached */
        /* is this from the attached front-end address? */
        if ((feAddrSize == l_feAddrSize)
            && (feAddr.data[1] == l_feAddr.data[1])
            && (feAddr.data[0] == l_feAddr.data[0]))
        {
            *pBytes = nBytes;
            return QSPY_FE_INPUT_EVT;
        }
        else {
            SNPRINTF_LINE("   <F-END> WARN     %s",
                          "UDP socket in use");
            QSPY_printError();
            /* this packet is from a DIFFERENT front-end -- ignore it */
        }
    }
    return QSPY_NO_EVT;
}

/*..........................................................................*/
QSPYEvtType PAL_receiveKbd(unsigned char *buf, uint32_t *pBytes) {
    *pBytes = read(0, buf, 1); /* the key pressed */
    if (*pBytes > 0) {
        return QSPY_KEYBOARD_EVT;
    }
    return QSPY_NO_EVT;
}
/*..........................................................................*/
void PAL_updateReadySet(int targetConn) {
    FD_ZERO(&l_readSet);
    FD_SET(0, &l_readSet); /* terminal to be checked in select */
    l_maxFd = 1;
    FD_SET(targetConn, &l_readSet); /* check in select */
    if (l_maxFd < targetConn + 1) {
        l_maxFd = targetConn + 1;
    }
    if (l_beSock != INVALID_SOCKET) {
        FD_SET(l_beSock, &l_readSet); /* check in select */
        if (l_maxFd < l_beSock + 1) {
            l_maxFd = l_beSock + 1;
        }
    }
}

/*..........................................................................*/
/* simplified strncpy_s() implementation "good enough" for the intended use */
int strncpy_s(char* strDest, uint32_t numberOfElements,
    const char* strSource, uint32_t count)
{
    uint32_t n;
    for (n = (numberOfElements < count) ? numberOfElements : count;
         n > 0;
         --n, ++strSource, ++strDest)
    {
        if (*strSource != '\0') {
            *strDest = *strSource;
        }
        else {
            *strDest = '\0';
            return 0; /* destination fits the source */
        }
    }
    strDest[numberOfElements - 1] = '\0';
    return -1; /* destination holds truncated source */
}
