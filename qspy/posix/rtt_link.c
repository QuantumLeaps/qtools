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
* @brief RTT via J-Link for QSPY tool (POSIX)
* @ingroup qpspy
*/
#include <stdint.h>
#include <dlfcn.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <rtt_link.h>

#include "safe_std.h" /* "safe" <stdio.h> and <string.h> facilities */

/* OS related ==============================================================*/

#ifndef JLINK_SHARED_LIB
#define JLINK_SHARED_LIB	"/opt/SEGGER/JLink/libjlinkarm.so"
#endif /* JLINK_SHARED_LIB */

static lib_exports_t lib_exports = { .lib_name = JLINK_SHARED_LIB };
/**
 * Open shared library and locate necessary exports
 */
static void lib_open(void) {
	if (lib_exports.lib == NULL) {
		lib_exports.lib = dlopen(lib_exports.lib_name, RTLD_NOW);
		if (lib_exports.lib == NULL) {
			pthread_exit(JLINK_SHARED_LIB " not found");
		}
	}
}
/**
 * Close shared library
 */
static void lib_close(void *p) {
	void * * const pLib = (void ** const)p;
	dlclose(*pLib);
	*pLib = NULL;
}
/**
 * Fetch entry point for an export
 *
 * @param pLib	shared library
 * @param name	export from library
 * @return	 	fetched export
 */
static void * _get_export(void *pLib, char *const name) {
	static char errMsg[256];
	void *pFnc;

	dlerror(); /* clear any pending error - see man7 */
	pFnc = dlsym(pLib, name);
	if (dlerror() != NULL) {
		SNPRINTF_S(errMsg, sizeof(errMsg), "Missing library export '%s'", name);
		pthread_exit(errMsg);
	}
	return pFnc;
}
/**
 * Fetch all procedures necessary to the task
 *
 * @note -Wpedantic ignored, otherwise compiler issues:
 * "ISO C forbids assignment between function pointer and 'void *'".
 *  (Typical *(void **)(&lib_exports.pfn*) workaround not used here.)
 */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#define _GET_EXPORT(pfn, name) do {                                          \
	(pfn) = (typeof((pfn)))_get_export(pLib, name);                          \
} while(0)
static void lib_get_exports(void *pLib) {
	_GET_EXPORT(lib_exports.pfnOpen, "JLINKARM_Open");
	_GET_EXPORT(lib_exports.pfnClose, "JLINKARM_Close");
	/* Locate J-Link(s) */
	_GET_EXPORT(lib_exports.pfnGetList, "JLINKARM_EMU_GetList");
	_GET_EXPORT(lib_exports.pfnSelectByUSBSN, "JLINKARM_EMU_SelectByUSBSN");
	_GET_EXPORT(lib_exports.pfnSelectByIPSN, "JLINKARM_EMU_SelectIPBySN");
	/* Locate and connect device */
	_GET_EXPORT(lib_exports.pfnExecCommand, "JLINKARM_ExecCommand");
	_GET_EXPORT(lib_exports.pfnTIF_GetAvailable, "JLINKARM_TIF_GetAvailable");
	_GET_EXPORT(lib_exports.pfnTIF_Select, "JLINKARM_TIF_Select");
	_GET_EXPORT(lib_exports.pfnConnect, "JLINKARM_Connect");
	_GET_EXPORT(lib_exports.pfnSetMaxSpeed, "JLINKARM_SetMaxSpeed");
	/* Start device (just in case) */
	_GET_EXPORT(lib_exports.pfnGetNumConnections, "JLINKARM_EMU_GetNumConnections");
	_GET_EXPORT(lib_exports.pfnIsHalted, "JLINKARM_IsHalted");
	_GET_EXPORT(lib_exports.pfnGo, "JLINKARM_Go");
	/* RTT usage */
	_GET_EXPORT(lib_exports.pfnRTT_Control, "JLINK_RTTERMINAL_Control");
	_GET_EXPORT(lib_exports.pfnRTT_Read, "JLINK_RTTERMINAL_Read");
	_GET_EXPORT(lib_exports.pfnRTT_Write, "JLINK_RTTERMINAL_Write");
}
#undef _GET_EXPORT
#pragma GCC diagnostic pop

/* J-Link related ==========================================================*/

#define NO_BUFFER	 UINT32_MAX /* 'flag' ... buffer can not be used */
/**
 * Data to the RTT system
 */
static struct {
	int bRTTrunning; /* flag RTT is working */
	uint32_t qspy_buffer_up; /* ID of buffer */
	uint32_t qspy_buffer_down; /* ID of buffer */
} rtt_system = { .qspy_buffer_up = NO_BUFFER, . qspy_buffer_down = NO_BUFFER };
/**
 * Open J-Link
 *
 * @param serial_number	J-Link's serial number (if provided)
 * @note Limited to N J-Links only (just now N = 7)
 */
static void jlink_open(uint32_t const serial_number) {
	uint8_t in[2048]; /* 7 J-Links should be more than enough */
	jlink_info_t const *pEnd, *pInfo = (jlink_info_t const *) in;

	int retval = lib_exports.pfnGetList(HOST_IF_IP | HOST_IF_USB, in,
			sizeof(in) / sizeof(jlink_info_t));
	if (retval <= 0) {
		pthread_exit("J-Link(s) not connected");
	}
	/*
	 * Use SN to select J-Link device
	 * Remark: If SN is 0 (not provided)/invalid, list of connected J-Links
	 *         will be shown by JLINKARM_Open()
	 */
	if (serial_number != 0) {
		/* Detect host_if */
		pEnd = pInfo + retval;
		if ((uint8_t*) pEnd > in + sizeof(in)) {
			pEnd = (jlink_info_t const*) (in + sizeof(in));
		}
		while (pInfo < pEnd) {
			if (pInfo->serial_number == serial_number) {
				/* Select searched J-Link via its host_if */
				switch (pInfo->host_if) {
					case HOST_IF_USB: {
						if (lib_exports.pfnSelectByUSBSN(serial_number) < 0) {
							pthread_exit("J-Link with given serial number not found");
						}
						break;
					}
					case HOST_IF_IP: {
						lib_exports.pfnSelectByIPSN(serial_number);
						/* Seems impossible to get success/error info here. */
						break;
					}
					default: {
						pthread_exit("Unknown HOST_IF");
						break;
					}
				}
				break;
			}
			pInfo++;
		}
	}
	char const * errMsg = lib_exports.pfnOpen();
	if (errMsg != NULL)	{
		pthread_exit((void *)errMsg);
	}
}
/**
 * Close J-Link
 * @param p
 */
static void jlink_close(void *p) {
	(void)p;
	lib_exports.pfnClose();
}
/**
 * Connect J-Link to a target device
 *
 * @param device		target processor
 */
static void connect_device(char const *device) {
	char out[1024];
	uint8_t in[2048]; /* 7 J-Links should be more than enough */
	uint32_t TIFs, tif;

	snprintf(out, sizeof(out), "device=%s", device);
	(void) lib_exports.pfnExecCommand(out, (char*) in, sizeof(in));
	if (in[0] != '\0') {
		pthread_exit("ExecCommand failed");
	}
	/* Find all TIFs of opened J-Link */
	lib_exports.pfnTIF_GetAvailable(&TIFs);
	/* 	TIFs is bitmask; bit positions: 0 ... JTAG, 1 ... SWD, etc. */
	while (TIFs) {
		tif = 31 - __builtin_clz(TIFs); /* 1 --> this TIF is present */
		(void)lib_exports.pfnTIF_Select(tif); /* TIF is selected by bitpos */
		if (lib_exports.pfnConnect() >= 0) {
			/* Connected */
			break;
		}
		/* Connect failed */
		TIFs &= ~(1 << tif);
		if (TIFs == 0) {
			pthread_exit("All TIFs available probed and all failed");
		}
	}
	lib_exports.pfnSetMaxSpeed();
}

/* RTT related =============================================================*/

/**
 * Parse RTT buffer descriptions to find the correct one by name
 *
 * @param cnt	total number of buffers available
 * @param dir	direction of searched buffer (host->target, target->host)
 * @return	NO_BUFFER	named buffer not found
 * 			!NO_BUFFER	ID of searched buffer
 */
static uint32_t _rtt_find_buffer(int cnt, uint32_t dir) {
	jlink_rtt_desc_t inout;
	int i;

	for (i = 0; i < cnt; i++) {
		memset(&inout, 0, sizeof(inout));
		inout.idx = i;
		inout.dir = dir;
		if (lib_exports.pfnRTT_Control(RTT_CMD_GETDESC, &inout) < 0) {
			break; /* Error */
		}
		if (strncmp(inout.acName, RTT_QSPY_BUFFER_NAME, sizeof(inout.acName)) == 0) {
			return i; /* Buffer exists */
		}
	}
	return NO_BUFFER; /* Not found/Error */
}
/**
 * Start new RTT communication
 *
 * @return	bRTTrunning
 * @note RTT start-up can take really long so bStartDone helps to split
 *  the procedure into shorter steps
 */
static int rtt_start(void) {
	static int bStartDone = 0;
	jlink_rtt_desc_t inout;
	int retval;

	if (bStartDone == 0) {
		bStartDone = 100; /* Num of retries among RTT_CMD_STARTs */
		/* Let's go (if halted) */
		if (lib_exports.pfnGetNumConnections() == 1) {
			/* As the only J-Link connected it has to start target if necessary */
			retval = lib_exports.pfnIsHalted();
			if (retval < 0)
				pthread_exit("Command JLINKARM_IsHalted() failed");
			if (retval == 1) {
				lib_exports.pfnGo(); // Halted device started
			}
		}
		(void)lib_exports.pfnRTT_Control(RTT_CMD_START, NULL); /* autodetect CB */
		return 0;
	}
	memset(&inout, 0, sizeof(inout));
	inout.dir = RTT_DIR_T2H;
	retval = lib_exports.pfnRTT_Control(RTT_CMD_GETNUMBUF, &inout);
	if (retval == -2) {
		/* -2 ... RTT is not ready (CB not found yet?) */
		usleep(5000); // 5ms
		bStartDone--;
		return 0;
	}
	if (retval < 0) {
		/* Target could be just reloaded ... */
		bStartDone = 0;
		return 0;
	}
	rtt_system.qspy_buffer_up = _rtt_find_buffer(retval, RTT_DIR_T2H);
	/* Buffer for H->T communication is optional */
	memset(&inout, 0, sizeof(inout));
	inout.dir = RTT_DIR_H2T;
	retval = lib_exports.pfnRTT_Control(RTT_CMD_GETNUMBUF, &inout);
	if (retval < 0) {
		pthread_exit("RTT_Control(CMD_GETNUMBUF, DIR_T2H) failed");
	}
	rtt_system.qspy_buffer_down = _rtt_find_buffer(retval, RTT_DIR_H2T);
	/* Buffer for T->H communication is mandatory */
	if (rtt_system.qspy_buffer_up == NO_BUFFER) {
		pthread_exit("RTT Channel T->H not found");
	}
	bStartDone = 0;
	return 1;
}
/**
 * Stop RTT (if already failed, just clean-up)
 */
static void rtt_stop(void) {
	lib_exports.pfnRTT_Control(RTT_CMD_STOP, NULL);
	rtt_system.qspy_buffer_up = NO_BUFFER;
	rtt_system.qspy_buffer_down = NO_BUFFER;
	rtt_system.bRTTrunning = 0;
}
/**
 * RTT communication from the target device to the host
 *
 * @param bExit	global flags of unrecoverable problems
 * @param fd	communication link to the main thread
 * @return		skip RTT for this iteration
 */
static int rtt_target2host(int *const bExit, int fd) {
	char buf[2048];
	int total, cnt, i;

	total = lib_exports.pfnRTT_Read(rtt_system.qspy_buffer_up, buf,
			sizeof(buf));
	if (total < 0) {
		/* Problem with RTT communication, stop it for now */
		rtt_stop();
		return 1;
	}
	if (total == 0) {
		/* No new data --> no need to hurry now */
		usleep(2000); // 2ms
		return 0;
	}
	/* Transmit additional data for the host */
	for (i = 0; i < total; i += cnt) {
		cnt = write(fd, buf + i, total - i);
		if (cnt == -1) {
			cnt = 0;
			if ((errno == EAGAIN) || (errno == EINTR)) {
				/* 1ms relax for busy host device */
				usleep(1000);
				continue;
			}
			*bExit = 1; // read-end closed or ERR
			return 1;
		}
	}
	return 0;
}
/**
 * RTT communication from the host to the target device
 *
 * @param bExit	global flags of unrecoverable problems
 * @param fd	communication link from the main thread
 * @return		skip RTT for this iteration
 */
static int rtt_host2target(int *const bExit, int fd) {
	char buf[256];
	int total, cnt, i;

	total = read(fd, buf, sizeof(buf));
	if (total <= 0) {
		if (((errno == EAGAIN) || (errno == EINTR)) == 0) {
			*bExit = 1; // write-end closed or ERR
		}
		return 1;
	}
	/* Transmit additional data for the target */
	for (i = 0; i < total; i += cnt) {
		cnt = lib_exports.pfnRTT_Write(rtt_system.qspy_buffer_down, buf + i, total - i);
		if (cnt < 0) {
			/* Problem with RTT communication, stop it for now */
			rtt_stop();
			return 1;
		}
		if (cnt == 0) {
			/* 1ms relax for busy target device */
			usleep(1000);
		}
	}
	return 0;
}

/* =========================================================================*/

/**
 * End of rtt_link_worker() task
 *
 * @param  arg	data about J-Link + fd for connection to the main thread
 * @note Closed pipes are signals for the main thread.
 */
static void close_pipes(void * arg) {
	rtt_link_worker_arg_t const * const args = (rtt_link_worker_arg_t *)arg;
	close(args->fdT2H[1]); // write-end
	close(args->fdH2T[0]); // read-end
}
/**
 * Exported function for threading (in parallel to QSPY)
 *
 * @param arg	data about J-Link + fd for connection to the main thread
 * @note -Wclobbered ignored, otherwise compiler issues:
 * "variable ‘__cancel_routine’ might be clobbered by ‘longjmp’ or ‘vfork’".
 *  (GCC bug 61118 since v4.9.0, 28/Jun/2022 retargeted to v10.5.)
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wclobbered"
void* rtt_link_worker(void *arg) {
	rtt_link_worker_arg_t const * const args = (rtt_link_worker_arg_t *)arg;
	int bGoToExit = 0;

	pthread_cleanup_push(close_pipes, arg);
	lib_open();
	pthread_cleanup_push(lib_close, &(lib_exports.lib));
	lib_get_exports(lib_exports.lib);
	jlink_open(args->serNo);
	pthread_cleanup_push(jlink_close, NULL);
	connect_device(args->device);

	while (bGoToExit == 0) {
		if (rtt_system.bRTTrunning == 0) {
			/* RTT has been interrupted, needs to be started again */
			rtt_system.bRTTrunning = rtt_start();
		}
		else {
			/* Target --> Host (mandatory) */
			if (rtt_target2host(&bGoToExit, args->fdT2H[1]) == 0) {
				/* Host --> Target (optional) */
				if (rtt_system.qspy_buffer_down != NO_BUFFER) {
					(void)rtt_host2target(&bGoToExit, args->fdH2T[0]);
				}
			}
		}
	}

	rtt_stop();
	pthread_cleanup_pop(1); // jlink_close()
	pthread_cleanup_pop(1); // lib_close()
	pthread_cleanup_pop(1); // close_pipes()
	return NULL;
}
#pragma GCC diagnostic pop
