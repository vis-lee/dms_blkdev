/*
 * Utils.c
 *
 *  Created on: Apr 9, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include "UTest_Debug.h"
#include "Utils.h"
#include "Volume_Manager.h"
//#include "Volume.h"


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

extern char *TVOL_MOD;

char *UTIL_MOD = "UTIL: ";

extern void Test_Request_Handler(struct request_queue *r_queue);
extern void Test_Commit_Func(struct request *req, struct request_queue *r_queue);

/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/





/********************************************************************************/
/*																				*/
/*							FUNCS Implementations								*/
/*																				*/
/********************************************************************************/

struct DMS_Volume * Attach_UTest_Volume(struct dms_volume_info *vinfo){

	struct DMS_Volume * volume = NULL;
	sector_t dsectors = 0;

	if(IS_LEGAL(UTIL_MOD, vinfo)){

		dsectors = CAL_NR_DMSBLKS(vinfo->capacity_in_bytes);

		//create volume, we don't use DMS_Attach_Volume due to it will add_disk to kernel and incur read op.
		volume = Create_and_Insert_DMS_Volume(vinfo->volid, dsectors);

		if( likely(volume != NULL) ) {

			//hack blk queue process function for testing, so request will be redirect to Test_Request_Handler()
			volume->r_queue->request_fn = Test_Request_Handler;

			//set the voluem state to "attached"
			__Attach_Volume(volume);

			DMS_PRINTK(TVOL_DBG, TVOL_MOD, "attach vol DONE, volumeID = %lld\n", vinfo->volid);

		} else {

			eprintk(TVOL_MOD, "ERROR! there is no available drive\n");

		}

	}

	return volume;

}


int Detach_UTest_Volume(struct dms_volume_info *vinfo){

	int retcode = -DMS_FAIL;
	struct DMS_Volume * volume = NULL;

	if(IS_LEGAL(UTIL_MOD, vinfo)){

		volume = Get_Volume_ptr_by_VolumeID(vinfo->volid);

		volume->r_queue->request_fn = Test_Request_Handler;

		DMS_Detach_Volume(vinfo->volid);

		DMS_PRINTK(TVOL_DBG, TVOL_MOD, "detach vol DONE, volumeID = %lld\n", vinfo->volid);

		retcode = DMS_OK;

	}

	return retcode;

}

