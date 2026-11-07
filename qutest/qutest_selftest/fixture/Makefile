#=============================================================================
# QUTest self-test fixture -- Makefile for Windows and POSIX *HOSTS*
#
# Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
#
# This Makefile ONLY builds the fixture executable. The tests are run by
# ../run_selftest.py (never by this Makefile), so that the qutest.py under
# test is always selected explicitly.
#
# examples:
# make                        # build with the default QP location
# make QP=c:/qp-dev/safe-qpc  # build against a specific QP/C
# make BIN_DIR=/tmp/st_build  # build outside the source tree
# make clean
#=============================================================================
PROJECT := selftest_fixture

# location of the QP/C framework (pinned; override only deliberately)
QP      ?= ../../../../safe-qpc
BIN_DIR ?= build

VPATH    := .
INCLUDES := -I.
C_SRCS   := selftest_fixture.c
DEFINES  := -DQ_SPY -DQ_UTEST -DQ_HOST

ifeq ($(OS),Windows_NT)
QP_PORT_DIR := $(QP)/ports/win32-qutest
LIBS        := -lws2_32
TARGET_EXT  := .exe
MKDIR       := mkdir
RM          := rm
else
QP_PORT_DIR := $(QP)/ports/posix-qutest
LIBS        := -lpthread
TARGET_EXT  :=
MKDIR       := mkdir -p
RM          := rm -f
endif

VPATH    += $(QP)/src/qf $(QP)/src/qs $(QP_PORT_DIR)
INCLUDES += -I$(QP)/include -I$(QP_PORT_DIR)

C_SRCS += \
	qep_hsm.c \
	qep_msm.c \
	qf_act.c \
	qf_actq.c \
	qf_defer.c \
	qf_dyn.c \
	qf_mem.c \
	qf_ps.c \
	qf_qact.c \
	qf_qeq.c \
	qf_qmact.c \
	qf_time.c \
	qs.c \
	qs_64bit.c \
	qs_rx.c \
	qs_fp.c \
	qutest.c \
	qf_stub.c \
	qutest_port.c

CC     := gcc
CFLAGS := -c -g -O -std=c11 -pedantic -Wall -Wextra -fno-pie \
	$(INCLUDES) $(DEFINES)
LINKFLAGS := -no-pie

TARGET_EXE := $(BIN_DIR)/$(PROJECT)$(TARGET_EXT)
C_OBJS_EXT := $(addprefix $(BIN_DIR)/, $(C_SRCS:.c=.o))

.PHONY : all clean show

all : $(TARGET_EXE)

$(TARGET_EXE) : $(C_OBJS_EXT)
	$(CC) $(CFLAGS) $(QP)/src/qs/qstamp.c -o $(BIN_DIR)/qstamp.o
	$(CC) $(LINKFLAGS) -o $@ $^ $(BIN_DIR)/qstamp.o $(LIBS)

$(BIN_DIR)/%.o : %.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR) :
	$(MKDIR) $(BIN_DIR)

clean :
	-$(RM) $(BIN_DIR)/*.o $(TARGET_EXE)

show :
	@echo QP         = $(QP)
	@echo BIN_DIR    = $(BIN_DIR)
	@echo TARGET_EXE = $(TARGET_EXE)
