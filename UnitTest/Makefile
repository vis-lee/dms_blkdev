
#################################
#	DMS VDD Distribution		#
#################################

.PHONY:clean

#################################
#		Global configs			#
#################################
UT_SUBDIRS		= kernelspace userspace
DMS_VDD_DIR		= $(PWD)/../DriverCore
BIN_DIR			= ../bin

#todo bin dir should be checked here.
#if test [ -d $(BIN_DIR) ]; else $(shell mkdir -p $(BIN_DIR)); fi

#################################
#			ENV PATH			#
#################################
#RS_LIB_HEADER		= rs_include
#RS_LIB_PATH		= /usr/cloudos/common/lib/rs/libccma_c_rs_api.so



#################################
#			MAKE TAGS			#
#################################

all:
	#dms_vdd_utest: chech Module.symvers of dms_vdd first, if doesn't exist, make it
	cd $(DMS_VDD_DIR) && $(MAKE) $@;
	cp -u $(DMS_VDD_DIR)/Module.symvers ./kernelspace;	\
	for dirs in $(UT_SUBDIRS); do						\
		(cd $$dirs && $(MAKE) $@);						\
    done;						   						\
	
check:
	for dirs in $(UT_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done

	
dbg:
	for dirs in $(UT_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done
	
phy:
	for dirs in $(UT_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done	
	
clean:
	@echo "Clean ALL..."
	for dirs in $(UT_SUBDIRS); do	\
		(cd $$dirs && $(MAKE) $@);	\
    done;							\





	
	