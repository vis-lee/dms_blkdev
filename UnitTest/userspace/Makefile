
#################################
#	Unitest Daemon MAKEFILE		#
#################################

.PHONY:clean

#################################
#		Global configs			#
#################################
PROC_NAME			= dms_vdd_utest
BIN_DIR				= ../../bin

UnitTest_INCLUDE 	= -I/usr/include -I../kernelspace/include -I../../DriverCore/include -I../../UserDaemon/include
UnitTest_CFLAGS	 	= -Wall

#UnitTest
DMS_TestSuites_SRC = TestSuites.c
DMS_TestSuites_OBJ = TestSuites.o


#################################
#			ENV PATH			#
#################################


#################################
#			BUILD targets		#
#################################

all:
	$(CC) -o $(PROC_NAME) $(DMS_TestSuites_SRC) -I $(UnitTest_INCLUDE) $(UnitTest_CFALGS)
	$(CC) -o CreateVolume CreateTestVolume.c -I $(UnitTest_INCLUDE) $(UnitTest_CFALGS)
	cp -u CreateVolume $(BIN_DIR)
	cp -u $(PROC_NAME) $(BIN_DIR)
	
clean:
	rm -rf *.o $(PROC_NAME) CreateVolume