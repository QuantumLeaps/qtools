##############################################################################
# Product: Makefile for QWIN Demo, GNU GCC
# Last updated for version 6.3.7
# Last updated on  2018-11-06
#
#                    Q u a n t u m  L e a P s
#                    ------------------------
#                    Modern Embedded Software
#
# Copyright (C) 2005-2018 Quantum Leaps, LLC. All rights reserved.
#
# This program is open source software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Alternatively, this program may be distributed and modified under the
# terms of Quantum Leaps commercial licenses, which expressly supersede
# the GNU General Public License and are specifically designed for
# licensees interested in retaining the proprietary status of their code.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Contact information:
# https://www.state-machine.com
# mailto:info@state-machine.com
##############################################################################
# examples of invoking this Makefile:
# building configurations: Debug (default), Release, and Spy
# make
# make CONF=rel
# make CONF=spy
#
# cleaning configurations: Debug (default), Release, and Spy
# make clean
# make CONF=rel clean
# make CONF=spy clean
#
# NOTE:
# To use this Makefile on Windows, you will need the GNU make utility, which
# is included in the QTools collection for Windows, see:
#    http://sourceforge.net/projects/qpc/files/QTools/
#

#-----------------------------------------------------------------------------
# project name
#
PROJECT := qwin_demo

#-----------------------------------------------------------------------------
# project directories
#

# list of all source directories used by this project
VPATH = \
	.

# list of all include directories needed by this project
INCLUDES  = \
	-I.

# list of resource include directories needed by this project
RCINCLUDES = \
	-IRes


#-----------------------------------------------------------------------------
# files
#

# C source files...
C_SRCS := \
	main.c \
	bsp_win32.c \
	qwin_gui.c

# C++ source files...
CPP_SRCS :=

# Resource files...
RC_SRCS := \
	qwin_demo.rc

LIB_DIRS  :=
LIBS      :=

# defines...
DEFINES   := -DQWIN_GUI

#-----------------------------------------------------------------------------
# MinGW toolset (NOTE: assumed to be on your PATH)
#
# NOTE:
# MinGW toolset is included in the Qtools collection for Windows, see:
#    http://sourceforge.net/projects/qpc/files/Qtools/
#
# NOTE:
# This Makefile assumes that the windres utility is available on the
# PATH (NOTE: windres is available in the Qtools collection for Windows)
#
CC    := gcc
CPP   := g++
LINK  := gcc    # for C programs
#LINK  := g++   # for C++ programs
RC    := windres

# basic utilities (included in Qtools for Windows), see:
#    http://sourceforge.net/projects/qpc/files/Qtools

MKDIR := mkdir
RM    := rm

#-----------------------------------------------------------------------------
# build options for various configurations
#

ifeq (rel, $(CONF)) # Release configuration ..................................

BIN_DIR := rel

CFLAGS = -c -ffunction-sections -fdata-sections \
	-O1 -Wall -Wextra $(INCLUDES) $(DEFINES) -DNDEBUG

CPPFLAGS = -c -ffunction-sections -fdata-sections \
	-O1 -Wall -Wextra $(INCLUDES) $(DEFINES) -DNDEBUG

else ifeq (spy, $(CONF))  # Spy configuration ................................

BIN_DIR := build_spy

LIBS    += -lws2_32

CFLAGS = -c -g -ffunction-sections -fdata-sections \
	-O -Wall -Wextra $(INCLUDES) $(DEFINES) -DQ_SPY

CPPFLAGS =-c  -g -ffunction-sections -fdata-sections \
	-O -Wall -Wextra $(INCLUDES) $(DEFINES) -DQ_SPY

else  # default Debug configuration ..........................................

BIN_DIR := build

CFLAGS = -c -g -ffunction-sections -fdata-sections \
	-O -Wall -Wextra $(INCLUDES) $(DEFINES)

CPPFLAGS = -c -g -ffunction-sections -fdata-sections \
	-O -Wall -Wextra $(INCLUDES) $(DEFINES)

endif  # .....................................................................

LINKFLAGS :=

# is it a GUI application (any GUI resources provided?) ...
ifneq (,$(RC_SRCS))
LINKFLAGS += -mwindows
DEFINES += -DQWIN_GUI
endif

#-----------------------------------------------------------------------------
C_OBJS       := $(patsubst %.c,%.o,   $(C_SRCS))
CPP_OBJS     := $(patsubst %.cpp,%.o, $(CPP_SRCS))
RC_OBJS      := $(patsubst %.rc,  %.o, $(RC_SRCS))

TARGET_EXE   := $(BIN_DIR)/$(PROJECT)$(TARGET_EXT)
C_OBJS_EXT   := $(addprefix $(BIN_DIR)/, $(C_OBJS))
C_DEPS_EXT   := $(patsubst %.o,%.d, $(C_OBJS_EXT))
CPP_OBJS_EXT := $(addprefix $(BIN_DIR)/, $(CPP_OBJS))
CPP_DEPS_EXT := $(patsubst %.o,%.d, $(CPP_OBJS_EXT))
RC_OBJS_EXT  := $(addprefix $(BIN_DIR)/, $(RC_OBJS))

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

$(BIN_DIR)/%.d : %.cpp
	$(CPP) -MM -MT $(@:.d=.o) $(CPPFLAGS) $< > $@

$(BIN_DIR)/%.d : %.c
	$(CC) -MM -MT $(@:.d=.o) $(CFLAGS) $< > $@

$(BIN_DIR)/%.o : %.cpp
	$(CPP) $(CPPFLAGS) $< -o $@

$(BIN_DIR)/%.o : %.c
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/%.o : %.rc
	$(RC) $(RCINCLUDES) -i $< -o $@

# include dependency files only if our goal depends on their existence
ifneq ($(MAKECMDGOALS),clean)
  ifneq ($(MAKECMDGOALS),show)
-include $(C_DEPS_EXT) $(CPP_DEPS_EXT)
  endif
endif

.PHONY : clean show

clean :
	-$(RM) $(BIN_DIR)/*.o \
	$(BIN_DIR)/*.d \
	$(TARGET_EXE)

show :
	@echo PROJECT      = $(PROJECT)
	@echo TARGET_EXE   = $(TARGET_EXE)
	@echo VPATH        = $(VPATH)
	@echo C_SRCS       = $(C_SRCS)
	@echo CPP_SRCS     = $(CPP_SRCS)
	@echo RC_SRCS      = $(RC_SRCS)
	@echo C_DEPS_EXT   = $(C_DEPS_EXT)
	@echo C_OBJS_EXT   = $(C_OBJS_EXT)
	@echo C_DEPS_EXT   = $(C_DEPS_EXT)
	@echo CPP_DEPS_EXT = $(CPP_DEPS_EXT)
	@echo CPP_OBJS_EXT = $(CPP_OBJS_EXT)
	@echo RC_OBJS_EXT  = $(RC_OBJS_EXT)
	@echo LIB_DIRS     = $(LIB_DIRS)
	@echo LIBS         = $(LIBS)
	@echo DEFINES      = $(DEFINES)
