/*****************************************************************************
* Product: Quantum Spy -- TCP/IP communication HAL for Win32
* Last Updated for Version: 4.5.04
* Date of the Last Update:  Jan 31, 2013
*
*                    Q u a n t u m     L e a P s
*                    ---------------------------
*                    innovating embedded systems
*
* Copyright (C) 2002-2013 Quantum Leaps, LLC. All rights reserved.
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
* Quantum Leaps Web sites: http://www.quantum-leaps.com
*                          http://www.state-machine.com
* e-mail:                  info@quantum-leaps.com
*****************************************************************************/
#include <winsock2.h>                    /* for WindSock network facilities */
#include <stdio.h>
#include <conio.h>

#include "hal.h"

/*..........................................................................*/
SOCKET l_serverSock = INVALID_SOCKET;
SOCKET l_clentSock  = INVALID_SOCKET;

/*..........................................................................*/
int HAL_tcpOpen(int portNum) {
    struct sockaddr_in local;
    ULONG ioctl_opt = 1;

    /* initialize Windows sockets */
    static WSADATA wsaData;
    int wsaErr = WSAStartup(MAKEWORD(2,0), &wsaData);
    if (wsaErr == SOCKET_ERROR) {
        printf("Windows Sockets cannot be initialized.\n"
               "The library reported error 0x%08X.", wsaErr);
        return 0;                                                  /* false */
    }

    l_serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); /* TCP socket */
    if (l_serverSock == INVALID_SOCKET){
        printf("Server socket cannot be created.\n"
               "Windows socket error 0x%08X.",
               WSAGetLastError());
        return 0;                                                  /* false */
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
        printf("Error by binding server socket.\n");
        return 0;                                                  /* false */
    }

    if (listen(l_serverSock, 1) == SOCKET_ERROR) {
        printf("Server socket listen failed.\n"
               "Windows socket error 0x%08X.",
               WSAGetLastError());
        return 0;                                                  /* false */
    }
    /* Set the socket to non-blocking mode. */
    if (ioctlsocket(l_serverSock, FIONBIO, &ioctl_opt) == SOCKET_ERROR) {
        printf("Socket configuration failed.\n"
               "Windows socket error 0x%08X.",
               WSAGetLastError());
        return 0;                                                  /* false */
    }
    return 1;                                                    /* success */
}
/*..........................................................................*/
void HAL_tcpClose(void) {
    if (l_serverSock != INVALID_SOCKET) {
        closesocket(l_serverSock);
    }
    WSACleanup();
}
/*..........................................................................*/
int HAL_tcpRead(unsigned char *buf, size_t size) {
    fd_set readSet;
    int nfound;
    int n;

    if (l_clentSock == INVALID_SOCKET) {   /* still waiting for the client? */
        struct timeval delay;

        n = 0;                                   /* no data from the client */
        if (_kbhit()) {
            return -1;                                         /* terminate */
        }

        FD_ZERO(&readSet);
        FD_SET(l_serverSock, &readSet);

        delay.tv_sec = 0;
        delay.tv_usec = 200000;

        nfound = select(0, &readSet, 0, 0, &delay);
        if (nfound == SOCKET_ERROR) {
            printf("Server socket select failed.\n"
                   "Windows socket error 0x%08X.",
                   WSAGetLastError());
            return -1;                                         /* terminate */
        }

        if (FD_ISSET(l_serverSock, &readSet)) {
            struct sockaddr_in fromAddr;
            int fromLen = (int)sizeof(fromAddr);
            l_clentSock = accept(l_serverSock,
                                 (struct sockaddr *)&fromAddr, &fromLen);
            if (l_clentSock == INVALID_SOCKET) {
                printf("Server socket accept failed.\n"
                       "Windows socket error 0x%08X.",
                       WSAGetLastError());
                return -1;                                     /* terminate */
            }
            printf("Accepted connection from %s, port %d\n",
                   inet_ntoa(fromAddr.sin_addr),
                   (int)ntohs(fromAddr.sin_port));
        }
    }
    else {
        FD_ZERO(&readSet);
        FD_SET(l_clentSock, &readSet);

        nfound = select(0, &readSet, 0, 0, 0);        /* selective blocking */
        if (nfound == SOCKET_ERROR) {
            printf("Client socket select failed.\n"
                   "Windows socket error 0x%08X.",
                   WSAGetLastError());
            return -1;                                         /* terminate */
        }
        if (FD_ISSET(l_clentSock, &readSet)) {
            n = recv(l_clentSock, (char *)buf, (int)size, 0);
            if (n == SOCKET_ERROR) {
                printf("Client socket error.\n"
                       "Windows socket error 0x%08X.",
                       WSAGetLastError());
            }
            else if (n <= 0) {                        /* the client hang up */
                closesocket(l_clentSock);

                /* go back to waiting for a client, or a keypress
                * to terminate
                */
                l_clentSock = INVALID_SOCKET;
                return 0;                            /* no data from client */
            }
        }
        else {
            n = 0;                               /* no data from the client */
        }
    }

    return n;
}
