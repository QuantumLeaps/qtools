##############################################################################
# Product: Makefile for QCLEAN console, POSIX(Linux/macOS), GCC compiler
#
#                    Q u a n t u m  L e a P s
#                    ------------------------
#                    Modern Embedded Software
#
# Copyright (C) 2005 Quantum Leaps, LLC. All rights reserved.
#
# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-QL-commercial
#
# This software is dual-licensed under the terms of the open source GNU
# General Public License version 3 (or any later version), or alternatively,
# under the terms of one of the closed source Quantum Leaps commercial
# licenses.
#
# The terms of the open source GNU General Public License version 3
# can be found at: <www.gnu.org/licenses/gpl-3.0>
#
# The terms of the closed source Quantum Leaps commercial licenses
# can be found at: <www.state-machine.com/licensing>
#
# Redistributions in source code must retain this top-level comment block.
# Plagiarizing this software to sidestep the license obligations is illegal.
#
# Contact information:
# <www.state-machine.com>
# <info@state-machine.com>
##############################################################################
# examples of invoking this Makefile:
# building configurations: Debug (default), Release, and Spy
# make
# make CONF=dbg
#
# cleaning configurations: Debug (default), Release, and Spy
# make clean
# make CONF=dbg clean

#-----------------------------------------------------------------------------
# project name
#
PROJECT := qclean

#-----------------------------------------------------------------------------
# project directories:
#

# list of all source directories used by this project
VPATH := . \
	../source

# list of all include directories needed by this project
INCLUDES := -I. \
	-I../include


#-----------------------------------------------------------------------------
# project files:
#

# C source files...
C_SRCS := \
	main.c \
	getopt.c \
	filesearch_posix.c

# C++ source files...
CPP_SRCS :=

LIB_DIRS  :=
LIBS      :=

#-----------------------------------------------------------------------------
# GNU toolset:
#
CC    := gcc
CPP   := g++
LINK  := gcc    # for C programs
#LINK  := g++   # for C++ programs

#-----------------------------------------------------------------------------
# basic utilities:
#
MKDIR := mkdir -p
RM    := rm

#-----------------------------------------------------------------------------
# build configurations...
#

ifeq (dbg, $(CONF)) # Debug configuration ....................................

BIN_DIR := dbg

CFLAGS = -c -g -std=gnu11 -ffunction-sections -fdata-sections \
	-O -Wall -Wextra -Wpedantic $(INCLUDES) $(DEFINES)

CPPFLAGS = -c -g -ffunction-sections -fdata-sections \
	-O -Wall -Wextra $(INCLUDES) $(DEFINES)

else  # default Release configuration ........................................

BIN_DIR := rel

CFLAGS = -c -std=gnu11 -ffunction-sections -fdata-sections \
	-O3 -Wall -Wextra -Wpedantic $(INCLUDES) $(DEFINES) -DNDEBUG

CPPFLAGS = -c -ffunction-sections -fdata-sections \
	-O3 -Wall -Wextra $(INCLUDES) $(DEFINES) -DNDEBUG

endif  # .....................................................................

UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
	LINKFLAGS :=
else
ifndef GCC_OLD
	LINKFLAGS := -no-pie
endif
endif

#-----------------------------------------------------------------------------
C_OBJS       := $(patsubst %.c,%.o,   $(C_SRCS))
CPP_OBJS     := $(patsubst %.cpp,%.o, $(CPP_SRCS))

TARGET_EXE   := $(BIN_DIR)/$(PROJECT)$(TARGET_EXT)
C_OBJS_EXT   := $(addprefix $(BIN_DIR)/, $(C_OBJS))
C_DEPS_EXT   := $(patsubst %.o,%.d, $(C_OBJS_EXT))
CPP_OBJS_EXT := $(addprefix $(BIN_DIR)/, $(CPP_OBJS))
CPP_DEPS_EXT := $(patsubst %.o,%.d, $(CPP_OBJS_EXT))

# create $(BIN_DIR) if it does not exist
ifeq ("$(wildcard $(BIN_DIR))","")
$(shell $(MKDIR) $(BIN_DIR))
endif

#-----------------------------------------------------------------------------
# rules
#

all: $(TARGET_EXE)

$(TARGET_EXE) : $(C_OBJS_EXT) $(CPP_OBJS_EXT) $(RC_OBJS_EXT)
	$(LINK) $(LINKFLAGS) $(LIB_DIRS) -o $@ $^ $(LIBS)
	cp $@ ../../bin

$(BIN_DIR)/%.d : %.cpp
	$(CPP) -MM -MT $(@:.d=.o) $(CPPFLAGS) $< > $@

$(BIN_DIR)/%.d : %.c
	$(CC) -MM -MT $(@:.d=.o) $(CFLAGS) $< > $@

$(BIN_DIR)/%.o : %.cpp
	$(CPP) $(CPPFLAGS) $< -o $@

$(BIN_DIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

.PHONY : clean show

# include dependency files only if our goal depends on their existence
ifneq ($(MAKECMDGOALS),clean)
  ifneq ($(MAKECMDGOALS),show)
-include $(C_DEPS_EXT) $(CPP_DEPS_EXT)
  endif
endif

.PHONY : clean show

clean:
	-$(RM) $(BIN_DIR)/*.o \
	$(BIN_DIR)/*.d \
	$(TARGET_EXE)

show:
	@echo PROJECT      = $(PROJECT)
	@echo TARGET_EXE   = $(TARGET_EXE)
	@echo VPATH        = $(VPATH)
	@echo C_SRCS       = $(C_SRCS)
	@echo CPP_SRCS     = $(CPP_SRCS)
	@echo C_DEPS_EXT   = $(C_DEPS_EXT)
	@echo C_OBJS_EXT   = $(C_OBJS_EXT)
	@echo C_DEPS_EXT   = $(C_DEPS_EXT)
	@echo CPP_DEPS_EXT = $(CPP_DEPS_EXT)
	@echo CPP_OBJS_EXT = $(CPP_OBJS_EXT)
	@echo LIB_DIRS     = $(LIB_DIRS)
	@echo LIBS         = $(LIBS)
	@echo DEFINES      = $(DEFINES)

