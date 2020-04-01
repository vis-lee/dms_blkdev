/*
 * Test_DMS_Mem_Pool.c
 *
 *  Created on: Mar 10, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */
#include "UTest_Common.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *TDMP_MOD = "DMP_TEST: ";



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

extern struct DList_Node * Malloc_DLN ( gfp_t flags );
extern void Free_DLN(struct DList_Node *dln);

int Test_DMS_Mem_Pool(TWorker_t *tw)
{

	int retcode = -DMS_FAIL;
	Test_Workers_t * tws = NULL;

	if(IS_LEGAL(TDMP_MOD, tw)){

		struct DList_Node *dln = Malloc_DLN(GFP_KERNEL);

		tws = tw->tws_mgr;

		if( likely( IS_LEGAL(TDMP_MOD, dln)) ) {

			DMS_PRINTK(TDMP_DBG, TDMP_MOD, "alloc DONE, dln = %p \n", dln);

			msleep(500);

			Free_DLN(dln);

			retcode = DMS_OK;

			DMS_PRINTK(TDMP_DBG, TDMP_MOD, "free my DLN DONE, test end! \n");

		} else {

			eprintk(TDMP_MOD, "ERROR! get NULL ptr! retcode = %ld \n", PTR_ERR(dln));
			retcode = -ENOMEM;

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


