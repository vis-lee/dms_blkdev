
#################################
#	User Daemon MAKEFILE		#
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
RS_LIB_HEADER	= ../rs_include
RS_LIB_PATH		= /usr/cloudos/common/lib/rs/libccma_c_rs_api.so



#################################
#		daemon 	configs			#
#################################
DAEMON_NAME		= dmsd
DMSD_SRCS		= Client_Daemon_Skeleton.c Client_Daemon.c
DMSD_OBJS		= Client_Daemon_Skeleton.o Client_Daemon.o

DMS_DRIVRECORE	= ../DriverCore/include

DMSD_INCLUDES	= -I/usr/include -I$(RS_LIB_HEADER) -Iinclude -I$(DMS_DRIVRECORE)
DMSD_LIBS		= $(RS_LIB_PATH)

DMSD_FLAGS	= -Wall -g $(DMSD_INCLUDES) $(DMSD_LIBS) -lpthread

#----no_rs flags----#
DMSD_CFLAGS		= $(DMSD_FLAGS)

#---- rs ----#
DMSD_RS_CFLAGS	= $(DMSD_CFLAGS) -DRS_SUPPORT=1



#################################
#	dmsd daemon on user space	#
#################################
all:
	echo "!!do nothing but copy dmsd to bin dir for now... remember take me back when you finish kernel devel..."
#	$(CC) -o $(DAEMON_NAME) $(DMSD_SRCS) $(DMSD_RS_CFLAGS)
	cp -u $(DAEMON_NAME) $(BIN_DIR)
	
dmsd_nors:
	$(CC) -o $(DAEMON_NAME) $(DMSD_SRCS) $(DMSD_CFLAGS)
	
dmsd:
	$(CC) -o $(DAEMON_NAME) $(DMSD_SRCS) $(DMSD_RS_CFLAGS)

utest:
	(cd $(UTEST_SRCDIR); $(MAKE); cd "..")
	
	
clean:
	echo "!!do nothing for now... remember take me back when you finish kernel devel..."
#	rm -rf *.o *.ko *.symvers *.mod.c .*.cmd Module.markers modules.order .tmp_versions version.h
#	rm -rf $(DAEMON_NAME)
#	for dirs in $(DMS_SUBDIRS); do \
#		(cd $$dirs && $@); \
#    done

    
    
    
    
    
    
    
    
    
    
    
    