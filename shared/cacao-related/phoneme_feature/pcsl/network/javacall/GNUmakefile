#
#
# Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License version
# 2 only, as published by the Free Software Foundation. 
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License version 2 for more details (a copy is
# included at /legal/license.txt). 
# 
# You should have received a copy of the GNU General Public License
# version 2 along with this work; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
# 02110-1301 USA 
# 
# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
# Clara, CA 95054 or visit www.sun.com if you need additional
# information or have any questions. 
#
######################################################################
#
# Makefile for building and packaging.
#
######################################################################

# Workspace directory
PCSL_DIR		= $(CURDIR)/../..
NETWORK_MODULE		= javacall

# include top.gmk for various directory and module definitions
ifdef PCSL_PLATFORM
include $(PCSL_DIR)/makefiles/top.gmk
endif

# define 'all' target and all dependencies
# 'all' is the default target

NET_LIB = $(OUTPUT_LIB_DIR)/libpcsl_network$(LIB_EXT)
NET_INTERFACE = $(OUTPUT_INC_DIR)/pcsl_network.h $(OUTPUT_INC_DIR)/pcsl_network_md.h

OBJS = \
	$(OUTPUT_OBJ_DIR)/pcsl_socket.o \
	$(OUTPUT_OBJ_DIR)/pcsl_serversocket.o \
	$(OUTPUT_OBJ_DIR)/pcsl_network.o \
	$(OUTPUT_OBJ_DIR)/pcsl_datagram.o

all: verify $(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(OUTPUT_INC_DIR) \
     $(NET_LIB) $(NET_INTERFACE)

include $(PCSL_DIR)/makefiles/verify.gmk

$(OUTPUT_OBJ_DIR) $(OUTPUT_LIB_DIR) $(OUTPUT_INC_DIR):
	@mkdir -p $@

$(NET_LIB): $(OBJS)
	@$(AR) $(AR_OUTPUT)`$(call fixcygpath,$@)` `$(call fixcygpath, $^)`

#	cp -f $< $@
$(NET_INTERFACE): $(NETWORK_DIR)/pcsl_network.h
	cp -f $(NETWORK_DIR)/pcsl_network.h $(OUTPUT_INC_DIR)
	cp -f $(NETWORK_DIR)/pcsl_socket.h $(OUTPUT_INC_DIR)
	cp -f $(NETWORK_DIR)/pcsl_datagram.h $(OUTPUT_INC_DIR)
	cp -f $(NETWORK_SELECT_DIR)/pcsl_network_md.h $(OUTPUT_INC_DIR)
	cp -f $(NETWORK_DIR)/pcsl_network_notifier.h $(OUTPUT_INC_DIR)
	cp -f $(NETWORK_DIR)/pcsl_serversocket.h $(OUTPUT_INC_DIR)

$(OUTPUT_OBJ_DIR)/pcsl_socket.o: $(NETWORK_SELECT_DIR)/pcsl_socket.c \
 $(NETWORK_DIR)/pcsl_network.h $(MEMORY_DIR)/pcsl_memory.h \
 $(NETWORK_SELECT_DIR)/GNUmakefile GNUmakefile
	@$(CC) -I. -I.. -I`$(call fixcygpath,$(OUTPUT_INC_DIR))` $(CFLAGS) $(CC_OUTPUT)`$(call fixcygpath,$@)` `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_serversocket.o: $(NETWORK_SELECT_DIR)/pcsl_serversocket.c \
 $(NETWORK_DIR)/pcsl_network.h $(MEMORY_DIR)/pcsl_memory.h \
 $(NETWORK_SELECT_DIR)/GNUmakefile GNUmakefile
	@$(CC) -I. -I.. -I`$(call fixcygpath,$(OUTPUT_INC_DIR))` $(CFLAGS) $(CC_OUTPUT)`$(call fixcygpath,$@)` `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_network.o: $(NETWORK_SELECT_DIR)/pcsl_network.c \
 $(NETWORK_DIR)/pcsl_network.h $(MEMORY_DIR)/pcsl_memory.h \
 $(NETWORK_SELECT_DIR)/GNUmakefile GNUmakefile
	@$(CC) -I. -I.. -I`$(call fixcygpath,$(OUTPUT_INC_DIR))` $(CFLAGS) $(CC_OUTPUT)`$(call fixcygpath,$@)` `$(call fixcygpath, $<)`

$(OUTPUT_OBJ_DIR)/pcsl_datagram.o: $(NETWORK_SELECT_DIR)/pcsl_datagram.c \
 $(NETWORK_DIR)/pcsl_network.h $(MEMORY_DIR)/pcsl_memory.h \
 $(NETWORK_SELECT_DIR)/GNUmakefile GNUmakefile
	@$(CC) -I. -I.. -I`$(call fixcygpath,$(OUTPUT_INC_DIR))` $(CFLAGS) $(CC_OUTPUT)`$(call fixcygpath,$@)` `$(call fixcygpath, $<)`

# doc stuff. The 'doc' target is defined in Docs.gmk.
# Just have to define DOXYGEN_INPUT_LIST and force
# rebuild

DOXYGEN_INPUT_LIST += $(NETWORK_DIR)
FRC_DOC_REBUILD = force_doc_rebuild
include $(PCSL_DIR)/makefiles/share/Docs.gmk

# define ''clean' target

clean: verify
	rm -rf $(OUTPUT_OBJ_DIR)/pcsl_socket.o
	rm -rf $(OUTPUT_INC_DIR)/pcsl_network.h
	rm -rf $(OUTPUT_INC_DIR)/pcsl_socket.h
	rm -rf $(OUTPUT_INC_DIR)/pcsl_datagram.h
	rm -rf $(OUTPUT_LIB_DIR)/libpcsl_network$(LIB_EXT)
	rm -rf $(DOC_DIR)

.PHONY: all clean donuts doc verify

