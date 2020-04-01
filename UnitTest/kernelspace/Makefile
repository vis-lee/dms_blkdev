
#################################
#	DriverCore MAKEFILE			#
#################################

.PHONY:clean

#################################
#		Global configs			#
#################################
DMS_VDD_DIR		= $(PWD)/../../DriverCore/
UT_SUBDIRS		= 
BIN_DIR			= ../../bin

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


# add dir to DMS_SUBDIRS
#DMS_SUBDIRS += $(UTEST_SRC_DIR)

#################################
#		DMS VDD CONFIGS			#
#################################
#module name
MODULE_NAME := dms_vdd_utest

ifneq ($(KERNELRELEASE),)
# call from kernel build system

EXTRA_CFLAGS := -I$(PWD)/include -I$(DMS_VDD_DIR)/include

$(MODULE_NAME)-objs := DMS_UTest.o Test_Worker.o Test_Workers.o Test_Volume_Manager.o Test_DMS_Mem_Pool.o Test_DMS_DList.o Utils.o \
                       Test_vIO_Handler.o Test_SSockets_Manager.o

 
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
	rm -rf *.o *.ko *.symvers *.mod.c .*.cmd Module.markers modules.order .tmp_versions version.h
	rm -rf $(DAEMON_NAME)
	for dirs in $(DMS_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done
endif

	
	
	
	