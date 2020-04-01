/*
 * Test_vIO_Handler.c
 *
 *  Created on: Apr 9, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include "DMS_Common.h"
#include "Test_Workers.h"

#include "Driver_Core.h"
#include "DIO.h"
#include "IO_Request.h"
#include "Metadata_Manager.h"
#include "Volume_Manager.h"
#include "DList.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *TVIO_MOD = "vIO_TEST: ";



/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/

#define DEFAULT_TEST_SLEEP_TIME 	10*1000

extern void DMS_Request_Handler(struct request_queue *r_queue);
extern void DMS_End_Request_Handler(struct request *kreq, unsigned long nsectors, int result);

extern void Test_Request_Handler(struct request_queue *r_queue);
extern void Test_Commit_Func(struct request *req, struct request_queue *r_queue);

/********************************************************************************/
/*																				*/
/*							FUNCS Implementations								*/
/*																				*/
/********************************************************************************/


int Test_vIO_Handler(TWorker_t *tw){

	int retcode = -DMS_FAIL;

	Test_Workers_t *tws = NULL;

	if(IS_LEGAL(TVIO_MOD, tw)){

		tws = tw->tws_mgr;

		//sleep enogh time for dd testing
		msleep(DEFAULT_TEST_SLEEP_TIME);
	}

	tws->done_func(tw, DMS_OK);

	return retcode;
}



int __UTest_Commit_All_LBs(struct DMS_IO *dio, ulong64 lbid, int result){

	int retcode = -DMS_FAIL;

	ulong64 sLBID = 0, nr_LBIDs = 0;
	int i = 0;

	if(IS_LEGAL(TVIO_MOD, dio)){

		sLBID = dio->sLBID;

		nr_LBIDs = dio->nr_LBIDs;

//		for( i = 0; i < nr_LBIDs; i++ ){

			//commit to dio
			retcode = Commit_LB_to_DMS_IO( dio, sLBID, nr_LBIDs, result );

//		}
	}


	return retcode;
}


/**
 * __VolumeIO_Handler - per volume request entry point
 * @param volume
 * @param kreq
 * @return
 */
int __UTest_VolumeIO_Handler(struct DMS_Volume *volume, struct request *kreq){

	int retcode = -DMS_FAIL;
	struct DMS_IO *dio = NULL;

	ulong64 rid = 0;
	char dbg_str[PAGE_SIZE];

	if(IS_LEGAL(TVIO_MOD, volume) &&
			IS_LEGAL(TVIO_MOD, kreq) )
	{
		struct IO_Request *ior = __Make_DIO_Requst(kreq);

		Add_IO_Reqeust_to_KRequest(kreq, ior);

		rid = ior->rid;

		memset(&dbg_str, 0, PAGE_SIZE);
		SPrint_IO_Request(&dbg_str, PAGE_SIZE-512, ior);

		printk("%s: {\t%s\t}\n", __func__, dbg_str);

		printk("%s: {\tcurrent: %s\t}\n", __func__, &current->comm);


		//go back to head
		Reset_IO_Request_Iterator(ior);

		while( (dio = Get_Next_DIO_from_IO_Request(ior)) )
		{
			//self checking
			if(ior->rid == rid){
				//commit direct
				//__UTest_VolumeIO_Commiter(dio, true);
				retcode = __UTest_Commit_All_LBs(dio, dio->sLBID, true);

				if(retcode == true){

					iprintk(TVIO_MOD, "this ior has been end-ed! \n");
					break;
				}
			}
		}

		retcode = DMS_OK;
	}



	return retcode;
}



/********************************************************************************/
/*																				*/
/*									Env APIs									*/
/*																				*/
/********************************************************************************/

int Setup_Test_vIO_Env(unsigned long param){

	long long volumeID = (long long)param;

	//find volume
	struct DMS_Volume *dv = Get_Volume_ptr_by_VolumeID(volumeID);

	if(IS_LEGAL(TVIO_MOD, dv)){

		dv->r_queue->request_fn = DMS_Request_Handler;

		//hack vIO_Handler
		dv->vIO_Handler = __UTest_VolumeIO_Handler;
		dv->vIO_Commiter = __UTest_Commit_All_LBs;

		return DMS_OK;
	}


	return -DMS_FAIL;

}

void Teardown_Test_vIO_Env(unsigned long param){

	long long volumeID = (long long)param;

	//find volume
	struct DMS_Volume *dv = Get_Volume_ptr_by_VolumeID(volumeID);

	if(IS_LEGAL(TVIO_MOD, dv)){

		dv->r_queue->request_fn = Test_Request_Handler;

		//recover vIO_Handler
		dv->vIO_Handler = __VolumeIO_Handler;
		dv->vIO_Commiter = __VolumeIO_Commiter;

	}

}

int Verify_Test_vIO_Result(unsigned long param){

	int retcode = DMS_OK;



	return retcode;
}



