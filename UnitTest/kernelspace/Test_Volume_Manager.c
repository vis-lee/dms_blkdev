/*
 * Test_Volume_Manager.c
 *
 *  Created on: 2011/7/18
 *      Author: 980263
 */

#include "Volume_Manager.h"
#include "UTest_Common.h"


char *TVOL_MOD = "VMGR_Test: ";


extern void Test_Request_Handler(struct request_queue *r_queue);
extern void Test_Commit_Func(struct request *req, struct request_queue *r_queue);

int Test_Volume_Manager(TWorker_t *tw)
{

	int retcode = -DMS_FAIL;
	Test_Workers_t *tws = NULL;

	if(IS_LEGAL(TVOL_MOD, tw)){

		long64 volumeID = tw->tid;
		sector_t capacity = 10000;
		struct DMS_Volume *volume = NULL;

		tws = tw->tws_mgr;

		DMS_PRINTK(TVOL_DBG, TVOL_MOD, "start~!\n");

		//create volume, we don't use DMS_Attach_Volume due to it will add_disk to kernel and incur read op.
		volume = Create_and_Insert_DMS_Volume(volumeID, capacity);

		if( likely(volume != NULL) ) {

			//hack blk queue process function for testing, so request will be redirect to Test_Request_Handler()
			volume->r_queue->request_fn = Test_Request_Handler;

			//set the voluem state to "attached"
			retcode = __Attach_Volume(volume);

			DMS_PRINTK(TVOL_DBG, TVOL_MOD, "attach vol DONE, volumeID = %lld\n", volumeID);

		} else {

			eprintk(TVOL_MOD, "ERROR! there is no available drive\n");
			retcode = -ATTACH_FAIL;

		}


		msleep(500);

		if (Check_Volume_Existence(volumeID))
		{

			Remove_and_Release_DMS_Volume(volume);

		}
		else
		{

			//eprintk("%s%s, My volume isn't exist!!\n", TVOL_MOD, __func__);
		}

		tws->done_func(tw, retcode);

	}


	return retcode;

}

#if 0
int Setup__Env(unsigned long param){

	return DMS_OK;

}

void Teardown__Env(unsigned long param){

}

int Verify_Test__Result(unsigned long param){

	int retcode = DMS_OK;

	return retcode;
}
#endif

#if 0
/*
 * main function for testing
 */
int IOCTL_Test_Volume_Manager(UT_Param_t *test_param)
{

	DMS_PRINTK(TVOL_DBG, "%s%s start~!\n", TVOL_MOD, __func__);

	test_param->result = true;

	DMS_PRINTK(TVOL_DBG, "%s%s done~!", TVOL_MOD, __func__);

	return 0;
}
#endif

//EXPORT_SYMBOL(IOCTL_Test_Volume_Manager);
EXPORT_SYMBOL(Test_Volume_Manager);
