/******************************************************************************
 * UDP Extension for Tcl 8.4
 *
 * Copyright (c) 1999-2000 by Columbia University; all rights reserved
 * Copyright (c) 2003-2005 Pat Thoyts <patthoyts@users.sourceforge.net>
 *
 * Written by Xiaotao Wu
 * Last modified: 11/03/2000
 *
 * $Id: udp_tcl.c,v 1.48 2014/08/24 07:17:21 huubeikens Exp $
 ******************************************************************************/

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

#include "udp_tcl.h"

#ifdef WIN32
#include <stdlib.h>
#include <malloc.h>
typedef int socklen_t;
#else /* ! WIN32 */
#if defined(HAVE_SYS_FILIO_H)
#include <sys/filio.h>
#endif
#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif
#if !defined(HAVE_SYS_FILIO_H) && !defined(HAVE_SYS_IOCTL_H)
#error "Neither sys/ioctl.h nor sys/filio.h found. We need ioctl()"
#endif
#endif /* WIN32 */

#if HAVE_FCNTL_H
#  include <fcntl.h>
#endif

/* Tcl 8.4 CONST support */
#ifndef CONST84
#define CONST84
#endif

/* bug #1240127: May not be found on certain versions of mingw-gcc */
#ifndef IP_TTL
#define IP_TTL 4
#endif

#if defined(_XOPEN_SOURCE_EXTENDED) && defined(__hpux)
/*
 * This won't get defined on HP-UX if _XOPEN_SOURCE_EXTENDED is defined,
 * but we need it and TEA causes this macro to be defined.
 */

struct ip_mreq {
    struct in_addr imr_multiaddr; /* IP multicast address of group */
    struct in_addr imr_interface; /* local IP address of interface */
};

struct ipv6_mreq {
    struct in6_addr ipv6mr_multiaddr; /* IPv6 multicast addr */
    unsigned int    ipv6mr_interface; /* interface index */
};

#endif /* _XOPEN_SOURCE_EXTENDED */

/* define some Win32isms for Unix */
#ifndef WIN32
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#define ioctlsocket ioctl
#endif /* WIN32 */

#ifdef DEBUG
#define UDPTRACE udpTrace
#else
#define UDPTRACE 1 ? ((void)0) : udpTrace
#endif

#ifdef _MSC_VER
#define snprintf _snprintf      /* trust Microsoft to complicate things */
#endif

FILE *dbg;

#define MAXBUFFERSIZE 4096

static char errBuf[256];

/*
 * Channel handling procedures
 */
static Tcl_DriverOutputProc    udpOutput;
static Tcl_DriverInputProc     udpInput;
static Tcl_DriverCloseProc     udpClose;
static Tcl_DriverWatchProc     udpWatch;
static Tcl_DriverGetHandleProc udpGetHandle;
static Tcl_DriverSetOptionProc udpSetOption;
static Tcl_DriverGetOptionProc udpGetOption;

/*
 * Tcl command procedures
 */
int Udp_CmdProc(ClientData, Tcl_Interp *, int, Tcl_Obj *CONST []);
int udpOpen(ClientData , Tcl_Interp *, int , CONST84 char * []);
int udpConf(ClientData , Tcl_Interp *, int , CONST84 char * []);
int udpPeek(ClientData , Tcl_Interp *, int , CONST84 char * []);

/*
 * internal functions
 */
static int UdpMulticast(UdpState *statePtr, Tcl_Interp *, const char *, int);
static int UdpSockGetPort(Tcl_Interp *interp, const char *s,
                          const char *proto, int *portPtr);
static void udpTrace(const char *format, ...);
static int  udpGetService(Tcl_Interp *interp, const char *service,
                          unsigned short *servicePort);
static Tcl_Obj *ErrorToObj(const char * prefix);
static int hasOption(int argc, CONST84 char * argv[],const char* option );
static int udpSetRemoteOption(UdpState* statePtr, Tcl_Interp *interp, CONST84 char *newValue);
static int udpSetMulticastAddOption(UdpState* statePtr, Tcl_Interp *interp, CONST84 char *newValue);
static int udpSetMulticastDropOption(UdpState* statePtr, Tcl_Interp *interp, CONST84 char *newValue);
static int udpSetBroadcastOption(UdpState* statePtr, Tcl_Interp *interp, CONST84 char *newValue);
static int udpGetBroadcastOption(UdpState* statePtr, Tcl_Interp *interp, int* value);
static int udpSetMcastloopOption(UdpState* statePtr, Tcl_Interp *interp, CONST84 char *newValue);
static int udpGetMcastloopOption(UdpState *statePtr, Tcl_Interp *interp, unsigned char * value);
static int udpSetTtlOption(UdpState* statePtr, Tcl_Interp *interp, CONST84 char *newValue);
static int udpGetTtlOption(UdpState *statePtr, Tcl_Interp *interp,unsigned int *value);

/*
 * Windows specific functions
 */
#ifdef WIN32

int  UdpEventProc(Tcl_Event *evPtr, int flags);
static void UDP_SetupProc(ClientData data, int flags);
void UDP_CheckProc(ClientData data, int flags);
int  Udp_WinHasSockets(Tcl_Interp *interp);

/* FIX ME - these should be part of a thread/package specific structure */
static HANDLE waitForSock;
static HANDLE waitSockRead;
static HANDLE sockListLock;
static UdpState *sockList;

#endif /* ! WIN32 */

/*
 * This structure describes the channel type for accessing UDP.
 */
static Tcl_ChannelType Udp_ChannelType = {
    "udp",                 /* Type name.                                    */
    NULL,                  /* Set blocking/nonblocking behaviour. NULL'able */
    udpClose,              /* Close channel, clean instance data            */
    udpInput,              /* Handle read request                           */
    udpOutput,             /* Handle write request                          */
    NULL,                  /* Move location of access point.      NULL'able */
    udpSetOption,          /* Set options.                        NULL'able */
    udpGetOption,          /* Get options.                        NULL'able */
    udpWatch,              /* Initialize notifier                           */
    udpGetHandle,          /* Get OS handle from the channel.               */
};

/*
 * ----------------------------------------------------------------------
 * udpInit
 * ----------------------------------------------------------------------
 */
int
Udp_Init(Tcl_Interp *interp)
{
    int r = TCL_OK;
#if defined(DEBUG) && !defined(WIN32)
    dbg = fopen("udp.dbg", "wt");
#endif

#ifdef USE_TCL_STUBS
    Tcl_InitStubs(interp, "8.1", 0);
#endif

#ifdef WIN32
    if (Udp_WinHasSockets(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    Tcl_CreateEventSource(UDP_SetupProc, UDP_CheckProc, NULL);
#endif

    Tcl_CreateCommand(interp, "udp_open", udpOpen ,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "udp_conf", udpConf ,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);
    Tcl_CreateCommand(interp, "udp_peek", udpPeek ,
                      (ClientData) NULL, (Tcl_CmdDeleteProc *) NULL);

    r = Tcl_PkgProvide(interp, PACKAGE_NAME, PACKAGE_VERSION);
    return r;
}

int
Udp_SafeInit(Tcl_Interp *interp)
{
    Tcl_SetResult(interp, "permission denied", TCL_STATIC);
    return TCL_ERROR;
}

/*
 * ----------------------------------------------------------------------
 * Udp_CmdProc --
 *  Provide a user interface similar to the Tcl stock 'socket' command.
 *
 *  udp ?options?
 *  udp ?options? host port
 *  udp -server command ?options? port
 *
 * ----------------------------------------------------------------------
 */
int
Udp_CmdProc(ClientData clientData, Tcl_Interp *interp,
            int objc, Tcl_Obj *CONST objv[])
{
    Tcl_SetResult(interp, "E_NOTIMPL", TCL_STATIC);
    return TCL_ERROR;
}

/*
 * Probably we should provide an equivalent to the C API for TCP.
 *
 * Tcl_Channel Tcl_OpenUdpClient(interp, port, host, myaddr, myport, async);
 * Tcl_Channel Tcl_OpenUdpServer(interp, port, myaddr, proc, clientData);
 * Tcl_Channel Tcl_MakeUdpClientChannel(sock);
 */

/*
 * ----------------------------------------------------------------------
 * checkOption --
 *
 *  Checks if the specified option is part of the specified command line options.
 *  Returns 1 if option is present, otherwise 0.
 * ----------------------------------------------------------------------
 */
static int
hasOption(int argc, CONST84 char * argv[],const char* option )
{
    int i;
    for (i=0;i<argc;i++) {
        if (strcmp(option, argv[i])==0) {
            /* Option found. */
            return 1;
        }
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------
 * udpOpen --
 *
 *  opens a UDP socket and addds the file descriptor to the tcl
 *  interpreter
 * ----------------------------------------------------------------------
 */
int
udpOpen(ClientData clientData, Tcl_Interp *interp,
        int argc, CONST84 char * argv[])
{
    int sock;
    char channelName[20];
    UdpState *statePtr;
    uint16_t localport = 0;
    int reuse = 0;
    struct sockaddr_storage addr,sockaddr;
    socklen_t addr_len;
    unsigned long status = 1;
    socklen_t len;
    short ss_family = AF_INET; /* Default ipv4 */
    char errmsg[] = "upd_open [remoteport] [ipv6] [reuse]";
    int remaining_options = argc;

    if (argc >= 2) {
        if (hasOption(argc,argv,"reuse")) {
           reuse = 1;
           remaining_options--;
         }

        if (hasOption(argc,argv,"ipv6")) {
             ss_family = AF_INET6;
             remaining_options--;
         }
        /* The remaining option must be the port (if specified) */
        if (remaining_options == 2) {
           if (udpGetService(interp, argv[1], &localport) != TCL_OK) {
                Tcl_SetResult (interp, errmsg, NULL);
                return TCL_ERROR;
           }
        }
    }
    memset(channelName, 0, sizeof(channelName));

    sock = socket(ss_family, SOCK_DGRAM, 0);
    if (sock < 0) {
        snprintf(errBuf, 255, "failed to create socket");
        errBuf[255] = 0;
        UDPTRACE("%s\n", errBuf);
        Tcl_AppendResult(interp, errBuf, (char *)NULL);
        return TCL_ERROR;
    }

    /*
     * bug #1477669: avoid socket inheritence after exec
     */

#if HAVE_FLAG_FD_CLOEXEC
    fcntl(sock, F_SETFD, FD_CLOEXEC);
#else
#ifdef WIN32
    if (SetHandleInformation((HANDLE)sock, HANDLE_FLAG_INHERIT, 0) == 0) {
        Tcl_AppendResult(interp, "failed to set close-on-exec bit", NULL);
        return TCL_ERROR;
    }
#endif /* WIN32 */
#endif /* HAVE_FLAG_FD_CLOEXEC */

    if (reuse) {
        int one = 1;
#ifdef SO_REUSEPORT
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT,
                       (const char *)&one, sizeof(one)) < 0) {
#else
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
                       (const char *)&one, sizeof(one)) < 0) {
#endif
             Tcl_SetObjResult(interp,
                             ErrorToObj("error setting socket option"));
            closesocket(sock);
            return TCL_ERROR;
        }
    }

    memset(&addr, 0, sizeof(addr));
    if (ss_family == AF_INET6) {
        ((struct sockaddr_in6 *) &addr)->sin6_family = AF_INET6;
        ((struct sockaddr_in6 *) &addr)->sin6_port = localport;
        addr_len = sizeof(struct sockaddr_in6);
    } else {
        ((struct sockaddr_in *) &addr)->sin_family = AF_INET;
        ((struct sockaddr_in *) &addr)->sin_port = localport;
        addr_len = sizeof(struct sockaddr_in);
    }
    if ( bind(sock,(struct sockaddr *)&addr, addr_len) < 0) {
        Tcl_SetObjResult(interp,
                         ErrorToObj("failed to bind socket to port"));

        closesocket(sock);
        return TCL_ERROR;
    }

    ioctlsocket(sock, FIONBIO, &status);

    if (localport == 0) {
        len = sizeof(sockaddr);
        getsockname(sock, (struct sockaddr *)&sockaddr, &len);
         if (ss_family == AF_INET6) {
            localport = ((struct sockaddr_in6 *) &sockaddr)->sin6_port;
        } else {
            localport = ((struct sockaddr_in *) &sockaddr)->sin_port;
        }
    }

    UDPTRACE("Open socket %d. Bind socket to port %d\n",
             sock, ntohs(localport));

    statePtr = (UdpState *) ckalloc((unsigned) sizeof(UdpState));
    memset(statePtr, 0, sizeof(UdpState));
    statePtr->sock = sock;
    sprintf(channelName, "sock%d", statePtr->sock);
    statePtr->channel = Tcl_CreateChannel(&Udp_ChannelType, channelName,
                                          (ClientData) statePtr,
                                          (TCL_READABLE | TCL_WRITABLE | TCL_MODE_NONBLOCKING));
    statePtr->doread = 1;
    statePtr->multicast = 0;
    statePtr->groupsObj = Tcl_NewListObj(0, NULL);
    Tcl_IncrRefCount(statePtr->groupsObj);
    statePtr->localport = localport;
    statePtr->ss_family = ss_family;
    Tcl_RegisterChannel(interp, statePtr->channel);
#ifdef WIN32
    statePtr->threadId = Tcl_GetCurrentThread();
    statePtr->packetNum = 0;
    statePtr->next = NULL;
    statePtr->packets = NULL;
    statePtr->packetsTail = NULL;
#endif
    /* Tcl_SetChannelOption(interp, statePtr->channel, "-blocking", "0"); */
    Tcl_AppendResult(interp, channelName, (char *)NULL);
#ifdef WIN32
    WaitForSingleObject(sockListLock, INFINITE);
    statePtr->next = sockList;
    sockList = statePtr;

    UDPTRACE("Added %d to sockList\n", statePtr->sock);
    SetEvent(sockListLock);
    SetEvent(waitForSock);
#endif
    return TCL_OK;
}

/*
* ----------------------------------------------------------------------
* udpConf --
* ----------------------------------------------------------------------
*/
int
udpConf(ClientData clientData, Tcl_Interp *interp,
        int argc, CONST84 char * argv[])
{
    Tcl_Channel chan;
    char remoteOptions[255];
    UdpState *statePtr = NULL;
    int r = TCL_ERROR;
    Tcl_DString ds;
    char errmsg[] =
        "udp_conf fileId [-mcastadd] [-mcastdrop] groupaddr | "
#ifdef WIN32
        "udp_conf fileId [-mcastadd] [-mcastdrop] \"groupaddr netwif_index\" | "
#else
        "udp_conf fileId [-mcastadd] [-mcastdrop] \"groupaddr netwif\" | "
#endif
        "udp_conf fileId remotehost remoteport | "
        "udp_conf fileId [-myport] [-remote] [-peer] [-mcastgroups] [-mcastloop] [-broadcast] [-ttl]";

    if (argc >= 2) {
        chan = Tcl_GetChannel(interp, (char *)argv[1], NULL);
        if (chan != (Tcl_Channel) NULL) {
            statePtr = (UdpState *) Tcl_GetChannelInstanceData(chan);
        }
    }

    if (argc == 3 && statePtr != NULL) {
        Tcl_DStringInit(&ds);
        r = Tcl_GetChannelOption(interp, statePtr->channel, argv[2], &ds);
        if (r == TCL_OK) {
            Tcl_DStringResult(interp, &ds);
        }
        Tcl_DStringFree(&ds);
    }

    if (argc == 4 && statePtr != NULL) {
        if (hasOption(argc,argv,"-mcastloop") ||
            hasOption(argc,argv,"-broadcast") ||
            hasOption(argc,argv,"-mcastadd") ||
            hasOption(argc,argv,"-mcastdrop") ||
            hasOption(argc,argv,"-ttl")) {
            r = Tcl_SetChannelOption(interp, statePtr->channel, argv[2], argv[3]);
        } else {
            sprintf(remoteOptions, "%s %s",argv[2],argv[3] );
            r = Tcl_SetChannelOption(interp, statePtr->channel, "-remote", remoteOptions);
        }
    }

    if (r != TCL_OK) {
        Tcl_SetResult (interp, errmsg, NULL);
    }
    return r;
}

/*
 * ----------------------------------------------------------------------
 * udpPeek --
 *  peek some data and set the peer information
 * ----------------------------------------------------------------------
 */
int
udpPeek(ClientData clientData, Tcl_Interp *interp,
        int argc, CONST84 char * argv[])
{
#ifndef WIN32
    int buffer_size = 16;
    int actual_size;
    socklen_t socksize;
    char message[17];
    struct sockaddr_storage recvaddr;

    Tcl_Channel chan;
    UdpState *statePtr;

    if (argc < 2) {
    Tcl_WrongNumArgs(interp, 0, NULL, "udp_peek sock ?buffersize?");
        return TCL_ERROR;
    }
    chan = Tcl_GetChannel(interp, (char *)argv[1], NULL);
    if (chan == (Tcl_Channel) NULL) {
        return TCL_ERROR;
    }
    statePtr = (UdpState *) Tcl_GetChannelInstanceData(chan);

    if (argc > 2) {
        buffer_size = atoi(argv[2]);
        if (buffer_size > 16) buffer_size = 16;
    }

    memset(message, 0 , sizeof(message));
    actual_size = recvfrom(statePtr->sock, message, buffer_size, MSG_PEEK,
                           (struct sockaddr *)&recvaddr, &socksize);

    if (actual_size < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        Tcl_SetObjResult(interp,  ErrorToObj("udppeek error"));
        return TCL_ERROR;
    }

    if (statePtr->ss_family == AF_INET6) {
        inet_ntop(AF_INET6, &((struct sockaddr_in6*)&recvaddr)->sin6_addr, statePtr->peerhost, sizeof(statePtr->peerhost) );
        statePtr->peerport = ntohs(((struct sockaddr_in6*)&recvaddr)->sin6_port);
    } else {
        inet_ntop(AF_INET, &((struct sockaddr_in*)&recvaddr)->sin_addr, statePtr->peerhost, sizeof(statePtr->peerhost) );
        statePtr->peerport = ntohs(((struct sockaddr_in*)&recvaddr)->sin_port);
    }

    Tcl_AppendResult(interp, message, (char *)NULL);
    return TCL_OK;
#else /* WIN32 */
    Tcl_SetResult(interp, "udp_peek not implemented for this platform",
                  TCL_STATIC);
    return TCL_ERROR;
#endif /* ! WIN32 */
}

#ifdef WIN32
/*
 * ----------------------------------------------------------------------
 * UdpEventProc --
 *
 *  Raise an event from the UDP read thread to notify the Tcl interpreter
 *  that something has happened.
 *
 * ----------------------------------------------------------------------
 */
int
UdpEventProc(Tcl_Event *evPtr, int flags)
{
    UdpEvent *eventPtr = (UdpEvent *) evPtr;
    int mask = 0;

    mask |= TCL_READABLE;
    UDPTRACE("UdpEventProc\n");
    Tcl_NotifyChannel(eventPtr->chan, mask);
    return 1;
}

/*
 * ----------------------------------------------------------------------
 * UDP_SetupProc - called in Tcl_SetEventSource to do the setup step
 * ----------------------------------------------------------------------
 */
static void
UDP_SetupProc(ClientData data, int flags)
{
    UdpState *statePtr;
    Tcl_Time blockTime = { 0, 0 };

    /* UDPTRACE("setupProc\n"); */

    if (!(flags & TCL_FILE_EVENTS)) {
        return;
    }

    WaitForSingleObject(sockListLock, INFINITE);
    for (statePtr = sockList; statePtr != NULL; statePtr=statePtr->next) {
        if (statePtr->packetNum > 0) {
            UDPTRACE("UDP_SetupProc\n");
            Tcl_SetMaxBlockTime(&blockTime);
            break;
        }
    }
    SetEvent(sockListLock);
}

/*
 * ----------------------------------------------------------------------
 * UDP_CheckProc --
 * ----------------------------------------------------------------------
 */
void
UDP_CheckProc(ClientData data, int flags)
{
    UdpState *statePtr;
    UdpEvent *evPtr;
    int actual_size;
    socklen_t socksize;
    int buffer_size = MAXBUFFERSIZE;
    char *message;
    struct sockaddr_storage recvaddr;
    PacketList *p;
#ifdef WIN32
    char hostaddr[256];
    char* portaddr;
    char remoteaddr[256];
      int remoteaddrlen = sizeof(remoteaddr);
    memset(hostaddr, 0 , sizeof(hostaddr));
    memset(remoteaddr,0,sizeof(remoteaddr));
#endif /*  WIN32 */

    /* UDPTRACE("checkProc\n"); */

    /* synchronized */
    WaitForSingleObject(sockListLock, INFINITE);

    for (statePtr = sockList; statePtr != NULL; statePtr=statePtr->next) {
        if (statePtr->packetNum > 0) {
            UDPTRACE("UDP_CheckProc\n");
            /* Read the data from socket and put it into statePtr */
            socksize = sizeof(recvaddr);
            memset(&recvaddr, 0, socksize);

            message = (char *)ckalloc(MAXBUFFERSIZE);
            if (message == NULL) {
                UDPTRACE("ckalloc error\n");
                exit(1);
            }
            memset(message, 0, MAXBUFFERSIZE);

            actual_size = recvfrom(statePtr->sock, message, buffer_size, 0,
                                   (struct sockaddr *)&recvaddr, &socksize);
            SetEvent(waitSockRead);

            if (actual_size < 0) {
                UDPTRACE("UDP error - recvfrom %d\n", statePtr->sock);
                ckfree(message);
            } else {
                p = (PacketList *)ckalloc(sizeof(struct PacketList));
                p->message = message;
                p->actual_size = actual_size;
#ifdef WIN32
                /*
                 * In windows, do not use getnameinfo() since this function does
                 * not work correctly in case of multithreaded. Also inet_ntop() is
                 * not available in older windows versions.
                 */
                if (WSAAddressToString((struct sockaddr *)&recvaddr,socksize,
                    NULL,remoteaddr,&remoteaddrlen)==0) {
                    /*
                     * We now have an address in the format of <ip address>:<port>
                     * Search backwards for the last ':'
                     */
                    portaddr = strrchr(remoteaddr,':') + 1;
                    strncpy(hostaddr,remoteaddr,strlen(remoteaddr)-strlen(portaddr)-1);
                    statePtr->peerport = atoi(portaddr);
                    p->r_port = statePtr->peerport;
                    strcpy(statePtr->peerhost,hostaddr);
                    strcpy(p->r_host,hostaddr);
                }
#else
                if (statePtr->ss_family == AF_INET ) {
                    inet_ntop(AF_INET, ((struct sockaddr_in*)&recvaddr)->sin_addr, statePtr->peerhost, sizeof(statePtr->peerhost) );
                    inet_ntop(AF_INET, ((struct sockaddr_in*)&recvaddr)->sin_addr, p->r_host, sizeof(p->r_host) );
                       p->r_port = ntohs(((struct sockaddr_in*)&recvaddr)->sin_port);
                    statePtr->peerport = ntohs(((struct sockaddr_in*)&recvaddr)->sin_port);
                } else {
                    inet_ntop(AF_INET6, ((struct sockaddr_in6*)&recvaddr)->sin6_addr, statePtr->peerhost, sizeof(statePtr->peerhost) );
                    inet_ntop(AF_INET6, ((struct sockaddr_in6*)&recvaddr)->sin6_addr, p->r_host, sizeof(p->r_host) );
                      p->r_port = ntohs(((struct sockaddr_in6*)&recvaddr)->sin6_port);
                    statePtr->peerport = ntohs(((struct sockaddr_in6*)&recvaddr)->sin6_port);
                }
#endif /*  WIN32 */

                p->next = NULL;

                if (statePtr->packets == NULL) {
                    statePtr->packets = p;
                    statePtr->packetsTail = p;
                } else {
                    statePtr->packetsTail->next = p;
                    statePtr->packetsTail = p;
                }

                UDPTRACE("Received %d bytes from %s:%d through %d\n",
                         p->actual_size, p->r_host, p->r_port, statePtr->sock);
                UDPTRACE("%s\n", p->message);
            }

            statePtr->packetNum--;
            statePtr->doread = 1;
            UDPTRACE("packetNum is %d\n", statePtr->packetNum);

            if (actual_size >= 0) {
                evPtr = (UdpEvent *) ckalloc(sizeof(UdpEvent));
                evPtr->header.proc = UdpEventProc;
                evPtr->chan = statePtr->channel;
                Tcl_QueueEvent((Tcl_Event *) evPtr, TCL_QUEUE_TAIL);
                UDPTRACE("socket %d has data\n", statePtr->sock);
            }
        }
    }

    SetEvent(sockListLock);
}

/*
 * ----------------------------------------------------------------------
 * InitSockets
 * ----------------------------------------------------------------------
 */
static int
InitSockets()
{
    WSADATA wsaData;

    /*
     * Load the socket DLL and initialize the function table.
     */

    if (WSAStartup(0x0101, &wsaData))
        return 0;

    return 1;
}

/*
 * ----------------------------------------------------------------------
 * SocketThread
 * ----------------------------------------------------------------------
 */
static DWORD WINAPI
SocketThread(LPVOID arg)
{
    fd_set readfds; /* variable used for select */
    struct timeval timeout;
    UdpState *statePtr;
    int found;
    int sockset;

    FD_ZERO(&readfds);

    UDPTRACE("In socket thread\n");

    while (1) {
        FD_ZERO(&readfds);
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;
        /* synchronized */
        WaitForSingleObject(sockListLock, INFINITE);

        /* no socket, just wait, use event */
        if (sockList == NULL) {
            SetEvent(sockListLock);
            UDPTRACE("Wait for adding socket\n");
            WaitForSingleObject(waitForSock, INFINITE);
            /* synchronized */
            WaitForSingleObject(sockListLock, INFINITE);
        }

        /* set each socket for select */
        for (statePtr = sockList; statePtr != NULL; statePtr=statePtr->next) {
            FD_SET((unsigned int)statePtr->sock, &readfds);
            UDPTRACE("SET sock %d\n", statePtr->sock);
        }

        SetEvent(sockListLock);
        UDPTRACE("Wait for select\n");
        /* block here */
        found = select(0, &readfds, NULL, NULL, &timeout);
        UDPTRACE("select end\n");

        if (found <= 0) {
            /* We closed the socket during select or time out */
            continue;
        }

        UDPTRACE("Packet comes in\n");

        WaitForSingleObject(sockListLock, INFINITE);
        sockset = 0;
        for (statePtr = sockList; statePtr != NULL; statePtr=statePtr->next) {
            if (FD_ISSET(statePtr->sock, &readfds)) {
                statePtr->packetNum++;
                sockset++;
                UDPTRACE("sock %d is set\n", statePtr->sock);
                break;
            }
        }
        SetEvent(sockListLock);

        /* wait for the socket data was read */
        if (sockset > 0) {
            UDPTRACE( "Wait sock read\n");
            /* alert the thread to do event checking */
            Tcl_ThreadAlert(statePtr->threadId);
            WaitForSingleObject(waitSockRead, INFINITE);
            UDPTRACE("Sock read finished\n");
        }
    }
}

/*
 * ----------------------------------------------------------------------
 * Udp_WinHasSockets --
 * ----------------------------------------------------------------------
 */
int
Udp_WinHasSockets(Tcl_Interp *interp)
{
    static int initialized = 0; /* 1 if the socket sys has been initialized. */
    static int hasSockets = 0;  /* 1 if the system supports sockets. */
    HANDLE socketThread;
    DWORD id;

    if (!initialized) {
        OSVERSIONINFO info;

        initialized = 1;

        /*
         * Find out if we're running on Win32s.
         */

        info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&info);

        /*
         * Check to see if Sockets are supported on this system.  Since
         * win32s panics if we call WSAStartup on a system that doesn't
         * have winsock.dll, we need to look for it on the system first.
         * If we find winsock, then load the library and initialize the
         * stub table.
         */

        if ((info.dwPlatformId != VER_PLATFORM_WIN32s)
            || (SearchPath(NULL, "WINSOCK", ".DLL", 0, NULL, NULL) != 0)) {
            hasSockets = InitSockets();
        }

        /*
         * Start the socketThread window and set the thread priority of the
         * socketThread as highest
         */

        sockList = NULL;
        waitForSock = CreateEvent(NULL, FALSE, FALSE, NULL);
        waitSockRead = CreateEvent(NULL, FALSE, FALSE, NULL);
        sockListLock = CreateEvent(NULL, FALSE, TRUE, NULL);

        socketThread = CreateThread(NULL, 8000, SocketThread, NULL, 0, &id);
        SetThreadPriority(socketThread, THREAD_PRIORITY_HIGHEST);

        UDPTRACE("Initialize socket thread\n");

        if (socketThread == NULL) {
            UDPTRACE("Failed to create thread\n");
        }
    }
    if (hasSockets) {
        return TCL_OK;
    }
    if (interp != NULL) {
        Tcl_AppendResult(interp, "sockets are not available on this system",
                         NULL);
    }
    return TCL_ERROR;
}

#endif /* ! WIN32 */

/*
 * ----------------------------------------------------------------------
 * udpClose --
 *  Called from the channel driver code to cleanup and close
 *  the socket.
 *
 * Results:
 *  0 if successful, the value of errno if failed.
 *
 * Side effects:
 *  The socket is closed.
 *
 * ----------------------------------------------------------------------
 */
static int
udpClose(ClientData instanceData, Tcl_Interp *interp)
{
    int sock;
    int errorCode = 0;
    int objc;
    Tcl_Obj **objv;
    UdpState *statePtr = (UdpState *) instanceData;
#ifdef WIN32
    UdpState *tmp, *p;

    WaitForSingleObject(sockListLock, INFINITE);
#endif /* ! WIN32 */

    sock = statePtr->sock;

#ifdef WIN32

    /* remove the statePtr from the list */
    for (tmp = p = sockList; p != NULL; tmp = p, p = p->next) {
        if (p->sock == sock) {
            UDPTRACE("Remove %d from the list\n", p->sock);
            if (p == sockList) {
                sockList = sockList->next;
            } else {
                tmp->next = p->next;
            }
        }
    }

#endif /* ! WIN32 */

    /*
    * If there are multicast groups added they should be dropped.
    */
    if (statePtr->groupsObj) {
        int n = 0;
        Tcl_Obj *dupGroupList = Tcl_DuplicateObj(statePtr->groupsObj);
        Tcl_IncrRefCount(dupGroupList);
        Tcl_ListObjGetElements(interp, dupGroupList, &objc, &objv);
        for (n = 0; n < objc; n++) {
            if (statePtr->ss_family==AF_INET) {
                UdpMulticast(statePtr, interp,
                    Tcl_GetString(objv[n]), IP_DROP_MEMBERSHIP);
            } else {
                UdpMulticast(statePtr, interp,
                    Tcl_GetString(objv[n]), IPV6_LEAVE_GROUP);
            }
        }
        Tcl_DecrRefCount(dupGroupList);
        Tcl_DecrRefCount(statePtr->groupsObj);
    }

    /* No - doing this causes a infinite recursion. Let Tcl handle this.
    *   Tcl_UnregisterChannel(interp, statePtr->channel);
    */
    if (closesocket(sock) < 0) {
        errorCode = errno;
    }
    ckfree((char *) statePtr);
    if (errorCode != 0) {
#ifndef WIN32
        sprintf(errBuf, "udp_close: %d, error: %d\n", sock, errorCode);
#else
        sprintf(errBuf, "udp_cose: %d, error: %d\n", sock, WSAGetLastError());
#endif
        UDPTRACE("UDP error - close %d", sock);
    } else {
        UDPTRACE("Close socket %d\n", sock);
    }

#ifdef WIN32
    SetEvent(sockListLock);
#endif

    return errorCode;
}

/*
 * ----------------------------------------------------------------------
 * udpWatch --
 * ----------------------------------------------------------------------
 */
static void
udpWatch(ClientData instanceData, int mask)
{
#ifndef WIN32
    UdpState *fsPtr = (UdpState *) instanceData;
    if (mask) {
        UDPTRACE("Tcl_CreateFileHandler\n");
        Tcl_CreateFileHandler(fsPtr->sock, mask,
                              (Tcl_FileProc *) Tcl_NotifyChannel,
                              (ClientData) fsPtr->channel);
    } else {
        UDPTRACE("Tcl_DeleteFileHandler\n");
        Tcl_DeleteFileHandler(fsPtr->sock);
    }
#endif
}

/*
 * ----------------------------------------------------------------------
 * udpGetHandle --
 *   Called from the channel driver to get a handle to the socket.
 *
 * Results:
 *   Puts the socket into handlePtr and returns TCL_OK;
 *
 * Side Effects:
 *   None
 * ----------------------------------------------------------------------
 */
static int
udpGetHandle(ClientData instanceData, int direction, ClientData *handlePtr)
{
    UdpState *statePtr = (UdpState *) instanceData;
    UDPTRACE("udpGetHandle %ld\n", (long)statePtr->sock);
#ifndef WIN32
    *handlePtr = (ClientData) (intptr_t) statePtr->sock;
#else
    *handlePtr = (ClientData) statePtr->sock;
#endif
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 * udpOutput--
 * ----------------------------------------------------------------------
 */
static int
udpOutput(ClientData instanceData, CONST84 char *buf, int toWrite, int *errorCode)
{
    UdpState *statePtr = (UdpState *) instanceData;
    int written;
    int socksize;
    struct hostent *name;
    struct sockaddr_in sendaddrv4;
    struct sockaddr_in6 sendaddrv6;
    struct addrinfo hints, *result;

    if (toWrite > MAXBUFFERSIZE) {
        UDPTRACE("UDP error - MAXBUFFERSIZE");
        return -1;
    }

     if (statePtr->ss_family == AF_INET6) {
        socksize = sizeof(sendaddrv6);
        memset(&sendaddrv6, 0, socksize);
        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET6;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        if (getaddrinfo(statePtr->remotehost, NULL, &hints, &result) != 0) {
                UDPTRACE("UDP error - getaddrinfo failed");
                return -1;
        }
        memcpy (&sendaddrv6, result->ai_addr, result->ai_addrlen);
        freeaddrinfo(result);

        sendaddrv6.sin6_family = AF_INET6;
        sendaddrv6.sin6_port = statePtr->remoteport;

        written = sendto(statePtr->sock, buf, toWrite, 0, (struct sockaddr *)&sendaddrv6, socksize);
    } else {
        socksize = sizeof(sendaddrv4);
        memset(&sendaddrv4, 0, socksize);
        sendaddrv4.sin_addr.s_addr = inet_addr(statePtr->remotehost);

        if (sendaddrv4.sin_addr.s_addr == -1) {
            name = gethostbyname(statePtr->remotehost);
            if (name == NULL) {
                UDPTRACE("UDP error - gethostbyname");
                return -1;
            }
            memcpy(&sendaddrv4.sin_addr, name->h_addr, sizeof(sendaddrv4.sin_addr));
        }

        sendaddrv4.sin_family = AF_INET;
        sendaddrv4.sin_port = statePtr->remoteport;

        written = sendto(statePtr->sock, buf, toWrite, 0, (struct sockaddr *)&sendaddrv4, socksize);
    }

    if (written < 0) {
        UDPTRACE("UDP error - sendto");
        return -1;
    }

    UDPTRACE("Send %d to %s:%d through %d\n", written, statePtr->remotehost,
             ntohs(statePtr->remoteport), statePtr->sock);

    return written;
}

/*
 * ----------------------------------------------------------------------
 * udpInput
 * ----------------------------------------------------------------------
 */
static int
udpInput(ClientData instanceData, char *buf, int bufSize, int *errorCode)
{
    UdpState *statePtr = (UdpState *) instanceData;
    int bytesRead;

#ifdef WIN32
    PacketList *packets;
#else /* ! WIN32 */
    socklen_t socksize;
    int buffer_size = MAXBUFFERSIZE;
    int sock = statePtr->sock;
    struct sockaddr_storage recvaddr;
#endif /* ! WIN32 */

    UDPTRACE("In udpInput\n");

    /*
     * The caller of this function is looking for a stream oriented
     * system, so it keeps calling the function until no bytes are
     * returned, and then appends all the characters together.  This
     * is not what we want from UDP, so we fake it by returning a
     * blank every other call.  whenever the doread variable is 1 do
     * a normal read, otherwise just return -1 to indicate that we want
     * to receive data again.
     */
    if (statePtr->doread == 0) {
        statePtr->doread = 1;  /* next time we want to behave normally */
        *errorCode = EAGAIN;   /* pretend that we would block */
        UDPTRACE("Pretend we would block\n");
        return -1;
    }

    *errorCode = 0;
    errno = 0;

    if (bufSize == 0) {
        return 0;
    }

#ifdef WIN32
    packets = statePtr->packets;
    UDPTRACE("udp_recv\n");

    if (packets == NULL) {
        UDPTRACE("packets is NULL\n");
        *errorCode = EAGAIN;
        return -1;
    }
    memcpy(buf, packets->message, packets->actual_size);
    ckfree((char *) packets->message);
    UDPTRACE("udp_recv message\n%s", buf);
    bufSize = packets->actual_size;
    strcpy(statePtr->peerhost, packets->r_host);
    statePtr->peerport = packets->r_port;
    statePtr->packets = packets->next;
    ckfree((char *) packets);
    bytesRead = bufSize;
#else /* ! WIN32 */
    socksize = sizeof(recvaddr);
    memset(&recvaddr, 0, socksize);

    bytesRead = recvfrom(sock, buf, buffer_size, 0,
                         (struct sockaddr *)&recvaddr, &socksize);
    if (bytesRead < 0) {
        UDPTRACE("UDP error - recvfrom %d\n", sock);
        *errorCode = errno;
        return -1;
    }

    if (statePtr->ss_family == AF_INET6) {
        inet_ntop(AF_INET6, &((struct sockaddr_in6*)&recvaddr)->sin6_addr, statePtr->peerhost, sizeof(statePtr->peerhost) );
        statePtr->peerport = ntohs(((struct sockaddr_in6*)&recvaddr)->sin6_port);
    } else {
        inet_ntop(AF_INET, &((struct sockaddr_in*)&recvaddr)->sin_addr, statePtr->peerhost, sizeof(statePtr->peerhost) );
        statePtr->peerport = ntohs(((struct sockaddr_in*)&recvaddr)->sin_port);
    }

    UDPTRACE("remotehost: %s:%d\n", statePtr->peerhost, statePtr->peerport);
#endif /* ! WIN32 */

    /* we don't want to return anything next time */
    if (bytesRead > 0) {
        buf[bytesRead] = '\0';
        statePtr->doread = 0;
    }

    UDPTRACE("udpInput end: %d, %s\n", bytesRead, buf);

    if (bytesRead == 0) {
        *errorCode = EAGAIN;
        return -1;
    }
    if (bytesRead > -1) {
        return bytesRead;
    }

    *errorCode = errno;
    return -1;
}

/* ----------------------------------------------------------------------
 *
 * LSearch --
 *
 *     Find a string item in a list and return the index of -1.
 */

static int
LSearch(Tcl_Obj *listObj, const char *group)
{
    int objc, n;
    Tcl_Obj **objv;
    Tcl_ListObjGetElements(NULL, listObj, &objc, &objv);
    for (n = 0; n < objc; n++) {
    if (strcmp(group, Tcl_GetString(objv[n])) == 0) {
        return n;
    }
    }
    return -1;
}

/*
 * ----------------------------------------------------------------------
 *
 * UdpMulticast --
 *
 *    Action should be IP_ADD_MEMBERSHIP | IPV6_JOIN_GROUP
 *  or IP_DROP_MEMBERSHIP | IPV6_LEAVE_GROUP
 *
 */

static int
UdpMulticast(UdpState *statePtr, Tcl_Interp *interp,
    const char *grp, int action)
{
    int r;
    Tcl_Obj *tcllist , *multicastgrp , *nw_interface;
    int len,result;
    int nwinterface_index =-1;
#ifndef WIN32
    struct ifreq ifreq;
#endif /* ! WIN32 */

    /*
     * Parameter 'grp' can be:
     *  Windows: <multicast group> or {<multicast group> <network interface index>}
     *  Not Windows: <multicast group> or {<multicast group> <network interface name>}
     */
    tcllist = Tcl_NewStringObj(grp, -1);
    result = Tcl_ListObjLength(interp, tcllist, &len);
    if (result == TCL_OK) {
        if (len==2) {
            Tcl_ListObjIndex(interp, tcllist, 0, &multicastgrp);
            Tcl_ListObjIndex(interp, tcllist, 1, &nw_interface);
#ifdef WIN32
            if ( Tcl_GetIntFromObj(interp,nw_interface,&nwinterface_index) == TCL_ERROR && nwinterface_index < 1) {
                Tcl_SetResult(interp, "not a valid network interface index; should start with 1", TCL_STATIC);
                return TCL_ERROR;
            }
#else
            int lenPtr = -1;
            if (nw_interface->length > IFNAMSIZ ) {
                Tcl_SetResult(interp, "unknown network interface", TCL_STATIC);
                return TCL_ERROR;
            }

            if (statePtr->ss_family == AF_INET) {
                /* For IPv4, we need the network interface address. */
                strcpy(ifreq.ifr_name,Tcl_GetStringFromObj(nw_interface,&lenPtr));
                if (ioctl(statePtr->sock, SIOCGIFADDR, &ifreq) < 0 ) {
                    Tcl_SetResult(interp, "unknown network interface", TCL_STATIC);
                    return TCL_ERROR;
                }
            }
            nwinterface_index = if_nametoindex(Tcl_GetStringFromObj(nw_interface,&lenPtr));
            if (nwinterface_index == 0 ) {
                Tcl_SetResult(interp, "unknown network interface", TCL_STATIC);
                return TCL_ERROR;
            }
#endif /* ! WIN32 */
        } else if (len==1) {
            Tcl_ListObjIndex(interp, tcllist, 0, &multicastgrp);
        } else {
            Tcl_SetResult(interp, "multicast group and/or local network interface not specified", TCL_STATIC);
            return TCL_ERROR;
        }
    }

    if (statePtr->ss_family == AF_INET) {
        struct ip_mreq mreq;
        struct hostent *name;

        memset(&mreq, 0, sizeof(mreq));

        mreq.imr_multiaddr.s_addr = inet_addr(Tcl_GetString(multicastgrp));
        if (mreq.imr_multiaddr.s_addr == -1) {
            name = gethostbyname(Tcl_GetString(multicastgrp));
            if (name == NULL) {
                if (interp != NULL) {
                    Tcl_SetResult(interp, "invalid group name", TCL_STATIC);
                }
                return TCL_ERROR;
            }
            memcpy(&mreq.imr_multiaddr.s_addr, name->h_addr, sizeof(mreq.imr_multiaddr));
        }

        if (nwinterface_index==-1) {
            /* No interface index specified. Let the system use the default interface. */
            mreq.imr_interface.s_addr = INADDR_ANY;
        } else {
#ifdef WIN32
            /* Using an interface index of x is indicated by 0.0.0.x */
            mreq.imr_interface.s_addr = htonl(nwinterface_index);
#else
            memcpy(&mreq.imr_interface, &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr, sizeof(struct in_addr));
#endif
        }

        if (setsockopt(statePtr->sock, IPPROTO_IP, action, (const char*)&mreq, sizeof(mreq)) < 0) {
            if (interp != NULL) {
                Tcl_SetObjResult(interp, ErrorToObj("error changing multicast group"));
            }
            return TCL_ERROR;
        }
    } else {
        struct ipv6_mreq mreq6;
        struct addrinfo hints;
        struct addrinfo *result = NULL;

        memset(&hints, 0, sizeof(hints));

        hints.ai_family = statePtr->ss_family;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        r = getaddrinfo(Tcl_GetString(multicastgrp), NULL, &hints, &result);

        if (r != 0 ) {
            Tcl_SetResult(interp, "invalid group name", TCL_STATIC);
            freeaddrinfo(result);
            return TCL_ERROR;
        } else {
            memcpy(&mreq6.ipv6mr_multiaddr, &((struct sockaddr_in6*)(result->ai_addr))->sin6_addr,sizeof(mreq6.ipv6mr_multiaddr));
            freeaddrinfo(result);
        }

        if (nwinterface_index == -1) {
            /* Let the system choose the default multicast network interface. */
            mreq6.ipv6mr_interface = 0;
         } else {
            /* Use the specified network interface. */
             mreq6.ipv6mr_interface = nwinterface_index;
        }

        if (setsockopt(statePtr->sock, IPPROTO_IPV6, action, (const char*)&mreq6, sizeof(mreq6)) < 0) {
            if (interp != NULL) {
                Tcl_SetObjResult(interp, ErrorToObj("error changing multicast group"));
            }
            return TCL_ERROR;
        }
    }

    if (action == IP_ADD_MEMBERSHIP || action == IPV6_JOIN_GROUP) {
        int ndx = LSearch(statePtr->groupsObj, grp);
        if (ndx == -1) {
            Tcl_Obj *newPtr;
            statePtr->multicast++;
            if (Tcl_IsShared(statePtr->groupsObj)) {
                newPtr = Tcl_DuplicateObj(statePtr->groupsObj);
                Tcl_DecrRefCount(statePtr->groupsObj);
                Tcl_IncrRefCount(newPtr);
                statePtr->groupsObj = newPtr;
            }
            Tcl_ListObjAppendElement(interp, statePtr->groupsObj,
                Tcl_NewStringObj(grp,-1));
        }
    } else {
        int ndx = LSearch(statePtr->groupsObj, grp);
        if (ndx != -1) {
            Tcl_Obj *old, *ptr;
            int dup = 0;
            old = ptr = statePtr->groupsObj;
            statePtr->multicast--;
            if ((dup = Tcl_IsShared(ptr))) {
                ptr = Tcl_DuplicateObj(ptr);
            }
            Tcl_ListObjReplace(interp, ptr, ndx, 1, 0, NULL);
            if (dup) {
                statePtr->groupsObj = ptr;
                Tcl_IncrRefCount(ptr);
                Tcl_DecrRefCount(old);
            }
        }
    }
    if (interp != NULL)
        Tcl_SetObjResult(interp, statePtr->groupsObj);
    return TCL_OK;
}

/*
* ----------------------------------------------------------------------
* udpGetOption --
* ----------------------------------------------------------------------
*/
static int
udpGetOption(ClientData instanceData, Tcl_Interp *interp,
             CONST84 char *optionName, Tcl_DString *optionValue)
{
    UdpState *statePtr = (UdpState *)instanceData;
    CONST84 char * options[] = { "myport", "remote", "peer", "mcastgroups", "mcastloop", "broadcast", "ttl", NULL};
    int r = TCL_OK;

    if (optionName == NULL) {
        Tcl_DString ds;
        const char **p;

        Tcl_DStringInit(&ds);
        for (p = options; *p != NULL; p++) {
            char op[16];
            sprintf(op, "-%s", *p);
            Tcl_DStringSetLength(&ds, 0);
            udpGetOption(instanceData, interp, op, &ds);
            Tcl_DStringAppend(optionValue, " ", 1);
            Tcl_DStringAppend(optionValue, op, -1);
            Tcl_DStringAppend(optionValue, " ", 1);
            Tcl_DStringAppendElement(optionValue, Tcl_DStringValue(&ds));
        }

    } else {

        Tcl_DString ds, dsInt;
        Tcl_DStringInit(&ds);
        Tcl_DStringInit(&dsInt);

        if (!strcmp("-myport", optionName)) {

            Tcl_DStringSetLength(&ds, TCL_INTEGER_SPACE);
            sprintf(Tcl_DStringValue(&ds), "%u", ntohs(statePtr->localport));

        } else if (!strcmp("-remote", optionName)) {
            if (statePtr->remotehost && *statePtr->remotehost) {
                Tcl_DStringSetLength(&dsInt, TCL_INTEGER_SPACE);
                sprintf(Tcl_DStringValue(&dsInt), "%u",
                    ntohs(statePtr->remoteport));
                Tcl_DStringAppendElement(&ds, statePtr->remotehost);
                Tcl_DStringAppendElement(&ds, Tcl_DStringValue(&dsInt));
            }

        } else if (!strcmp("-peer", optionName)) {

           if (statePtr->peerhost && *statePtr->peerhost) {
                 Tcl_DStringSetLength(&dsInt, TCL_INTEGER_SPACE);
                sprintf(Tcl_DStringValue(&dsInt), "%u", statePtr->peerport);
                Tcl_DStringAppendElement(&ds, statePtr->peerhost);
                Tcl_DStringAppendElement(&ds, Tcl_DStringValue(&dsInt));
           }

        } else if (!strcmp("-mcastgroups", optionName)) {

            int objc, n;
            Tcl_Obj **objv;
            Tcl_ListObjGetElements(interp, statePtr->groupsObj, &objc, &objv);
            for (n = 0; n < objc; n++) {
                Tcl_DStringAppendElement(&ds, Tcl_GetString(objv[n]));
            }

        } else if (!strcmp("-broadcast", optionName)) {
            int tmp =1;
            r = udpGetBroadcastOption(statePtr,interp,&tmp);
            if (r==TCL_OK) {
                Tcl_DStringSetLength(&ds, TCL_INTEGER_SPACE);
                sprintf(Tcl_DStringValue(&ds), "%d", tmp);
            }
        } else if (!strcmp("-mcastloop", optionName)) {
            unsigned char tmp = 0;
            r = udpGetMcastloopOption(statePtr, interp,&tmp);
            if (r==TCL_OK) {
                Tcl_DStringSetLength(&ds, TCL_INTEGER_SPACE);
                sprintf(Tcl_DStringValue(&ds), "%d", (int)tmp);
            }
        } else if (!strcmp("-ttl", optionName)) {

            unsigned int tmp = 0;
            r = udpGetTtlOption(statePtr,interp,&tmp);
            if (r==TCL_OK) {
                Tcl_DStringSetLength(&ds, TCL_INTEGER_SPACE);
                sprintf(Tcl_DStringValue(&ds), "%u", tmp);
            }
        } else {
            CONST84 char **p;
            Tcl_DString tmp;
            Tcl_DStringInit(&tmp);
            for (p = options; *p != NULL; p++)
                Tcl_DStringAppendElement(&tmp, *p);
            r = Tcl_BadChannelOption(interp, optionName, Tcl_DStringValue(&tmp));
            Tcl_DStringFree(&tmp);
        }

        if (r == TCL_OK) {
            Tcl_DStringAppend(optionValue, Tcl_DStringValue(&ds), -1);
        }
        Tcl_DStringFree(&dsInt);
        Tcl_DStringFree(&ds);
    }

    return r;
}

/*
 * ----------------------------------------------------------------------
 * udpSetOption --
 *
 *  Handle channel configuration requests from the generic layer.
 *
 * ----------------------------------------------------------------------
 */
static int
udpSetOption(ClientData instanceData, Tcl_Interp *interp,
             CONST84 char *optionName, CONST84 char *newValue)
{
    UdpState *statePtr = (UdpState *)instanceData;
    CONST84 char * options = "remote mcastadd mcastdrop mcastloop broadcast ttl";
    int r = TCL_OK;

    if (!strcmp("-remote", optionName)) {
        r = udpSetRemoteOption(statePtr,interp,(const char *)newValue);
    } else if (!strcmp("-mcastadd", optionName)) {
        r = udpSetMulticastAddOption(statePtr, interp, (const char *)newValue);
    } else if (!strcmp("-mcastdrop", optionName)) {
        r = udpSetMulticastDropOption(statePtr, interp, (const char *)newValue);
    } else if (!strcmp("-broadcast", optionName)) {
        r = udpSetBroadcastOption(statePtr, interp, (const char*) newValue);
     } else if (!strcmp("-mcastloop", optionName)) {
        r = udpSetMcastloopOption(statePtr, interp, (const char*) newValue);
     } else if (!strcmp("-ttl", optionName)) {
        r = udpSetTtlOption(statePtr, interp, (const char*) newValue);
    } else {
        Tcl_BadChannelOption(interp, optionName, options);
        r=TCL_ERROR;
    }

    return r;
}

/*
 * ----------------------------------------------------------------------
 * udpGetTtlOption --
 *
 *  Handle ttl configuration requests.
 *
 * ----------------------------------------------------------------------
 */
static int
udpGetTtlOption(UdpState *statePtr, Tcl_Interp *interp,unsigned int *value)
{
    int result = TCL_ERROR;
    int cmd;
    socklen_t optlen = sizeof(unsigned int);

    if (statePtr->ss_family==AF_INET) {
        if (statePtr->multicast > 0) {
            cmd = IP_MULTICAST_TTL;
        } else {
            cmd = IP_TTL;
        }
        result = getsockopt(statePtr->sock, IPPROTO_IP, cmd, (char*)value, &optlen);
    } else {
        if (statePtr->multicast > 0) {
            cmd = IPV6_MULTICAST_HOPS;
        } else {
            cmd = IPV6_UNICAST_HOPS;
        }
        result = getsockopt(statePtr->sock, IPPROTO_IPV6, cmd, (char*)value, &optlen);
    }

    if (result==TCL_ERROR) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error getting -ttl",-1));
    }
    return result;
}

/*
 * ----------------------------------------------------------------------
 * udpSetTtlOption --
 *
 *  Handle ttl configuration requests.
 *
 * ----------------------------------------------------------------------
 */
static int
udpSetTtlOption(UdpState *statePtr, Tcl_Interp *interp,CONST84 char *newValue)
{
    int result = TCL_ERROR;
    int tmp = 0;
    int cmd;
    result = Tcl_GetInt(interp, newValue, &tmp);

    if (statePtr->ss_family==AF_INET) {
        if (statePtr->multicast > 0) {
            cmd = IP_MULTICAST_TTL;
        } else {
            cmd = IP_TTL;
        }
        if (result == TCL_OK) {
            result = setsockopt(statePtr->sock, IPPROTO_IP, cmd,(const char *)&tmp, sizeof(unsigned int));
        }
    } else {
        if (statePtr->multicast > 0) {
            cmd = IPV6_MULTICAST_HOPS;
        } else {
            cmd = IPV6_UNICAST_HOPS;
        }
        if (result == TCL_OK) {
            result = setsockopt(statePtr->sock, IPPROTO_IPV6, cmd,(const char *)&tmp, sizeof(unsigned int));
        }
    }

    if (result==TCL_ERROR) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error setting -ttl",-1));
    } else {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(tmp));
    }
    return result;
}

/*
 * ----------------------------------------------------------------------
 * udpGetMcastloopOption --
 *
 *  Handle multicast loop configuration requests.
 *
 * ----------------------------------------------------------------------
 */
static int
udpGetMcastloopOption(UdpState *statePtr, Tcl_Interp *interp, unsigned char * value)
{
    int result = TCL_ERROR;
    socklen_t optlen=sizeof(int);
    if (statePtr->ss_family == AF_INET) {
        result = getsockopt(statePtr->sock, IPPROTO_IP, IP_MULTICAST_LOOP,value, &optlen);
    } else {
        result = getsockopt(statePtr->sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,value, &optlen);
    }

    if (result == TCL_ERROR) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error getting -mcastloop",-1));
    }

    return result;
}

/*
 * ----------------------------------------------------------------------
 * udpSetMcastloopOption --
 *
 *  Handle multicast loop configuration requests.
 *
 * ----------------------------------------------------------------------
 */
static int
udpSetMcastloopOption(UdpState *statePtr, Tcl_Interp *interp,CONST84 char *newValue)
{
    int result = TCL_ERROR;
    int tmp = 1;
    if (Tcl_GetBoolean(interp, newValue, &tmp)==TCL_OK) {
        if (statePtr->ss_family == AF_INET) {
            result = setsockopt(statePtr->sock, IPPROTO_IP, IP_MULTICAST_LOOP,
            (const char *)&tmp, sizeof(tmp));
        } else {
            result = setsockopt(statePtr->sock, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
            (const char *)&tmp, sizeof(tmp));
        }
    }

    if (result == TCL_ERROR) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error setting -mcastloop",-1));
    } else {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(tmp));
    }


    return result;
}

/*
 * ----------------------------------------------------------------------
 * udpGetBroadcastOption --
 *
 *  Handle broadcast configuration requests.
 *
 * ----------------------------------------------------------------------
 */
static int
udpGetBroadcastOption(UdpState *statePtr, Tcl_Interp *interp, int* value)
{
    int result = TCL_OK;
    socklen_t optlen = sizeof(int);
    if (statePtr->ss_family == AF_INET6 ) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("broadcast not supported under ipv6",-1));
        return TCL_ERROR;
    }
    if (getsockopt(statePtr->sock, SOL_SOCKET, SO_BROADCAST, (char*)value, &optlen)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error getting -broadcast",-1));
        result = TCL_ERROR;
    }

    return result;
}

/*
 * ----------------------------------------------------------------------
 * udpSetBroadcastOption --
 *
 *  Handle broadcast configuration requests.
 *
 * ----------------------------------------------------------------------
 */
static int
udpSetBroadcastOption(UdpState *statePtr, Tcl_Interp *interp,CONST84 char *newValue)
{
    int result;
    int tmp = 1;

    if (statePtr->ss_family == AF_INET6 ) {
            Tcl_SetObjResult(interp, ErrorToObj("broadcast not supported under ipv6"));
            return TCL_ERROR;
    }

    result = Tcl_GetInt(interp, newValue, &tmp);
    if (result == TCL_OK ) {
        if (setsockopt(statePtr->sock, SOL_SOCKET, SO_BROADCAST,
            (const char *)&tmp, sizeof(int))) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("error setting -broadcast",-1));
            result = TCL_ERROR;
        } else {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(tmp));
        }
    }

    return result;
}

/*
 * ----------------------------------------------------------------------
 * udpSetRemoteOption --
 *
 *  Handle remote port/host configuration requests.
 *
 * ----------------------------------------------------------------------
 */
static int
udpSetRemoteOption(UdpState *statePtr, Tcl_Interp *interp,CONST84 char *newValue)
{
    int result;

    Tcl_Obj *valPtr;
    int len;

    valPtr = Tcl_NewStringObj(newValue, -1);
    result = Tcl_ListObjLength(interp, valPtr, &len);
    if (result == TCL_OK) {
        if (len < 1 || len > 2) {
            Tcl_SetResult(interp, "wrong # args", TCL_STATIC);
            result = TCL_ERROR;
        } else {
            Tcl_Obj *hostPtr, *portPtr;

            Tcl_ListObjIndex(interp, valPtr, 0, &hostPtr);
            strcpy(statePtr->remotehost, Tcl_GetString(hostPtr));

            if (len == 2) {
                Tcl_ListObjIndex(interp, valPtr, 1, &portPtr);
                result = udpGetService(interp, Tcl_GetString(portPtr),
                    &(statePtr->remoteport));
            }
        }
    }

    if (result==TCL_ERROR) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error setting -remote",-1));
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(newValue,-1));
    }

    return result;
}

/*
 * ----------------------------------------------------------------------
 * udpSetMulticastAddOption --
 *
 *  Handle multicast add configuration requests.
 *
 * ----------------------------------------------------------------------
 */
static int
udpSetMulticastAddOption(UdpState *statePtr, Tcl_Interp *interp,CONST84 char *newValue)
{
     int result;

    if (statePtr->ss_family == AF_INET) {
        result = UdpMulticast(statePtr, interp,
                (const char *)newValue, IP_ADD_MEMBERSHIP);
    } else {
        result = UdpMulticast(statePtr, interp,
                (const char *)newValue, IPV6_JOIN_GROUP);
    }
    return result;
}

/*
 * ----------------------------------------------------------------------
 * udpSetMulticastDropOption --
 *
 *  Handle multicast drop configuration requests.
 *
 * ----------------------------------------------------------------------
 */
static int
udpSetMulticastDropOption(UdpState *statePtr, Tcl_Interp *interp,CONST84 char *newValue)
{
     int result;

    if (statePtr->ss_family == AF_INET) {
        result = UdpMulticast(statePtr, interp,
                (const char *)newValue, IP_DROP_MEMBERSHIP);
    } else {
        result = UdpMulticast(statePtr, interp,
                (const char *)newValue, IPV6_LEAVE_GROUP);
    }

    if (result==TCL_ERROR) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("error setting -mcastdrop",-1));
    }

    return result;
}

static Tcl_Obj *
ErrorToObj(const char * prefix)
{
    Tcl_Obj *errObj;
#ifdef WIN32
    LPVOID sMsg;
    DWORD len = 0;

    len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
                         | FORMAT_MESSAGE_FROM_SYSTEM
                         | FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL, GetLastError(),
                         MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                         (LPWSTR)&sMsg, 0, NULL);
    errObj = Tcl_NewStringObj(prefix, -1);
    Tcl_AppendToObj(errObj, ": ", -1);
    Tcl_AppendUnicodeToObj(errObj, (LPWSTR)sMsg, len - 1);
    LocalFree(sMsg);
#elif defined(HAVE_STRERROR)
    extern int errno;
    errObj = Tcl_NewStringObj(prefix, -1);
    Tcl_AppendStringsToObj(errObj, ": ", strerror(errno), NULL);
#endif
    return errObj;
}

/*
 * ----------------------------------------------------------------------
 * udpTrace --
 * ----------------------------------------------------------------------
 */
static void
udpTrace(const char *format, ...)
{
    va_list args;

#ifdef WIN32

    static char buffer[1024];
    va_start (args, format);
    _vsnprintf(buffer, 1023, format, args);
    OutputDebugString(buffer);

#else /* ! WIN32 */

    va_start (args, format);
    vfprintf(dbg, format, args);
    fflush(dbg);

#endif /* ! WIN32 */

    va_end(args);
}

/*
 * ----------------------------------------------------------------------
 * udpGetService --
 *
 *  Return the service port number in network byte order from either a
 *  string representation of the port number or the service name. If the
 *  service string cannot be converted (ie: a name not present in the
 *  services database) then set a Tcl error.
 * ----------------------------------------------------------------------
 */

static int
udpGetService(Tcl_Interp *interp, const char *service,
              unsigned short *servicePort)
{
    int port = 0;
    int r = UdpSockGetPort(interp, service, "udp", &port);
    *servicePort = htons((short)port);
    return r;
}

/*
 *---------------------------------------------------------------------------
 *
 * UdpSockGetPort --
 *
 *      Maps from a string, which could be a service name, to a port.
 *      Used by socket creation code to get port numbers and resolve
 *      registered service names to port numbers.
 *
 *      NOTE: this is a copy of TclSockGetPort.
 *
 * Results:
 *      A standard Tcl result.  On success, the port number is returned
 *      in portPtr. On failure, an error message is left in the interp's
 *      result.
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

int
UdpSockGetPort(interp, string, proto, portPtr)
     Tcl_Interp *interp;
     const char *string;         /* Integer or service name */
     const char *proto;          /* "tcp" or "udp", typically */
     int *portPtr;               /* Return port number */
{
    struct servent *sp;          /* Protocol info for named services */
    Tcl_DString ds;
    CONST char *native;

    if (Tcl_GetInt(NULL, string, portPtr) != TCL_OK) {
        /*
         * Don't bother translating 'proto' to native.
         */

        native = Tcl_UtfToExternalDString(NULL, string, -1, &ds);
        sp = getservbyname(native, proto);              /* INTL: Native. */
        Tcl_DStringFree(&ds);
        if (sp != NULL) {
            *portPtr = ntohs((unsigned short) sp->s_port);
            return TCL_OK;
        }
    }
    if (Tcl_GetInt(interp, string, portPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (*portPtr > 0xFFFF) {
        Tcl_AppendResult(interp, "couldn't open socket: port number too high",
                         (char *) NULL);
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * Local variables:
 * mode: c
 * indent-tabs-mode: nil
 * End:
 */
