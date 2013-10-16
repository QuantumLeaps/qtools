##############################################################################
# Product: QSpy utility, Linux, gcc
# Last Updated for Version: 4.5.00
# Date of the Last Update:  May 24, 2012
#
#                    Q u a n t u m     L e a P s
#                    ---------------------------
#                    innovating embedded systems
#
# Copyright (C) 2002-2012 Quantum Leaps, LLC. All rights reserved.
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
# Quantum Leaps Web sites: http://www.quantum-leaps.com
#                          http://www.state-machine.com
# e-mail:                  info@quantum-leaps.com
##############################################################################
# general utilities
RM = rm

# The gcc compiler and linker
CC   = gcc
LINK = gcc

INC_DIR   = ../../include
SRC_DIR   = ../../source
BLDDIR    = .
LINKFLAGS =

CCINC    := -I$(INC_DIR) -I$(BLDDIR)
APP_DEP  := \
	$(INC_DIR)/getopt.h \
	$(INC_DIR)/hal.h \
	$(INC_DIR)/qspy.h
APP      := qspy

# rel (default target) .......................................................

BINDIR   = rel

# g++ options
# -c    : compile only
# -O2   : optimize
CCFLAGS  = -O2 -c -Wall -o$@

# debug ......................................................................
ifeq ($(MAKECMDGOALS), dbg)

BINDIR   = dbg

# gcc options
# -c    : compile only
# -g    : source level debugging
CCFLAGS  = -g -c -Wall -o$@

endif
ifeq ($(MAKECMDGOALS), rel_clean)

BINDIR   = rel

endif

#.............................................................................

$(BINDIR)/$(APP) : \
	$(BINDIR)/getopt.o \
	$(BINDIR)/main.o \
	$(BINDIR)/qspy.o \
	$(BINDIR)/com.o \
	$(BINDIR)/tcp.o
	$(LINK) $(LINKFLAGS) \
	-o $(BINDIR)/$(APP) \
	$(BINDIR)/getopt.o \
	$(BINDIR)/main.o \
	$(BINDIR)/qspy.o \
	$(BINDIR)/com.o \
	$(BINDIR)/tcp.o

$(BINDIR)/getopt.o: $(SRC_DIR)/getopt.c $(APP_DEP)
	$(CC) $(CCFLAGS) $(CCINC) $<

$(BINDIR)/main.o: $(SRC_DIR)/main.c $(APP_DEP)
	$(CC) $(CCFLAGS) $(CCINC) $<

$(BINDIR)/qspy.o: $(SRC_DIR)/qspy.c $(APP_DEP)
	$(CC) $(CCFLAGS) $(CCINC) $<

$(BINDIR)/com.o: $(BLDDIR)/com.c $(APP_DEP)
	$(CC) $(CCFLAGS) $(CCINC) $<

$(BINDIR)/tcp.o: $(BLDDIR)/tcp.c $(APP_DEP)
	$(CC) $(CCFLAGS) $(CCINC) $<


dbg: $(BINDIR)/$(APP)

rel: $(BINDIR)/$(APP)


# clean targets...

.PHONY: clean rel_clean

rel_clean: clean

dbg_clean: clean

clean:
	-$(RM) $(BINDIR)/*.o
	-$(RM) $(BINDIR)/*.d
	-$(RM) $(BINDIR)/$(APP)
