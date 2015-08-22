/*****************************************************************************
* Product: QSPY -- PAL implementation for POSIX
* Last updated for version 5.5.0
* Last updated on  2015-08-18
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
* http:://www.state-machine.com
* mailto:info@state-machine.com
*****************************************************************************/
#include <stddef.h>  /* for size_t */
#include <stdlib.h>  /* for system() */
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>

#include "qspy.h"    /* QSPY data parser */
#include "be.h"      /* Back-End interface */
#include "pal.h"     /* Platform Abstraction Layer */

/*..........................................................................*/
PAL_VtblType PAL_vtbl;   /* global PAL virtual table */

/* specific implementations of the PAL "virutal functions" .................*/
static QSPYEvtType ser_getEvt(unsigned char *buf, size_t *pBytes);
static QSpyStatus  ser_send2Target(unsigned char *buf, size_t nBytes);
static void ser_cleanup(void);

static QSPYEvtType tcp_getEvt(unsigned char *buf, size_t *pBytes);
static QSpyStatus  tcp_send2Target(unsigned char *buf, size_t nBytes);
static void tcp_cleanup(void);

static QSPYEvtType file_getEvt(unsigned char *buf, size_t *pBytes);
static QSpyStatus  file_send2Target(unsigned char *buf, size_t nBytes);
static void file_cleanup(void);

/* helper functions ........................................................*/
static QSPYEvtType be_receive (fd_set const *pReadSet,
                               unsigned char *buf, size_t *pBytes);

static QSpyStatus kbd_open(void);
static void kbd_close(void);
static QSPYEvtType kbd_receive(fd_set const *pReadSet,
                               unsigned char *buf, size_t *pBytes);
static void updateReadySet(int targetConn);

/*..........................................................................*/
#define INVALID_SOCKET -1

static int l_serFD      = 0;  /* Serial port file descriptor */
static int l_serverSock = INVALID_SOCKET;
static int l_clientSock = INVALID_SOCKET;
static int l_beSock     = INVALID_SOCKET;


static struct termios l_termios_saved; /* saved terminal attributes */
static fd_set l_readSet; /* descriptor set for reading all input sources */
static int l_maxFd;      /* maximum file descriptor for select() */

static struct sockaddr l_beReturnAddr;
static socklen_t       l_beReturnAddrSize;

static FILE *l_file = (FILE *)0;

/* PAL timeout determines how long to wait for an event */
#define PAL_TOUT_MS 100

/*==========================================================================*/
/* Win32 serial communication with the Target */
QSpyStatus PAL_openTargetSer(char const *comName, int baudRate) {
    struct termios t;
    speed_t spd;

    /* setup the PAL virtual table for the Serial communication... */
    PAL_vtbl.getEvt      = &ser_getEvt;
    PAL_vtbl.send2Target = &ser_send2Target;
    PAL_vtbl.cleanup     = &ser_cleanup;

    /* start with initializing the keyboard (terminal) */
    if (kbd_open() != QSPY_SUCCESS) {
        return QSPY_ERROR;
    }

    l_serFD = open(comName, O_RDWR | O_NOCTTY | O_NONBLOCK);/* R/W,no-block */
    if (l_serFD == -1) {
        fprintf(stderr, "*** PAL: cannot open serial port %s\n", comName);
        return QSPY_ERROR; /* open failed */
    }

    if (tcgetattr(l_serFD, &t) == -1) {
        fprintf(stderr, "*** PAL: cannot get serial attributes; errno=%d\n",
                        errno);
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
        default:
            fprintf(stderr, "*** PAL: Unsupported baud rate: %d\n", baudRate);
            return QSPY_ERROR;
    }

    t.c_cflag &= ~(PARENB | PARODD); /* no parity */
    t.c_cflag |= (CS8);              /* 8 bits in a byte */
    t.c_cflag &= ~(CSTOPB);          /* 1 stop bit */

    if (cfsetispeed(&t, spd) == -1) {
        fprintf(stderr, "*** PAL: setting input speed failed\n");
        return QSPY_ERROR;
    }
    if (cfsetospeed(&t, spd) == -1) {
        fprintf(stderr, "*** PAL: setting output speed failed\n");
        return QSPY_ERROR;
    }
    if (tcflush(l_serFD, TCIFLUSH) == -1) {
        fprintf(stderr, "*** PAL: flushing the port failed\n");
        return QSPY_ERROR;
    }
    if (tcsetattr(l_serFD, TCSANOW, &t) == -1) {
        fprintf(stderr, "*** PAL: seting serial attributes failed\n");
        return QSPY_ERROR;
    }

    updateReadySet(l_serFD); /* Serial port to be checked in select() */

    return QSPY_SUCCESS;
}
/*..........................................................................*/
static QSPYEvtType ser_getEvt(unsigned char *buf, size_t *pBytes) {
    QSPYEvtType evt;
    fd_set readSet = l_readSet;

    /* block indefinitely until any input source has intput */
    int nrec = select(l_maxFd, &readSet, 0, 0, NULL);

    if (nrec == 0) {
        return QSPY_NO_EVT;
    }
    else if (nrec < 0) {
        fprintf(stderr, "*** PAL: select() error: %d\n", errno);
        return QSPY_ERROR_EVT;
    }

    /* any intput available from the keyboard? */
    evt = kbd_receive(&readSet, buf, pBytes);
    if (evt != QSPY_NO_EVT) {
        return evt;
    }

    /* any intput available from the Back-End socket? */
    evt = be_receive(&readSet, buf, pBytes);
    if (evt != QSPY_NO_EVT) {
        return evt;
    }

    /* any input available from the Serial port? */
    if (FD_ISSET(l_serFD, &readSet)) {
        ssize_t nBytes = read(l_serFD, buf, *pBytes);
        if (nBytes > 0) {
            *pBytes = (size_t)nBytes;
            return QSPY_TARGET_INPUT_EVT;
        }
    }

    return QSPY_NO_EVT;
}
/*..........................................................................*/
static QSpyStatus ser_send2Target(unsigned char *buf, size_t nBytes) {
    ssize_t nBytesWritten = write(l_serFD, buf, nBytes);
    if (nBytesWritten == (ssize_t)nBytes) {
        return QSPY_SUCCESS;
    }
    else {
        fprintf(stderr, "*** PAL: Writing to serial port failed; errno=%d\n",
                errno);
        return QSPY_ERROR;
    }
}
/*..........................................................................*/
static void ser_cleanup(void) {
    kbd_close(); /* close the keyboard */

    if (l_serFD != 0) {
        close(l_serFD); /* close the serial port */
        l_serFD = 0;
    }
}


/*==========================================================================*/
/* POSIX TCP/IP communication with the Target */

/*..........................................................................*/
QSpyStatus PAL_openTargetTcp(int portNum) {
    struct sockaddr_in local;

    /* setup the PAL virtual table for the TCP/IP Target connection... */
    PAL_vtbl.getEvt      = &tcp_getEvt;
    PAL_vtbl.send2Target = &tcp_send2Target;
    PAL_vtbl.cleanup     = &tcp_cleanup;

    /* start with initializing the keyboard (terminal) */
    if (kbd_open() != QSPY_SUCCESS) {
        return QSPY_ERROR;
    }

    l_serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* TCP socket */
    if (l_serverSock == INVALID_SOCKET) {
        fprintf(stderr, "*** PAL: Server socket open error; errno=%d\n",
                errno);
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
    if (bind(l_serverSock, (struct sockaddr *)&local, sizeof(local)) == -1) {
        fprintf(stderr, "*** PAL: socket binding error; errno=%d\n", errno);
        return QSPY_ERROR;
    }

    if (listen(l_serverSock, 1) == -1) {
        fprintf(stderr, "*** PAL: socket listen failed; errno %d\n", errno);
        return QSPY_ERROR;
    }

    updateReadySet(l_serverSock); /* to be checked in select() */

    return QSPY_SUCCESS;
}
/*..........................................................................*/
static void tcp_cleanup(void) {
    kbd_close(); /* close the keyboard */

    if (l_serverSock != INVALID_SOCKET) {
        close(l_serverSock);
    }
}
/*..........................................................................*/
static QSPYEvtType tcp_getEvt(unsigned char *buf, size_t *pBytes) {
    QSPYEvtType evt;
    fd_set readSet = l_readSet;

    /* block indefinitely until any input source has intput */
    int nrec = select(l_maxFd, &readSet, 0, 0, NULL);

    if (nrec == 0) {
        return QSPY_NO_EVT;
    }
    else if (nrec < 0) {
        fprintf(stderr, "*** PAL: select() error: %d\n", errno);
        return QSPY_ERROR_EVT;
    }

    /* any intput available from the keyboard? */
    evt = kbd_receive(&readSet, buf, pBytes);
    if (evt != QSPY_NO_EVT) {
        return evt;
    }

    /* any intput available from the Back-End socket? */
    evt = be_receive(&readSet, buf, pBytes);
    if (evt != QSPY_NO_EVT) {
        return evt;
    }

    /* still waiting for the client? */
    if (l_clientSock == INVALID_SOCKET) {
        if (FD_ISSET(l_serverSock, &readSet)) {
            struct sockaddr_in fromAddr;
            socklen_t fromLen = (socklen_t)sizeof(fromAddr);
            l_clientSock = accept(l_serverSock,
                                  (struct sockaddr *)&fromAddr, &fromLen);
            if (l_clientSock == INVALID_SOCKET) {
                fprintf(stderr, "*** PAL: Socket accept failed; errno=%d\n",
                       errno);
                return QSPY_ERROR_EVT;
            }
            printf("*** PAL: Accepted connection from %s, port %d\n",
                   inet_ntoa(fromAddr.sin_addr),
                   (int)ntohs(fromAddr.sin_port));

            /* re-evaluate the ready set and max FD for select() */
            updateReadySet(l_clientSock);
        }
    }
    else {
        if (FD_ISSET(l_clientSock, &readSet)) {
            nrec = recv(l_clientSock, (char *)buf, *pBytes, 0);
            if (nrec == -1) {
                fprintf(stderr, "*** PAL: Client socket error; errno=%d\n",
                        errno);
            }
            if (nrec <= 0) { /* the client hang up */
                printf("Client hang up. Waiting for new client...\n");
                /* go back to waiting for a client */
                close(l_clientSock);
                l_clientSock = INVALID_SOCKET;  /* invalidate client socket */

                /* re-evaluate the ready set and max FD for select() */
                updateReadySet(l_serverSock);
            }
            else {
                *pBytes = (size_t)nrec;
                return QSPY_TARGET_INPUT_EVT;
            }
        }
    }

    return QSPY_NO_EVT;
}
/*..........................................................................*/
static QSpyStatus tcp_send2Target(unsigned char *buf, size_t nBytes) {
    if (l_clientSock != INVALID_SOCKET) {
        if (send(l_clientSock, (char *)buf, (int)nBytes, 0) == -1) {
            fprintf(stderr,
                    "*** PAL: Writing to TCP socket failed; errno=%d\n",
                    errno);
            return QSPY_ERROR;
        }
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

    /* start with initializing the keyboard (terminal) */
    if (kbd_open() != QSPY_SUCCESS) {
        return QSPY_ERROR;
    }

    FOPEN_S(l_file, fName, "rb"); /* open for reading binary */
    if (l_file != (FILE *)0) {
        printf("*** PAL: File %s opened, hit any key to quit...\n", fName);
    }
    else {
        fprintf(stderr, "*** PAL: file %s not found\n", fName);
        return QSPY_ERROR;
    }

    fd = fileno(l_file); /* FILE* to file-descriptor */
    updateReadySet(fd);  /* fd to be checked in select() */

    return QSPY_SUCCESS;
}
/*..........................................................................*/
static QSPYEvtType file_getEvt(unsigned char *buf, size_t *pBytes) {
    QSPYEvtType evt;
    size_t nBytes;
    fd_set readSet = l_readSet;

    /* block indefinitely until any input source has intput */
    int nrec = select(l_maxFd, &readSet, 0, 0, NULL);

    if (nrec == 0) {
        return QSPY_NO_EVT;
    }
    else if (nrec < 0) {
        fprintf(stderr, "*** PAL: select() error: %d\n", errno);
        return QSPY_ERROR_EVT;
    }

    /* any intput available from the keyboard? */
    evt = kbd_receive(&readSet, buf, pBytes);
    if (evt != QSPY_NO_EVT) {
        return evt;
    }

    /* any intput available from the Back-End socket? */
    evt = be_receive(&readSet, buf, pBytes);
    if (evt != QSPY_NO_EVT) {
        return evt;
    }

    /* any input available from the File? */
    nBytes = fread(buf, 1, (int)(*pBytes), l_file);
    if (nBytes > 0) {
        *pBytes = nBytes;
        return QSPY_TARGET_INPUT_EVT;
    }

    /* no more input available from the file, QSPY is done */
    return QSPY_DONE_EVT;
}
/*..........................................................................*/
static QSpyStatus file_send2Target(unsigned char *buf, size_t nBytes) {
    (void)buf;
    (void)nBytes;
    return QSPY_ERROR;
}
/*..........................................................................*/
static void file_cleanup(void) {
    kbd_close(); /* close the keyboard */

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
        fprintf(stderr, "*** PAL: Back-End create error %d\n", errno);
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
    if (bind(l_beSock, (struct sockaddr *)&local, sizeof(local)) == -1) {
        fprintf(stderr, "*** PAL: Back-End bind error %d\n", errno);
        return QSPY_ERROR;
    }

    /* put the socket into NON-BLOCKING mode... */
    flags = fcntl(l_beSock, F_GETFL, 0);
    if (flags < 0) {
        fprintf(stderr, "*** PAL: Back-End fcntl get error %d\n", errno);
        return QSPY_ERROR;
    }
    flags |= O_NONBLOCK;
    if (fcntl(l_beSock, F_SETFL, flags) != 0) {
        fprintf(stderr, "*** PAL: Back-End fcntl set error %d\n", errno);
        return QSPY_ERROR;
    }

    BE_onStartup();  /* Back-End startup callback */

    /* NOTE:
    * The Back-End socket l_beSock needs to be checked in select() just
    * like all other sources of input. However, the l_beSock socket is
    * added automatically in the updateReadySet() function *later* in
    * the initialization phase. This assumes that PAL_openBE() is called
    * always *before* opening the specific Target link (see configure() in
    * main.c).
    */

    return QSPY_SUCCESS;
}
/*..........................................................................*/
void PAL_closeBE(void) {
    if (l_beSock != INVALID_SOCKET) {
        if (l_beReturnAddrSize > 0) { /* front-end attached? */
            fd_set writeSet;
            struct timeval delay;

            /* Back-End cleanup callback (send detach packet)... */
            BE_onCleanup();

            /* block until the packet comes out... */
            FD_ZERO(&writeSet);
            FD_SET(l_beSock, &writeSet);
            delay.tv_sec  = 0U;
            delay.tv_usec = 2000 * 1000U; /* delay for up to 2 seconds */
            if (select(l_beSock + 1, (fd_set *)0,
                       &writeSet, (fd_set *)0, &delay) == -1)
            {
                fprintf(stderr, "*** PAL: Back-End socket select error %d\n",
                                errno);
            }
        }

        close(l_beSock);
        l_beSock = INVALID_SOCKET;
    }
}
/*..........................................................................*/
void PAL_send2FE(unsigned char const *buf, size_t nBytes) {
    if (l_beReturnAddrSize > 0) { /* front-end attached? */
        if (sendto(l_beSock, (char *)buf, (int)nBytes, 0,
                   &l_beReturnAddr, l_beReturnAddrSize) == -1)
        {
            PAL_detachFE(); /* detach the Front-End */

            fprintf(stderr,
                "*** PAL: Writing to Back-End socket failed; errno=%d\n",
                errno);
        }
    }
}
/*..........................................................................*/
void PAL_detachFE(void) {
    l_beReturnAddrSize = 0;
}

/*..........................................................................*/
void PAL_clearScreen(void) {
    int status = system("clear");
    if (status < 0) {
        fprintf(stderr, "*** PAL: clearing the screen failed\n");
    }
}

/*--------------------------------------------------------------------------*/
static QSPYEvtType be_receive(fd_set const *pReadSet,
                              unsigned char *buf, size_t *pBytes)
{
    if (l_beSock == INVALID_SOCKET) { /* Back-End socket not initialized? */
        return QSPY_NO_EVT;
    }

    /* attempt to receive packet from the Back-End socket */
    if (FD_ISSET(l_beSock, pReadSet)) {
        socklen_t beReturnAddrSize = sizeof(l_beReturnAddr);
        ssize_t nBytes = recvfrom(l_beSock, buf, *pBytes, 0,
                              &l_beReturnAddr, &beReturnAddrSize);
        if (nBytes > 0) {  /* reception succeeded? */
        l_beReturnAddrSize = beReturnAddrSize; /* attach connection */
            *pBytes = (size_t)nBytes;
            return QSPY_FE_INPUT_EVT;
        }
        else {
            if (nBytes < 0) {
                PAL_detachFE(); /* detach from the Front-End */
                fprintf(stderr, "*** PAL: Back-End recvfrom() error %d\n",
                                errno);
                return QSPY_ERROR_EVT;
            }
        }
    }
    return QSPY_NO_EVT;
}

/*..........................................................................*/
static QSpyStatus kbd_open(void) {
    struct termios t;

    /* modify the terminal attributes... */
    /* get the original terminal settings */
    if (tcgetattr(0, &l_termios_saved) == -1) {
        fprintf(stderr, "*** PAL: getting terminal attributes failed\n");
        return QSPY_ERROR;
    }

    t = l_termios_saved;
    t.c_lflag &= ~(ICANON | ECHO); /* disable canonical mode and echo */
    if (tcsetattr(0, TCSANOW, &t) == -1) {
        fprintf(stderr, "*** PAL: setting terminal attributes failed\n");
        return QSPY_ERROR;
    }

    return QSPY_SUCCESS;
}
/*..........................................................................*/
static void kbd_close(void) {
    /* restore the saved terminal settings */
    tcsetattr(0, TCSANOW, &l_termios_saved);
}
/*..........................................................................*/
static QSPYEvtType kbd_receive(fd_set const *pReadSet,
                               unsigned char *buf, size_t *pBytes)
{
    if (FD_ISSET(0, pReadSet)) {
        *pBytes = read(0, buf, 1); /* the key pressed */
        if (*pBytes > 0) {
            return QSPY_KEYBOARD_EVT;
        }
    }
    return QSPY_NO_EVT;
}
/*..........................................................................*/
static void updateReadySet(int targetConn) {
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
