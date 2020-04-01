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

char *dms_vdd_path;
char *dms_utest_path;

char *g_cmd;


//int Get_Namenode_IP(char *nn_ip_addr){
//
//	struct hostent *nn_hostent = NULL;
//	struct sockaddr_in nn_Addr = { 0 };
//
//	unsigned char *ucp = NULL;
//
//	nn_hostent = (struct hostent *) gethostbyname(NamenodeHostname);
//
//	if (nn_hostent == NULL)
//	{
//		syslog(LOG_ERR, "cannot get ip from hostname : %s\n", NamenodeHostname);
//		exit(1);
//	}
//
//	memcpy(&(nn_Addr.sin_addr.s_addr), nn_hostent->h_addr, nn_hostent->h_length);
//
//	ucp = (unsigned char *) &nn_Addr.sin_addr;
//
//	sprintf(nn_ip_addr, "%d.%d.%d.%d", ucp[0] & 0xff, ucp[1] & 0xff, ucp[2] & 0xff, ucp[3] & 0xff);
//	printf("The namenode ip addr is %s\n", nn_ip_addr);
//
//	return 0;
//}

void set_env_path(){

	dms_vdd_path = (char *)malloc(PATH_CHAR_LEN);
	dms_utest_path = (char *)malloc(PATH_CHAR_LEN);

	INIT_G_CMD();

	memset(dms_vdd_path, 0, PATH_CHAR_LEN);
	memset(dms_utest_path, 0, PATH_CHAR_LEN);

	sprintf(dms_vdd_path, "/dev/%s", DMS_CONTROLLER_NAME);
	sprintf(dms_utest_path, "/dev/%s", TEST_CONTROLLER_NAME);

}

int build_test_env(){

	int dev_fp = 0;

	set_env_path();

	//check dms_vdd
	if( (dev_fp = open(dms_vdd_path, O_RDWR)) == -1 ){

		//not exist, load module first
		SET_G_CMD("./%s &", DMSD_NAME);

		syslog(LOG_INFO, "%s not exist, loading by command: %s\n", DMS_DEV_NAME, (char *)g_cmd);

		EXE_G_CMD();

		sleep(3);

	}else{

		close(dev_fp);

	}

//	//check dms_vdd_utest
//	if( (dev_fp = open(dms_utest_path, O_RDWR)) == -1 ){
//
//		//not exist, load module first
//		SET_G_CMD("insmod ./%s.ko; chmod 666 %s;", TEST_CONTROLLER_NAME, dms_utest_path);
//
//		syslog(LOG_INFO, "%s not exist, loading by command: %s \n", TEST_CONTROLLER_NAME, (char *)g_cmd);
//
//		EXE_G_CMD();
//
//	}else{
//
//		close(dev_fp);
//
//	}

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


int Create_Test_Volumes(){

	int retcode = DMS_OK;

	int vdd_fp = 0;
	struct dms_volume_info vinfo = {123456, 1000*MB, 0};

	/* test vdd IOCTL */
	vdd_fp = open(dms_vdd_path, O_RDWR);

	if(vdd_fp > 0){

		int i = 0;

		for( i=0; i<10; i++){

			int retcode = 0;

			vinfo.volid = i+1;

			retcode = ioctl(vdd_fp, IOCTL_ATTACH_VOL, &vinfo);

			syslog(LOG_INFO, "Unit Test: dev_name = %s, IOCTL_ATTACH_VOL result = %d\n", dms_vdd_path, retcode);

		}

	} else {

		syslog(LOG_ERR, "Unit Test: open fail! dev_name = %s\n", dms_vdd_path);
	}

	close(vdd_fp);

	return retcode;
}



int main(int argc, char **argv){

	int retcode = DMS_OK;

	if(build_test_env() < 0){
		syslog(LOG_ERR, "build test environment fail!");
		exit(1);
	}


	Create_Test_Volumes();


	destroy_env();


	return retcode;

}

