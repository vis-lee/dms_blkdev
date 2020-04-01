/*
 * Test_Namenode_Protocol.c
 *
 *  Created on: May 18, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include <linux/random.h>

#include "DMS_Common.h"
#include "UTest_Common.h"


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

static char *T_MOD = "TNNP: ";



/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/


#define NR_HBID_RANGE			KSECTORS_TO_DSECTORS(MAX_KSECTORS)
#define NR_TRIPLETS_RANGE		KSECTORS_TO_DSECTORS(MAX_KSECTORS)


/********************************************************************************/
/*																				*/
/*							FUNCS Implementations								*/
/*																				*/
/********************************************************************************/


int Generate_Test_Datanode_Location(struct Datanode_Location *dn_loc, int ip, ulong64 triplets){

	int i, retcode = -DMS_FAIL;

	if(IS_LEGAL(T_MOD, dn_loc)){

		dn_loc->ssk.ip = ip;

		dn_loc->ssk.port = 9009;

		dn_loc->nr_triplets = triplets % NR_TRIPLETS_RANGE;

		for( i = 0; i < dn_loc->nr_triplets; i++){

			dn_loc->triplets[i] = triplets+i;
		}

		retcode = DMS_OK;
	}

	return retcode;
}


int Generate_Test_Located_Request(struct Located_Request *lr){

	ulong64 random_shbid = 0;

	if(IS_LEGAL(T_MOD, lr)){

		get_random_bytes((void *)&random_shbid, sizeof(random_shbid));

		lr->lr_state = true;

		lr->nr_hbids = random_shbid % NR_HBID_RANGE;

		for( i = 0; i < lr->nr_hbids; i++){
			lr->HBIDs[i] = random_shbid + i ;
		}

		lr->nr_dn_locs = 3;

		Create_Datanode_Locations(lr->nr_dn_locs);

		for( i = 0; i < lr->nr_dn_locs; i++){

			Generate_Test_Datanode_Location(lr->dn_locs[i], random_shbid%10+i, random_shbid);

		}

		retcode = DMS_OK;
	}

	return retcode;

}



struct DMS_Metadata * Generate_Test_DMeta(int tid){

	int i = 0;
	struct DMS_Metadata *dmeta = NULL;

	dmeta = (struct DMS_Metadata *) kzalloc( sizeof(struct DMS_Metadata), GFP_KERNEL );

	dmeta->commitID = tid;

	dmeta->nr_lrs = 3;

	dmeta->lrs = Create_Located_Requests(dmeta->nr_lrs);

	for( i = 0; i < dmeta->nr_lrs; i++){

		Generate_Test_Located_Request(dmeta->lrs[i]);
	}


	return dmeta;

}






int Test_Func(TWorker_t *tw)
{

	int retcode = DMS_OK;
	Test_Workers_t tws = NULL;

	if( IS_LEGAL(T_MOD, tw) &&
			IS_LEGAL(T_MOD, tw->tws_mgr) )
	{
		int tid = 0;
		unsigned int nr_elements = 0;

		struct DMS_Metadata *dmeta = NULL;

		struct DMS_Protocol_Header header;
		union NN_Protocol_Body body;

		ulong64 volid;
		get_random_bytes((void *)&volid, sizeof(volid));

		tid = tw->tid;
		tws = tw->tws_mgr;
		nr_elements = tws->ut_arg->nr_elements;

		DMS_PRINTK(TNNP_DBG, T_MOD, "tid = %d, nr_elements = %u \n", tid, nr_elements);

		dmeta = Generate_Test_DMeta(tid);

		//generate nn request header
		Generate_NN_Protocol_Body(&body, tid, tid, (volid % NR_HBID_RANGE));
		Parse_NN_Protocol_Body(&header, buf, &body);
		Generate_NN_Protocol_Header(&header, 1, QUERY_FOR_WRITE, body_len, NULL);

		memset(&body, 0, sizeof(body));
		memset(&header, 0, sizeof(header));


		//parse nn_request

		//compare


	} else {

		retcode = -DMS_FAIL;
	}

	if( CHECK_PTR(T_MOD, tws->done_func) ){
		tws->done_func(tw, retcode);
	}



	return retcode;

}




/********************************************************************************/
/*																				*/
/*									Env APIs									*/
/*																				*/
/********************************************************************************/

int Setup_TDList_Env(unsigned long param){

	int retcode = DMS_OK;


	return retcode;

}

void Teardown_TDList_Env(unsigned long param){



}

int Verify_Test_TDList_Result(unsigned long param){

	int retcode = DMS_OK;


	return retcode;
}








