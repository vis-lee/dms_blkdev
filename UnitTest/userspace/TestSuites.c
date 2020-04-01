/*
 * TestSuites.c
 *
 *  Created on: 2011/7/22
 *      Author: 980263
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/ioctl.h>
#include <netdb.h>
#include <syslog.h>
#include <sysexits.h>
#include <unistd.h>
//#include <fcntl.h>
//#include <sys/stat.h>

//#include "DMS_Common.h"
#include "DMS_IOCTL.h"
#include "Test_IOCTL.h"
#include "Client_Daemon.h"
#include "DMS_Error.h"

/********************************************************************************/
/*																				*/
/*							DEFINITIONS 										*/
/*																				*/
/********************************************************************************/

#define MB					1000*1000

#define HOST_NAME_LEN 		20
#define PATH_CHAR_LEN		32

#define CMD_CHAR_LEN		256

#define INIT_G_CMD()					g_cmd = (char *)malloc(CMD_CHAR_LEN)
#define SET_G_CMD(fmt...)				do{ memset(g_cmd, 0, CMD_CHAR_LEN); sprintf(g_cmd, fmt); } while(0)
#define EXE_G_CMD()						system(g_cmd)



/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

static char *DEVICE_NAME_PREFIX =	"dms";

char *dms_vdd_path;
char *dms_utest_path;

char *g_cmd;

char *appname;


int Get_Namenode_IP_Str(char *nn_ip_addr){

	struct hostent *nn_hostent = NULL;
	struct sockaddr_in nn_Addr = { 0 };

	unsigned char *ucp = NULL;

	nn_hostent = (struct hostent *) gethostbyname(NamenodeHostname);

	if (nn_hostent == NULL)
	{
		syslog(LOG_ERR, "cannot get ip from hostname : %s\n", NamenodeHostname);
		exit(1);
	}

	memcpy(&(nn_Addr.sin_addr.s_addr), nn_hostent->h_addr, nn_hostent->h_length);

	ucp = (unsigned char *) &nn_Addr.sin_addr;

	sprintf(nn_ip_addr, "%d.%d.%d.%d", ucp[0] & 0xff, ucp[1] & 0xff, ucp[2] & 0xff, ucp[3] & 0xff);
	printf("%s, The namenode ip addr is %s\n", __func__, nn_ip_addr);

	return 0;
}


int Get_Namenode_IP_Int(char *hostname){

	struct hostent *nn_hostent = NULL;
//	struct sockaddr_in nn_Addr = { 0 };
	int ret_ip = 0;

	unsigned char *ucp = NULL;

	nn_hostent = (struct hostent *) gethostbyname(hostname);

	if (nn_hostent == NULL)
	{
		syslog(LOG_ERR, "cannot get ip from hostname : %s\n", hostname);
		exit(1);
	}

//	memcpy(&(nn_Addr.sin_addr.s_addr), nn_hostent->h_addr, nn_hostent->h_length);

//	ucp = (unsigned char *) &nn_Addr.sin_addr;

	ret_ip = ntohl( *((__be32 *)(nn_hostent->h_addr)) );

	ucp = (unsigned char *) (&ret_ip);


	printf("%s, The namenode ip addr is %d.%d.%d.%d\n",
			__func__, ucp[3] & 0xff, ucp[2] & 0xff, ucp[1] & 0xff, ucp[0] & 0xff);

	return ret_ip;
}



void set_env_path(){

	dms_vdd_path = (char *)malloc(PATH_CHAR_LEN);
	dms_utest_path = (char *)malloc(PATH_CHAR_LEN);

	INIT_G_CMD();

	memset(dms_vdd_path, 0, PATH_CHAR_LEN);
	memset(dms_utest_path, 0, PATH_CHAR_LEN);

	sprintf(dms_vdd_path, "/dev/%s", DMS_CONTROLLER_NAME);
	sprintf(dms_utest_path, "/dev/%s", TEST_CONTROLLER_NAME);

}



int check_dms_vdd(){

	int dev_fp = 0;

	//check dms_vdd
	if( (dev_fp = open(dms_vdd_path, O_RDWR)) != -1 ){

		close(dev_fp);

	}

	return dev_fp;
}


int check_dms_utest(){

	int dev_fp = 0;

	//check dms_vdd_utest
	if( (dev_fp = open(dms_utest_path, O_RDWR)) != -1 ){

		close(dev_fp);

	}

	return dev_fp;
}

int check_test_env(){

	int retcode = 0;

	//check dms_vdd
	if( (retcode = check_dms_vdd()) == -1 ){

		return retcode;

	}

	//check dms_vdd_utest
	if( (retcode = check_dms_utest()) == -1 ){

		return retcode;

	}

	return 0;
}


int build_test_env(){

	int retcode = 0;

	set_env_path();

	//check dms_vdd
	//if( (dev_fp = open(dms_vdd_path, O_RDWR)) == -1 ){
	if( (retcode = check_dms_vdd()) == -1 ){

		//not exist, load module first
		SET_G_CMD("./%s &", DMSD_NAME);

		syslog(LOG_INFO, "%s not exist, loading by command: %s\n", DMS_DEV_NAME, (char *)g_cmd);

		EXE_G_CMD();

		sleep(3);

		//check load result
		if( (retcode = check_dms_vdd()) == -1 ){

			return retcode;

		}

	}

	//check dms_vdd_utest
	if( (retcode = check_dms_utest()) == -1 ){

		//not exist, load module first
		SET_G_CMD("insmod ./%s.ko; chmod 666 %s;", TEST_CONTROLLER_NAME, dms_utest_path);

		syslog(LOG_INFO, "%s not exist, loading by command: %s \n", TEST_CONTROLLER_NAME, (char *)g_cmd);

		EXE_G_CMD();

		//check load result
		if( (retcode = check_dms_utest()) == -1 ){

			return retcode;

		}
	}

	return 0;
}

int destroy_env(){

	if(dms_vdd_path){
		free(dms_vdd_path);
	}

	if(dms_utest_path){
		free(dms_utest_path);
	}



	return 0;
}

#if NECESSARY
int Test_VDD_IOCTL(){

	int retcode = DMS_OK;

	int vdd_fp = 0;
	struct dms_volume_info vinfo = {123456, 1000, 0};

	/* test vdd IOCTL */
	vdd_fp = open(dms_vdd_path, O_RDWR);

	if(vdd_fp > 0){

		int retcode = 0;

		retcode = ioctl(vdd_fp, IOCTL_ATTACH_VOL, &vinfo);

		syslog(LOG_INFO, "Unit Test: dev_name = %s, IOCTL_ATTACH_VOL result = %d\n", dms_vdd_path, retcode);

	} else {

		syslog(LOG_ERR, "Unit Test: open fail! dev_name = %s\n", dms_vdd_path);
	}

	close(vdd_fp);

	return retcode;
}
#endif


int Attach_UTest_Volume(long long volumeID, unsigned long long volumeCap, int type){

	int retcode = DMS_OK;

	int utest_fp = 0;
	struct dms_volume_info vinfo = {volumeID, volumeCap, 0, type};

	/* test vdd IOCTL */
	utest_fp = open(dms_utest_path, O_RDWR);

	if(utest_fp > 0){

		retcode = ioctl(utest_fp, IOCTL_CTVOL, &vinfo);

		syslog(LOG_INFO, "Unit Test: volumeID = %lld, volcap = %llu, IOCTL_CTVOL result = %d\n",
				vinfo.volid, vinfo.capacity_in_bytes, retcode);

		close(utest_fp);

	} else {

		syslog(LOG_ERR, "Unit Test: create fail! dev_name = %s\n", dms_utest_path);
	}

	return retcode;
}

int Create_UTest_Volumes(long long sVolumeID, int nr_volumes){

	int retcode = DMS_OK;

	int i = 0;

	for( i = 0; i < nr_volumes; i++){

		retcode = Attach_UTest_Volume(sVolumeID+i, 1000*MB, 0);

	}

	sleep(1);

	return retcode;
}

int Create_Simple_Volume(long long volumeID, unsigned long long volumeCap){

	return Attach_UTest_Volume(volumeID, volumeCap, 0);
}

int Create_vIO_Volume(long long volumeID, unsigned long long volumeCap){

	return Attach_UTest_Volume(volumeID, volumeCap, 1);
}


int Detach_UTest_Volume(long long volumeID, int type){

	int retcode = DMS_OK;

	int utest_fp = 0;
	struct dms_volume_info vinfo = {volumeID, 0, 0, type};

	/* test vdd IOCTL */
	utest_fp = open(dms_utest_path, O_RDWR);

	if(utest_fp > 0){

		retcode = ioctl(utest_fp, IOCTL_RTVOL, &vinfo);

		syslog(LOG_INFO, "%s: volumeID = %lld, IOCTL_RTVOL result = %d\n",
						__func__, vinfo.volid, retcode);

		close(utest_fp);

	} else {

		syslog(LOG_ERR, "Unit Test: release fail! volumeID = %lld\n", volumeID);
	}

	syslog(LOG_INFO, "Unit Test, %s: result = %d\n",
					__func__, retcode);

	return retcode;
}


int Release_UTest_Volumes(long long sVolumeID, int nr_volumes){

	int retcode = DMS_OK;

	int i = 0;

	for( i = 0; i < nr_volumes; i++){


		retcode = Detach_UTest_Volume(sVolumeID+i, 0);


	}


	return retcode;
}

int Release_Simple_Volume(long long volumeID){

	return Detach_UTest_Volume(volumeID, 0);
}

int Release_vIO_Volume(long long volumeID){

	return Detach_UTest_Volume(volumeID, 1);
}



int Test_Volume_Manager(){

	int retcode = DMS_OK;

	int utest_fp = 0;
	UT_Param_t testdata = {0, TTYPE_BLOCKING, TCASE_VOLMGR, 100, 0, 0};

	/* test utest IOCTL */
	utest_fp = open(dms_utest_path, O_RDWR);

	if(utest_fp > 0){

		ioctl(utest_fp, IOCTL_CMDT, &testdata);

		syslog(LOG_INFO, "Unit Test: dev_name = %s, IOCTL_CMDT result = %d\n", dms_utest_path, testdata.result);

		sleep(1);
		close(utest_fp);

	} else {

		syslog(LOG_ERR, "Unit Test: open fail! dev_name = %s\n", dms_utest_path);
	}

	syslog(LOG_INFO, "%s: dev_name = %s, result = %d \n", __func__, dms_utest_path, testdata.result);

	printf("%s end, result = %d\n", __func__, testdata.result);

	return retcode;

}

int Test_DMS_Mem_Pool(){

	int retcode = DMS_OK;

	int utest_fp = 0;
	UT_Param_t testdata = {0, TTYPE_BLOCKING, TCASE_DMP, 100, 0, 0};

	/* test utest IOCTL */
	utest_fp = open(dms_utest_path, O_RDWR);

	if(utest_fp > 0){

		ioctl(utest_fp, IOCTL_CMDT, &testdata);

		syslog(LOG_INFO, "Unit Test: dev_name = %s, IOCTL_CMDT result = %d\n", dms_utest_path, testdata.result);

		sleep(1);
		close(utest_fp);

	} else {

		syslog(LOG_ERR, "Unit Test: open fail! dev_name = %s\n", dms_utest_path);
	}

	syslog(LOG_INFO, "%s: dev_name = %s, result = %d \n", __func__, dms_utest_path, testdata.result);

	printf("%s end, result = %d\n", __func__, testdata.result);

	return retcode;

}


int Test_DMS_DList(){

	int retcode = DMS_OK;

	int utest_fp = 0;
	UT_Param_t testdata = {0, TTYPE_BLOCKING, TCASE_DLIST, 1, 50, 0};

	/* test utest IOCTL */
	utest_fp = open(dms_utest_path, O_RDWR);

	if(utest_fp > 0){

		ioctl(utest_fp, IOCTL_CMDT, &testdata);

		syslog(LOG_INFO, "Unit Test: dev_name = %s, IOCTL_CMDT result = %d\n", dms_utest_path, testdata.result);

		sleep(5);

		/* multi-thread testing */
		testdata.threads = 3;
		ioctl(utest_fp, IOCTL_CMDT, &testdata);

		sleep(1);
		close(utest_fp);

	} else {

		syslog(LOG_ERR, "Unit Test: open fail! dev_name = %s\n", dms_utest_path);
	}

	syslog(LOG_INFO, "%s: dev_name = %s, result = %d \n", __func__, dms_utest_path, testdata.result);

	printf("%s end, result = %d\n", __func__, testdata.result);

	return retcode;

}


int Test_vIO_Handler(){

	int retcode = DMS_OK, status = 0;
	pid_t childpid;

	int utest_fp = 0;
	UT_Param_t testdata = {1, TTYPE_BLOCKING, TCASE_VIOHDER, 1, 50, 0};

	//create volume
	Create_UTest_Volumes(testdata.user_param, 1);

	/* test utest IOCTL */
	utest_fp = open(dms_utest_path, O_RDWR);

	if(utest_fp > 0){

		childpid = fork();

		if( childpid >= 0 ){

			//child process
			if(childpid == 0){

				ioctl(utest_fp, IOCTL_CMDT, &testdata);
				syslog(LOG_INFO, "Unit Test: dev_name = %s, TCASE_VIOHDER result = %d\n", dms_utest_path, testdata.result);

				exit(0);

			}else{

				//wait for prepare test env
				sleep(3);

				syslog(LOG_INFO, "\n/***************************************/ DO dd Test /**************************************/\n");

				//do dd test
				SET_G_CMD("dd if=/dev/urandom of=/dev/%s%lld bs=4M count=100;", DEVICE_NAME_PREFIX, (long long)testdata.user_param);

				EXE_G_CMD();

				//do dd test
				SET_G_CMD("sync");

				EXE_G_CMD();

				//wait for childp exit
				wait(&status);

			}

		} else {

			syslog(LOG_ERR, "fork return error = %d \n", childpid);
		}

		/* multi-thread testing */
//		testdata.threads = 3;
//		ioctl(utest_fp, IOCTL_CMDT, &testdata);

		sleep(1);
		close(utest_fp);

	} else {

		syslog(LOG_ERR, "Unit Test: open fail! dev_name = %s\n", dms_utest_path);
	}

	Release_UTest_Volumes((long long)testdata.user_param, 1);

	syslog(LOG_INFO, "%s: dev_name = %s result = %d \n", __func__, dms_utest_path, testdata.result);

	printf("%s end, result = %d\n", __func__, testdata.result);

	return retcode;

}


int Test_DNC(){

	int retcode = DMS_OK, status = 0;
	char n_ip[HOST_NAME_LEN];

	int utest_fp = 0, nn_ip = 0;

//	Get_Namenode_IP_Str(&n_ip);
	nn_ip = Get_Namenode_IP_Int(NamenodeHostname);

	//#we user the result to record port number and nr_elements to mention time out period.
	UT_Param_t testdata = {(unsigned long long)nn_ip, TTYPE_BLOCKING, TCASE_DNC_BUILDING, 2, 30/*sec*/, 1234};

	/* test utest IOCTL */
	utest_fp = open(dms_utest_path, O_RDWR);

	if(utest_fp > 0){

		ioctl(utest_fp, IOCTL_CMDT, &testdata);

		syslog(LOG_INFO, "Unit Test: %s, dev_name = %s, utest_fp = %d, result = %d\n", __func__, dms_utest_path, utest_fp, testdata.result);

		sleep(1);
		retcode = close(utest_fp);

		syslog(LOG_INFO, "\n/***************************************/ close file, retcode = %d /**************************************/\n", retcode );

		SET_G_CMD("echo Test_DNC:; lsmod | grep dms");
		EXE_G_CMD();

	} else {

		syslog(LOG_ERR, "Unit Test: %s, open fail! dev_name = %s\n", __func__, dms_utest_path);
	}

	syslog(LOG_INFO, "%s: end~ dev_name = %s result = %d \n", __func__, dms_utest_path, testdata.result);

	printf("%s end, result = %d\n", __func__, testdata.result);

	return retcode;

}


int Test_DNC_Error_Handling(){

	int retcode = DMS_OK, status = 0;
	pid_t childpid;
	char n_ip[HOST_NAME_LEN];

	int utest_fp = 0, nn_ip = 0;

	//Get_Namenode_IP_Str(&n_ip);
	nn_ip = Get_Namenode_IP_Int(NamenodeHostname);

	//#we use the result to record port number and nr_elements to mention testcase time out period.
	UT_Param_t testdata = {(unsigned long long)nn_ip, TTYPE_BLOCKING, TCASE_DNC_BUILDING, 2, 60/*sec*/, 1234};

	/* test utest IOCTL */
//	utest_fp = open(dms_utest_path, O_RDWR);

//	if(utest_fp > 0){

		childpid = fork();

		if( childpid >= 0 ){

			//child process
			if(childpid == 0){

				SET_G_CMD("echo Test_DNC_Error_Handling : before IOCTL; lsmod | grep dms");
				EXE_G_CMD();

				//open file here, or some wired thing happened when close file
				utest_fp = open(dms_utest_path, O_RDWR);

				ioctl(utest_fp, IOCTL_CMDT, &testdata);

				syslog(LOG_INFO, "Unit Test: %s, IOCTL, dev_name = %s, result = %d\n", __func__, dms_utest_path, testdata.result);

				SET_G_CMD("echo Test_DNC_Error_Handling : after IOCTL; lsmod | grep dms");
				EXE_G_CMD();


				syslog(LOG_INFO, "\n/***************************************/ close file, childpid = %d, utest_fp = %d /**************************************/\n", childpid, utest_fp);
				retcode = close(utest_fp);

				SET_G_CMD("echo Test_DNC_Error_Handling : after close; lsmod | grep dms");
				EXE_G_CMD();

				exit(0);

			} else {

				//wait for prepare test env
				sleep(10);

				SET_G_CMD("echo Test_DNC_Error_Handling: befor ifdown; lsmod | grep dms");
				EXE_G_CMD();
				syslog(LOG_INFO, "\n/***************************************/ ifdown Test /**************************************/\n");

				//do ifdown test
				SET_G_CMD("ifdown eth0");
				EXE_G_CMD();

				sleep(30);
				SET_G_CMD("echo Test_DNC_Error_Handling: befor ifup; lsmod | grep dms");
				EXE_G_CMD();

				syslog(LOG_INFO, "\n/***************************************/ ifup Test /**************************************/\n");
				SET_G_CMD("ifup eth0");
				EXE_G_CMD();

				SET_G_CMD("echo Test_DNC_Error_Handling: after ifup; lsmod | grep dms");
				EXE_G_CMD();

				//wait for childp exit
				wait(&status);

				syslog(LOG_INFO, "\n/***************************************/ IOCTL exit /**************************************/\n");

			}

		} else {

			syslog(LOG_ERR, "fork return error = %d \n", childpid);
		}


//	} else {
//
//		syslog(LOG_ERR, "Unit Test: %s, open fail! dev_name = %s\n", __func__, dms_utest_path);
//	}

	syslog(LOG_INFO, "%s: end~ dev_name = %s, result = %d \n", __func__, dms_utest_path, testdata.result);

	printf("%s end, result = %d\n", __func__, testdata.result);

	return retcode;

}

static void usage() {

	(void)fprintf(stderr,
			"usage: %-16s [-c volume_capacity(in MB)] [-t (run all tests)] \n\n", appname);

	exit(EX_USAGE);
}

int Exe_Test_Suite(){

	int retcode = DMS_OK;

//	if(build_test_env() < 0){
//		syslog(LOG_ERR, "build test environment fail!");
//		exit(1);
//	}

#if NECESSARY
	Test_VDD_IOCTL();
#endif


	Test_Volume_Manager();

	Test_DMS_Mem_Pool();

	Test_DMS_DList();

	sleep(20);

	//TODO Test_Allocation_Flag();

	Test_vIO_Handler();

	Test_DNC();

	Test_DNC_Error_Handling();

	destroy_env();

	syslog(LOG_INFO, "\n/***************************************/ UTest exit /**************************************/\n");

	return retcode;
}


int main(int argc, char **argv){

	int retcode = DMS_OK;
	int ch = 0;

	appname = *argv;

	if(build_test_env() < 0){
		syslog(LOG_ERR, "build test environment fail!");
		exit(1);
	}


	while ((ch = getopt(argc, argv, "c:tr:")) != -1) {

		switch(ch) {

			case 'c':
			{
				//TODO volumeID should create and get from namenode
				srand(time(0));
				long long volumeID = (long long)rand();
				long long size = atoll(optarg);
				//create test volume;
				retcode = Create_vIO_Volume(volumeID, size*MB);

			}
				break;

			case 'r':
			{
				long long volid = atoll(optarg);
				retcode = Release_vIO_Volume(volid);
			}
				break;

			case 't':
				retcode = Exe_Test_Suite();
				break;

			case '?':
			default:
				usage();
				/* NOTREACHED */
		}
	}

    if (optind < argc) {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        printf ("\n");
    }

	destroy_env();

	syslog(LOG_INFO, "\n/***************************************/ UTest exit /**************************************/\n");


	return retcode;

}

