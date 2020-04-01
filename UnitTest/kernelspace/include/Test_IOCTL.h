/*
 * Test_IOCTL.h
 *
 *  Created on: Feb 14, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef TEST_IOCTL_H_
#define TEST_IOCTL_H_

#include <linux/types.h>
#include <linux/ioctl.h>

#include "DMS_IOCTL.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/



/********************************************************************************/
/*																				*/
/*								Definitions	 									*/
/*																				*/
/********************************************************************************/

/* the device name presented under /dev */

#define TEST_CONTROLLER_NAME		"dms_vdd_utest"

//#define DMS_CONTROLLER_MAJOR		151

#define DD_IOCTL_NUM				0xE0


/*
 * cmdt, the magic number is the same as dms_vdd
 */
#define IOCTL_CMDT					_IOWR(DD_IOCTL_NUM, 90, UT_Param_t *)
#define IOCTL_CTVOL					_IOWR(DD_IOCTL_NUM, 92, struct dms_volume_info *)
#define IOCTL_RTVOL					_IOWR(DD_IOCTL_NUM, 93, struct dms_volume_info *)


#define TCASE_VOLMGR				1
#define TCASE_DMP					2
#define TCASE_DLIST					3
#define TCASE_VIOHDER				4
#define TCASE_DNC_BUILDING			5


#define TTYPE_NONBLOCKING			0
#define TTYPE_BLOCKING				1


#define DECLARE_UTPARAM(NAME, TCASE)		do{UT_Param_t NAME={0, TTYPE_BLOCKING, TCASE, 100, 50, false}}while(0);


/********************************************************************************/
/*																				*/
/*								DATA STRUCTURES									*/
/*																				*/
/********************************************************************************/


typedef struct UnitTest_Param{
	unsigned long long user_param;
	unsigned int test_type;
	unsigned int test_case;
	unsigned int threads;
	unsigned int nr_elements;
	int result;
}UT_Param_t;


#endif /* TEST_IOCTL_H_ */
