
#################################
#	DMS VDD Distribution		#
#################################

.PHONY:clean

#################################
#		Global configs			#
#################################
DMS_SUBDIRS		= DriverCore UserDaemon
BIN_DIR			= bin
UTEST_DIR		= ./UnitTest
ALL_SUBDIRS		= $(DMS_SUBDIRS) $(UTEST_DIR)

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
	@echo "default target is release build"
	for dirs in $(DMS_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done
	#copy files to bin_dir should triggered from here or pass the BIN_DIR to subdirs
	
check:
	for dirs in $(DMS_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done

dbg:
	for dirs in $(DMS_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done

phy:
	for dirs in $(DMS_SUBDIRS); do \
		(cd $$dirs && $(MAKE) $@); \
    done
    
utest: all
	@echo "utest target build all additional unit test modules!"
	cd $(UTEST_DIR) && $(MAKE) all;
    
clean:
	@echo "Clean ALL..."
	for dirs in $(ALL_SUBDIRS); do	\
		(cd $$dirs && $(MAKE) $@);	\
    done;							\
    rm -f $(BIN_DIR)/**;




	
	