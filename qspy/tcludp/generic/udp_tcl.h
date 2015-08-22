/*
 *----------------------------------------------------------------------
 * UDP Extension for Tcl 8.4
 *
 * Copyright (c) 1999-2003 by Columbia University; all rights reserved
 * Copyright (c) 2003-2005 Pat Thoyts <patthoyts@users.sourceforge.net>
 *
 * Written by Xiaotao Wu
 *
 * $Id: udp_tcl.h,v 1.13 2014/05/02 14:41:24 huubeikens Exp $
 *----------------------------------------------------------------------
 */

#ifndef UDP_TCL_H
#define UDP_TCL_H

#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#if STDC_HEADERS
#  include <stdlib.h>
#endif

#if defined(_WIN32) && !defined(WIN32)
#define WIN32
#endif

#ifdef WIN32
#  if !defined( _WIN32_WINNT ) || ( _WIN32_WINNT < 0x0501 )
#    undef  _WIN32_WINNT
#    define _WIN32_WINNT 0x0501
#  endif
#  if !defined( WINVER ) || ( WINVER < 0x0501 )
#    undef  WINVER
#    define WINVER 0x0501
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  if HAVE_UNISTD_H
#    include <unistd.h>
#  endif
#  if HAVE_SYS_TIME_H
#    include <sys/time.h>
#  endif
#  if HAVE_STDINT_H
#    include <stdint.h>
#  endif
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <net/if.h>
#endif /* WIN32 */

#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include "tcl.h"

#ifdef BUILD_udp
#undef TCL_STORAGE_CLASS
#define TCL_STORAGE_CLASS DLLEXPORT
#endif /* BUILD_udp */

#ifdef WIN32

typedef u_short uint16_t;

typedef struct {
  Tcl_Event         header;     /* Information that is standard for */
  Tcl_Channel       chan;       /* Socket descriptor that is ready  */
} UdpEvent;

typedef struct PacketList {
  char              *message;
  int               actual_size;
  char              r_host[256];
  int               r_port;
  struct PacketList *next;
} PacketList;

#endif /* WIN32 */

typedef struct UdpState {
  Tcl_Channel       channel;
  int               sock;
  char              remotehost[256]; /* send packets to */
  uint16_t          remoteport;
  char              peerhost[256];   /* receive packets from */
  uint16_t          peerport;
  uint16_t          localport;
  int               doread;
#ifdef WIN32
  HWND              hwnd;
  PacketList        *packets;
  PacketList        *packetsTail;
  int               packetNum;
  struct UdpState   *next;
  Tcl_ThreadId      threadId;        /* for Tcl_ThreadAlert */
#endif
  short                ss_family;         /* indicator set for ipv4 or ipv6 usage */
  int               multicast;       /* indicator set for multicast add */
  Tcl_Obj          *groupsObj;       /* list of the mcast groups */
} UdpState;


EXTERN int Udp_Init(Tcl_Interp *interp);
EXTERN int Udp_SafeInit(Tcl_Interp *interp);

#endif
