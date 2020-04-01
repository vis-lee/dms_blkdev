
#################################
#	DriverCore MAKEFILE			#
#################################

.PHONY:clean

#################################
#		Global configs			#
#################################
DMS_SUBDIRS		= 
BIN_DIR			= ../bin

#################################
#			ENV PATH			#
#################################



#################################
#			sub-modules			#
#################################

#####	Overwritten Flag ########
#OVW_MOD_NAME = ovw_module
#OVW_SUBDIRS = overwritten
#OVW_INC_DIR = $(OVW_SUBDIRS)/inc
#OVW_SRC_DIR = $(OVW_SUBDIRS)/src

#####	CCMAKDBG ########
#CCMAKDBG_MOD_NAME = ccmakdbg
#CCMAKDBG_SUBDIRS  = ccmakdbg

#####	CCMAKDBG CLI Engine ########
#CCMAKDBG_CLI_MOD_NAME = ccmakdbg_cli
#CCMAKDBG_CLI_SUBDIRS  = ccmakdbg_cli

UTEST_MOD_NAME = utest
UTEST_SRCDIR = ../UnitTest
UTEST_INC_DIR = $(UTEST_SRCDIR)/include
UTEST_SRC_DIR = $(UTEST_SRCDIR)/src


# add dir to DMS_SUBDIRS
#DMS_SUBDIRS += $(UTEST_SRC_DIR)

#################################
#		DMS VDD CONFIGS			#
#################################
#module name
MODULE_NAME := dms_vdd

ifneq ($(KERNELRELEASE),)
# call from kernel build system

EXTRA_CFLAGS := -I$(PWD)/include -I$(PWD)/../UnitTest/kernelspace/include

$(MODULE_NAME)-objs := DMS_VDDriver.o DMS_Error.o Volume.o Volume_Manager.o DMS_Mem_Pool.o DList.o Allocation_Flag.o LogicalBlockList.o \
                       DIO.o IO_Request.o Driver_Core.o Namenode_Protocol.o Metadata.o Metadata_Manager.o SSocket.o SSockets_Manager.o #$(UTEST_MOD_NAME)/$(UTEST_MOD_NAME).o


obj-m := $(MODULE_NAME).o


else
#################################
#			MAKE TAGS			#
#################################

all:
	for dirs in $(DMS_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done
	$(MAKE) -C/lib/modules/$(shell uname -r)/build M=$(PWD)	
	cp -u $(MODULE_NAME).ko $(BIN_DIR)
	
check:
	for dirs in $(DMS_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done
	$(MAKE) C=1 -C/lib/modules/$(shell uname -r)/build M=$(PWD)	
	
dbg:
	for dirs in $(DMS_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done
	$(MAKE) -C/lib/modules/2.6.18-164.el5debug M=$(PWD)	
	
phy:
	for dirs in $(DMS_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done
	$(MAKE) -C/lib/modules/2.6.18-194.8.1.el5xen/build M=$(PWD)	
	
clean:
	rm -rf *.o *.ko *.symvers *.mod.c .*.cmd Module.markers modules.order version.h
	rm -rf $(DAEMON_NAME)
	for dirs in $(DMS_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done
endif

	
	
	
	