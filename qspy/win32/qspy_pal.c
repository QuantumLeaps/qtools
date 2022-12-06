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
* @brief QSPY PAL implementation for Win32
*/
#include <stdlib.h>   /* for system() */
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4255)
#endif

#include <ws2tcpip.h> /* for Windows socket facilities */

#ifdef _MSC_VER
#pragma warning(pop)
#endif

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
    FE_DETACHED = 0,   /* Front-End detached */
    PAL_TOUT_MS = 10,  /* determines how long to wait for an event [ms] */
};

/* fron-end address */
typedef union {
    uint64_t data[2];
    struct sockaddr addr;
} fe_addr;

static bool l_kbd_inp = false;
static bool l_color = false;

static HANDLE       l_serHNDL;
static COMMTIMEOUTS l_timeouts;

static SOCKET  l_beSock = INVALID_SOCKET;
static fe_addr l_feAddr;
static int     l_feAddrSize = FE_DETACHED;

static SOCKET l_serverSock = INVALID_SOCKET;
static SOCKET l_clientSock = INVALID_SOCKET;

static FILE *l_file = (FILE *)0;

/*==========================================================================*/
/* Ctrl-C handler */
static BOOL WINAPI CtrlHandler(_In_ DWORD dwCtrlType) {
    (void)dwCtrlType; /* unused parameter */
    QSPY_cleanup();
    exit(0);
}

/* Keyboard input */
QSpyStatus PAL_openKbd(bool kbd_inp, bool color) {
    l_kbd_inp = kbd_inp;
    l_color = color;
    SetConsoleCtrlHandler(&CtrlHandler, TRUE);
    if (l_color) {
        system("color"); /* to support color output */
        fputs("\033[0m", stdout); /* switch default colors */
    }
    return QSPY_SUCCESS;
}
/*..........................................................................*/
void PAL_closeKbd(void) {
    if (l_color) {
        fputs("\033[0m", stdout); /* switch default colors */
    }
}

/*==========================================================================*/
/* Win32 serial communication with the Target */
QSpyStatus PAL_openTargetSer(char const *comName, int baudRate) {
    DCB dcb;
    char comPortName[40];
    char comSettings[120];

    /* setup the PAL virtual table for the Serial communication... */
    PAL_vtbl.getEvt      = &ser_getEvt;
    PAL_vtbl.send2Target = &ser_send2Target;
    PAL_vtbl.cleanup     = &ser_cleanup;

    /* open serial port (use \\.\COM<num> name to allow large <num>)... */
    SNPRINTF_S(comPortName, sizeof(comPortName), "\\\\.\\%s", comName);
    l_serHNDL = CreateFile((LPCSTR)comPortName,
                       GENERIC_READ | GENERIC_WRITE,
                       0U,            /* exclusive access */
                       NULL,          /* no security attrs */
                       OPEN_EXISTING,
                       0U,            /* standard (not-overlapped) I/O */
                       NULL);

    if (l_serHNDL == INVALID_HANDLE_VALUE) {
        SNPRINTF_LINE("   <COMMS> ERROR    Opening COM Port=%s,Baud=%d",
                      comName, baudRate);
        QSPY_printError();
        return QSPY_ERROR;
    }

    /* configure the serial port... */
    SNPRINTF_S(comSettings, sizeof(comSettings),
        "baud=%d parity=N data=8 stop=1 odsr=off dtr=on octs=off rts=on",
        baudRate);
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(l_serHNDL, &dcb)) {
        SNPRINTF_LINE("   <COMMS> ERROR    %s", "Getting COM port settings");
        QSPY_printError();
        return QSPY_ERROR;
    }

    /* drill in the DCB... */
    dcb.fAbortOnError = 0U; /* don't abort on error */
    if (!BuildCommDCB((LPCSTR)comSettings, &dcb)) {
        SNPRINTF_LINE("   <COMMS> ERROR    %s", "Parsing COM port settings");
        QSPY_printError();
        return QSPY_ERROR;
    }

    if (!SetCommState(l_serHNDL, &dcb)) {
        SNPRINTF_LINE("   <COMMS> ERROR    %s", "Setting up the COM port");
        QSPY_printError();
        return QSPY_ERROR;
    }

    QSPY_reset();   /* reset the QSPY parser to start over cleanly */
    QSPY_txReset(); /* reset the QSPY transmitter */

    /* setup the serial port buffers... */
    SetupComm(l_serHNDL,
              4*1024,   /* 4K input buffer  */
              4*1024);  /* 4K output buffer */

    /* purge any information in the buffers */
    PurgeComm(l_serHNDL, PURGE_TXABORT | PURGE_RXABORT
                         | PURGE_TXCLEAR | PURGE_RXCLEAR);

    /* the read timeouts for the serial communication are set accorging
    * to the following remark from the Win32 help documentation:
    *
    * If an application sets ReadIntervalTimeout and
    * ReadTotalTimeoutMultiplier to MAXDWORD and sets
    * ReadTotalTimeoutConstant to a value greater than zero and less than
    * MAXDWORD, one of the following occurs when the ReadFile function
    * is called:
    * 1. If there are any characters in the input buffer, ReadFile
    * returns immediately with the characters in the buffer.
    * 2. If there are no characters in the input buffer, ReadFile waits
    * until a character arrives and then returns immediately.
    * 3. If no character arrives within the time specified by
    * ReadTotalTimeoutConstant, ReadFile times out.
    */
    l_timeouts.ReadIntervalTimeout        = MAXDWORD;
    l_timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    l_timeouts.ReadTotalTimeoutConstant   = PAL_TOUT_MS;

    /* the write timeouts for the serial communication are set accorging
    * to the following remark from the Win32 help documentation:
    *
    * A value of zero for both the WriteTotalTimeoutMultiplier and
    * WriteTotalTimeoutConstant members indicates that total time-outs
    * are not used for write operations.
    *
    * This means that the WriteFile() returns immediately and the
    * serial driver must cache any bytes that have not been sent yet.
    * (see also the output buffer setting for SetupComm() earlier).
    *
    * Exceeding the write buffer capacity indicates that the Target
    * cannot accept all the bytes at this rate. This error will produce
    * an error message to the screen.
    */
    l_timeouts.WriteTotalTimeoutMultiplier = 0;
    l_timeouts.WriteTotalTimeoutConstant   = 0;

    SetCommTimeouts(l_serHNDL, &l_timeouts);

    return QSPY_SUCCESS;
}
/*..........................................................................*/
static QSPYEvtType ser_getEvt(unsigned char *buf, uint32_t *pBytes) {
    QSPYEvtType evtType;

    /* try to receive data from keyboard... */
    evtType = PAL_receiveKbd(buf, pBytes);
    if (evtType != QSPY_NO_EVT) {
        return evtType;
    }

    /* try to receive data from the Back-End socket... */
    evtType = PAL_receiveBe(buf, pBytes);
    if (evtType != QSPY_NO_EVT) {
        return evtType;
    }

    /* try to receive data from the Target... */
    /* NOTE: If data is not available immediately, this call blocks
    * for up to PAL_TOUT_MS milliseconds.
    */
    if (ReadFile(l_serHNDL, buf, (DWORD)(*pBytes), (LPDWORD)pBytes, NULL)) {
        if (*pBytes > 0) {
            return QSPY_TARGET_INPUT_EVT;
        }
    }
    else {
        COMSTAT comstat;
        DWORD errors;

        SNPRINTF_LINE("   <COMMS> ERROR    Reading COM port failed Err=%d",
                      WSAGetLastError());
        QSPY_printError();
        ClearCommError(l_serHNDL, &errors, &comstat);

        *pBytes = 0;
        return QSPY_ERROR_EVT;
    }

    return QSPY_NO_EVT;
}
/*..........................................................................*/
static QSpyStatus ser_send2Target(unsigned char *buf, uint32_t nBytes) {
    DWORD nBytesWritten;

    if (WriteFile(l_serHNDL, buf, (DWORD)nBytes, &nBytesWritten, NULL)) {
        if (nBytesWritten == (DWORD)nBytes) {
            return QSPY_SUCCESS;
        }
        else {
            return QSPY_ERROR;
        }
    }
    else {
        SNPRINTF_LINE("   <COMMS> ERROR    Writing to COM port Err=%d",
                      WSAGetLastError());
        QSPY_printError();
        return QSPY_ERROR;
    }
}
/*..........................................................................*/
static void ser_cleanup(void) {
    CloseHandle(l_serHNDL);
}


/*==========================================================================*/
/* Win32 TCP/IP communication with the Target */

static struct sockaddr_in l_clientAddr;

/*..........................................................................*/
QSpyStatus PAL_openTargetTcp(int portNum) {
    struct sockaddr_in local;
    u_long sockmode;
    WSADATA wsaData;

    /* setup the PAL virtual table for the TCP/IP Target connection... */
    PAL_vtbl.getEvt      = &tcp_getEvt;
    PAL_vtbl.send2Target = &tcp_send2Target;
    PAL_vtbl.cleanup     = &tcp_cleanup;

    /* itialize Windows sockets version 2.2)... */
    /* WSAStartup() might be already called from PAL_openBE(), but
    * in case the Back-End socket is not used it must be called here.
    * NOTE: It's OK to call WSAStartup() multiple times, as long as
    * the cleanup is performed equal number of times.
    */
    int wsaErr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaErr == SOCKET_ERROR) {
        SNPRINTF_LINE("   <COMMS> ERROR    Init Windows Sockets Err=%d",
                      wsaErr);
        QSPY_printError();
        return QSPY_ERROR;
    }

    /* create TCP socket */
    l_serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (l_serverSock == INVALID_SOCKET) {
        SNPRINTF_LINE("   <COMMS> ERROR    Server socket create Err=%d",
                      WSAGetLastError());
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
        SNPRINTF_LINE("   <COMMS> ERROR    Binding server socket Err=%d",
                      WSAGetLastError());
        QSPY_printError();
        return QSPY_ERROR;
    }

    if (listen(l_serverSock, 1) == SOCKET_ERROR) {
        SNPRINTF_LINE("   <COMMS> ERROR    Server socket listen Err=%d",
                      WSAGetLastError());
        QSPY_printError();
        return QSPY_ERROR;
    }

    /* put the socket into NON-BLOCKING mode... */
    sockmode = 1UL;  /* NON-BLOCKING socket */
    if (ioctlsocket(l_serverSock, FIONBIO, &sockmode) == SOCKET_ERROR) {
        SNPRINTF_LINE("   <COMMS> ERROR    Server socket non-blocking Err=%d",
                      WSAGetLastError());
        QSPY_printError();
        return QSPY_ERROR;
    }

    return QSPY_SUCCESS;
}
/*..........................................................................*/
static void tcp_cleanup(void) {
    if (l_serverSock != INVALID_SOCKET) {
        closesocket(l_serverSock);
    }
    WSACleanup();
}
/*..........................................................................*/
static QSPYEvtType tcp_getEvt(unsigned char *buf, uint32_t *pBytes) {
    QSPYEvtType evtType;
    struct timeval timeout = {(long)0, (long)(PAL_TOUT_MS*1000)};
    fd_set readSet;
    int status;
    int nrec;
    char client_hostname[128];

    /* try to receive data from keyboard... */
    evtType = PAL_receiveKbd(buf, pBytes);
    if (evtType != QSPY_NO_EVT) {
        return evtType;
    }

    /* try to receive data from the Back-End socket... */
    evtType = PAL_receiveBe(buf, pBytes);
    if (evtType != QSPY_NO_EVT) {
        return evtType;
    }

    /* still waiting for the client? */
    if (l_clientSock == INVALID_SOCKET) {

        FD_ZERO(&readSet);
        FD_SET(l_serverSock, &readSet);

        /* selective, timed blocking... */
        status = select(0, &readSet, (fd_set *)0, (fd_set *)0, &timeout);

        if (status == SOCKET_ERROR) {
            SNPRINTF_LINE("   <COMMS> ERROR    Server socket select Err=%d",
                          WSAGetLastError());
            QSPY_printError();
            return QSPY_ERROR_EVT;
        }
        else if (FD_ISSET(l_serverSock, &readSet)) {
            int fromLen = (int)sizeof(l_clientAddr);
            ULONG sockmode;
            //BOOL  sockopt_bool;

            l_clientSock = accept(l_serverSock,
                                 (struct sockaddr *)&l_clientAddr, &fromLen);
            if (l_clientSock == INVALID_SOCKET) {
                SNPRINTF_LINE("   <COMMS> ERROR    Socket accept Err=%d",
                              WSAGetLastError());
                QSPY_printError();
                return QSPY_ERROR_EVT;
            }

            /* put the client socket into NON-BLOCKING mode... */
            sockmode = 1UL;  /* NON-BLOCKING socket */
            if (ioctlsocket(l_clientSock, FIONBIO, &sockmode)
                == SOCKET_ERROR)
            {
                SNPRINTF_LINE("   <COMMS> ERROR    Client socket non-blocking"
                              " Err=%d", WSAGetLastError());
                QSPY_printError();
                return QSPY_ERROR_EVT;
            }

            QSPY_reset();   /* reset the QSPY parser to start over cleanly */
            QSPY_txReset(); /* reset the QSPY transmitter */

#ifdef _MSC_VER
            inet_ntop(l_clientAddr.sin_family, &l_clientAddr.sin_addr,
                      client_hostname, sizeof(client_hostname));
#else
            strncpy(client_hostname, inet_ntoa(l_clientAddr.sin_addr),
                    sizeof(client_hostname) - 1U);
#endif
            SNPRINTF_LINE("   <COMMS> TCP-IP   Connected to Host=%s,Port=%d",
                      client_hostname, (int)ntohs(l_clientAddr.sin_port));
            QSPY_printInfo();
        }
    }
    else { /* client is connected... */
        FD_ZERO(&readSet);
        FD_SET(l_clientSock, &readSet);

        /* selective, timed blocking... */
        status = select(0, &readSet, (fd_set *)0, (fd_set *)0, &timeout);

        if (status == SOCKET_ERROR) {
            SNPRINTF_LINE("   <COMMS> ERROR    Client socket select Err=%d",
                          WSAGetLastError());
            QSPY_printError();
            return QSPY_ERROR_EVT;
        } else if (FD_ISSET(l_clientSock, &readSet)) {
            nrec = recv(l_clientSock, (char *)buf, (int)(*pBytes), 0);
            if (nrec <= 0) { /* the client hang up */
#ifdef _MSC_VER
                inet_ntop(l_clientAddr.sin_family, &l_clientAddr.sin_addr,
                    client_hostname, sizeof(client_hostname));
#else
                strncpy(client_hostname, inet_ntoa(l_clientAddr.sin_addr),
                    sizeof(client_hostname) - 1U);
#endif
                SNPRINTF_LINE("   <COMMS> TCP-IP   Disconn from "
                              "Host=%s,Port=%d",
                              client_hostname,
                              (int)ntohs(l_clientAddr.sin_port));
                QSPY_printInfo();

                /* go back to waiting for a client */
                closesocket(l_clientSock);
                l_clientSock = INVALID_SOCKET;
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
    if (send(l_clientSock, (char *)buf, (int)nBytes, 0) == SOCKET_ERROR) {
        SNPRINTF_LINE("   <COMMS> ERROR    Writing to TCP socket Err=%d",
                      WSAGetLastError());
        QSPY_printError();
        return QSPY_ERROR;
    }
    return QSPY_SUCCESS;
}

/*==========================================================================*/
/* File communication with the "Target" */
QSpyStatus PAL_openTargetFile(char const *fName) {
    /* setup the PAL virtual table for the File connection... */
    PAL_vtbl.getEvt      = &file_getEvt;
    PAL_vtbl.send2Target = &file_send2Target;
    PAL_vtbl.cleanup     = &file_cleanup;

    FOPEN_S(l_file, fName, "rb"); /* open for reading binary */
    if (l_file != (FILE *)0) {
        QSPY_reset();   /* reset the QSPY parser to start over cleanly */
        QSPY_txReset(); /* reset the QSPY transmitter */

        SNPRINTF_LINE("   <COMMS> File     Opened File=%s", fName);
        QSPY_printInfo();
        return QSPY_SUCCESS;
    }
    else {
        SNPRINTF_LINE("   <COMMS> ERROR    Cannot find File=%s", fName);
        QSPY_printError();
        return QSPY_ERROR;
    }
}
/*..........................................................................*/
static QSPYEvtType file_getEvt(unsigned char *buf, uint32_t *pBytes) {
    QSPYEvtType evtType;
    uint32_t nBytes;

    /* try to receive data from keyboard... */
    evtType = PAL_receiveKbd(buf, pBytes);
    if (evtType != QSPY_NO_EVT) {
        return evtType;
    }

    /* try to receive data from the Back-End socket... */
    evtType = PAL_receiveBe(buf, pBytes);
    if (evtType != QSPY_NO_EVT) {
        return evtType;
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
    u_long sockmode;
    WSADATA wsaData;
    int wsaErr;

    /* itialize Windows sockets version 2.2)... */
    /* NOTE: assuming that PAL_openBE() is always called before initialzing
    * the target connection, including the TCP/IP connection, the
    */
    wsaErr = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaErr == SOCKET_ERROR) {
        SNPRINTF_LINE("   <F-END> ERROR    Windows Sockets Init Err=%d",
                      wsaErr);
        QSPY_printError();
        return QSPY_ERROR;
    }

    l_beSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); /* UDP socket */
    if (l_beSock == INVALID_SOCKET){
        SNPRINTF_LINE("   <F-END> ERROR    UDP socket create Err=%d",
                      WSAGetLastError());
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
        SNPRINTF_LINE("   <F-END> ERROR    UDP socket binding Err=%d",
                      WSAGetLastError());
        QSPY_printError();
        return QSPY_ERROR;
    }

    /* put the socket into NON-BLOCKING mode... */
    sockmode = 1UL;  /* NON-BLOCKING socket */
    if (ioctlsocket(l_beSock, FIONBIO, &sockmode) == SOCKET_ERROR) {
        SNPRINTF_LINE("   <F-END> ERROR    UDP socket non-blocking Err=%d",
                      WSAGetLastError());
        QSPY_printError();
        return QSPY_ERROR;
    }

    BE_onStartup();  /* Back-End startup callback */

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
            if (select(0, (fd_set *)0, &writeSet, (fd_set *)0, &delay)
                == SOCKET_ERROR)
            {
                SNPRINTF_LINE("   <F-END> ERROR    UDP socket select Err=%d",
                        WSAGetLastError());
                QSPY_printError();
            }
        }

        closesocket(l_beSock);
        l_beSock = INVALID_SOCKET;
    }
    WSACleanup();
}
/*..........................................................................*/
void PAL_send2FE(unsigned char const *buf, uint32_t nBytes) {
    if (l_feAddrSize != FE_DETACHED) { /* front-end attached? */
        if (sendto(l_beSock, (char *)buf, (int)nBytes, 0,
                   &l_feAddr.addr, l_feAddrSize) == SOCKET_ERROR)
        {
            PAL_detachFE(); /* detach the Front-End */

            SNPRINTF_LINE("   <F-END> ERROR    UDP socket failed Err=%d",
                          WSAGetLastError());
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
    system("cls");
}

/*--------------------------------------------------------------------------*/
QSPYEvtType PAL_receiveBe(unsigned char *buf, uint32_t *pBytes) {
    fe_addr feAddr;
    int feAddrSize;
    int status;

    if (l_beSock == INVALID_SOCKET) { /* Back-End socket not initialized? */
        return QSPY_NO_EVT;
    }

    /* receive a packet from the Back-End socket */
    feAddrSize = sizeof(feAddr);
    status = recvfrom(l_beSock, (char *)buf, (int)*pBytes, 0,
                      &feAddr.addr, &feAddrSize);

    if (status == SOCKET_ERROR) { /* socket error - most likely would block */
        status = WSAGetLastError();
        if (status != WSAEWOULDBLOCK) {
            PAL_detachFE(); /* detach from the Front-End */

            SNPRINTF_LINE("   <F-END> ERROR    UDP socket failed Err=%d",
                          status);
            QSPY_printError();
            return QSPY_ERROR_EVT;
        }
    }
    else if (l_feAddrSize == 0) { /* not attached yet? */
        memcpy(&l_feAddr, &feAddr, feAddrSize);
        l_feAddrSize = feAddrSize; /* attach connection */
        *pBytes = (uint32_t)status;
        return QSPY_FE_INPUT_EVT;
    }
    else { /* already attached */
        /* is this from the attached front-end address? */
        if ((feAddrSize == l_feAddrSize)
            && (feAddr.data[1] == l_feAddr.data[1])
            && (feAddr.data[0] == l_feAddr.data[0]))
        {
            *pBytes = (uint32_t)status;
            return QSPY_FE_INPUT_EVT;
        }
        else {
            SNPRINTF_LINE("   <F-END> WARN     %s", "UDP socket in use");
            QSPY_printError();
            /* this packet is from a DIFFERENT front-end -- ignore it */
        }
    }
    return QSPY_NO_EVT;
}

/*..........................................................................*/
QSPYEvtType PAL_receiveKbd(unsigned char *buf, uint32_t *pBytes) {
    if (l_kbd_inp) {
        wint_t ch = 0;
        while (_kbhit()) {
            ch = _getwch();
            if ((ch == 0x00) || (ch == 0xE0)) {
                ch = _getwch();
            }
        }
        if (ch != 0) {
            buf[0]  = (unsigned char)ch;
            *pBytes = 1; /* number of bytes in the buffer */
            return QSPY_KEYBOARD_EVT;
        }
    }
    return QSPY_NO_EVT;
}

