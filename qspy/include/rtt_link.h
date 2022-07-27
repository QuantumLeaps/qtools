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
* @date Last updated on: 2022-07-27
* @version Last updated for version: 7.0.0
*
* @file
* @brief RTT via J-Link for QSPY tool
* @ingroup qpspy
*/
#ifndef RTT_LINK_H
#define RTT_LINK_H

#ifndef RTT_QSPY_BUFFER_NAME
#define RTT_QSPY_BUFFER_NAME "qspy"
#endif /* RTT_QSPY_BUFFER_NAME */

/**
 * Data about J-Link + fd for connection to the 'main' thread of QSPY
 */
typedef struct {
	char const *device;
	uint32_t serNo;
	int fdT2H[2];
	int fdH2T[2];	
} rtt_link_worker_arg_t;

/**
 * Some enums and structs related to J-Link's  
 */
typedef enum {
	HOST_IF_USB = 1, HOST_IF_IP = 2,
} enJLINKARM_HOST_IF;

typedef enum {
	RTT_CMD_START,
	RTT_CMD_STOP,
	RTT_CMD_GETDESC,
	RTT_CMD_GETNUMBUF,
	RTT_CMD_GETSTAT,
} enJLINKARM_RTT_CMD;

typedef enum {
	RTT_DIR_T2H, RTT_DIR_H2T
} enJLINKARM_RTT_DIR;

typedef struct {
	uint32_t serial_number;
	uint32_t host_if; /* enJLINKARM_HOST_IF as discriminant */
	uint8_t placeholder[256]; /* Up to total size of 0x108 Bytes */
	/*
	 * Remark (07/2022 by JH):
	 *   placeholder[] Bytes definitely means something (at least some of them
	 *   at the start of the placeholder array), but it seems to me now, that
	 *   their meaning is irrelevant for the RTT_LINK.
	 *   (In fact, they even mean something different for different HOST_IFs.)
	 */
} jlink_info_t;

typedef struct {
	uint32_t idx;
	uint32_t dir; /* enJLINKARM_RTT_DIR */
	char acName[32];
	uint32_t size;
	uint32_t mode;
} jlink_rtt_desc_t;

/**
 * Representation of a shared library for J-Links (only exports used)
 */
typedef struct {
	void *lib;
	char const *lib_name;
	//
	char const* (*pfnOpen)(void); /*  ok ... NULL */
	void (*pfnClose)(void);
	int (*pfnGetList)(int, void* const, int); /* ok ... >=0 (num of JLinks) */
	int (*pfnSelectByUSBSN)(uint32_t); /* ok ... >=0 */
	void (*pfnSelectByIPSN)(uint32_t);
	int (*pfnExecCommand)(char const*, char const*, int); /* ok ... 2.param[0] == '\0' */
	void (*pfnTIF_GetAvailable)(uint32_t*);
	int (*pfnTIF_Select)(int); /* ok ... ??? */
	void (*pfnSetMaxSpeed)(void);
	int (*pfnConnect)(void); /* ok ... >=0 */
	int (*pfnGetNumConnections)(void); /* ok ... >=0 */
	char (*pfnIsHalted)(void); /* ok ... >=0 */
	void (*pfnGo)(void);
	//
	int (*pfnRTT_Control)(uint32_t, void*); /* ok ... >=0 */
	int (*pfnRTT_Read)(uint32_t, void* const, uint32_t); /* ok ... >=0 */
	int (*pfnRTT_Write)(uint32_t, void const* const, uint32_t); /* ok ... >=0 */
} lib_exports_t;

#endif /* RTT_LINK_H */

/**
 * Exported function (to be run in parallel to QSPY)
 * @param arg	(rtt_link_worker_arg_t) 
 */
extern void * rtt_link_worker(void *arg);
