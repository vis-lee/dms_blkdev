/*
 * Payload.c
 *
 *  Created on: May 17, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include "DMS_Common.h"
#include "Payload.h"
#include "DIO.h"


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char * PAL_MOD = "PAL: ";



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

/**
 * Check_DN_Tag_by_SSocket
 *
 * @param src
 * @param arg
 * @return	1: 	true
 * 			0:	false
 *
 * 		get target data.
 */
int Check_DN_Tag_by_SSocket(void *src, void *arg){

	if(IS_LEGAL(PAL_MOD, src) &&
			IS_LEGAL(PAL_MOD, arg) )
	{
		struct DMS_Datanode_Tag *ddt = (struct DMS_Datanode_Tag *)src;
		struct Datanode_Location *dn_loc = (struct Datanode_Location *)arg;

		if( Compare_SSocket(&ddt->ssk, &dn_loc->ssk) ){

			//has been sent, get next one
			if(ddt->dnrid){
				return false;
			}

			return true;
		}

		return false;

	}

}


//TODO should this func be singleton? not necessary.
//if we extend nn receiver to multi-thread, one thread do a request in diff context, so they shouldn't be concurrent here.
//(lock is better than check every list node)
/**
 * Get_Datanode_Tag
 * @param dn_tags
 * @param dn_loc
 * @return	ptr:	ddt ptr
 * 			NULL:	create/insert fail
 *
 * DESCRIPTION:
 * 		get datanode tag from dn_tags. create new one, if not found.
 */
static struct DMS_Datanode_Tag * __Get_Datanode_Tag(struct DMS_Metadata *dmeta, struct Datanode_Location *dn_loc){

	struct DMS_Datanode_Tag *ddt = NULL;

	if( IS_LEGAL(PAL_MOD, dmeta) &&
			IS_LEGAL(PAL_MOD, dn_loc) )
	{

		struct DList *dn_tags = &dmeta->dn_tags;
		struct DList_Node *node = NULL;

		//check list
		ddt = Get_User_Data_By_Check_Func(dn_tags, Check_DN_Tag_by_SSocket, (void *)dn_loc);

		/*
		 * if not exist, create new one.
		 */
		if( !CHECK_PTR(PAL_MOD, ddt) ){

			//create ddt
			ddt = Create_DMS_Datanode_Tag(dmeta, dn_loc->ssk);

			//add to dn_tags
			if( CHECK_PTR(PAL_MOD, ddt) ){

				node = Insert_User_Data_to_DList_Tail(dn_tags, (void *)ddt);

				goto INSERT_DDT_FAIL;
			}
		}
	}

	return ddt;

INSERT_DDT_FAIL:

	if( CHECK_PTR(PAL_MOD, ddt) ){

		Release_DMS_Datanode_Tag(ddt);
	}

	return NULL;

}


/**
 * Integrate_to_DN_Service_Request
 *
 * @param dn_sreq
 * @param dpt
 * @return 	0: 		OK
 * 			-1:		NULL ptr
 *
 * DESCRIPTION:
 * 		collect dn service request data.
 */
int Integrate_to_DN_Service_Request(struct DN_Service_Request *dn_sreq, struct DMS_Payload_Tag *dpt){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(PAL_MOD, dn_sreq) &&
			IS_LEGAL(PAL_MOD, dpt) )
	{

		struct Located_Request *lr = dpt->lr;
		struct Datanode_Location *dn_loc = lr->dn_locs[dpt->dn_index];

		//shift id array of sreq to correct index
		ulong64 *hbidsp = &(dn_sreq->hbids[dn_sreq->nr_hbids]);
		ulong64 *tripletsp = &(dn_sreq->triplets[dn_sreq->nr_triplets]);

		//copy hbids and triplets into service request
		memcpy(hbidsp, &lr->HBIDs, sizeof(ulong64) * lr->nr_hbids);
		memcpy(tripletsp, &dn_loc->triplets, sizeof(ulong64) * dn_loc->nr_triplets);

		//accumulate numbers
		dn_sreq->nr_hbids += lr->nr_hbids;
		dn_sreq->nr_triplets += dn_loc->nr_triplets;

		retcode = DMS_OK;

	}

	return retcode;
}


/**
 *	Add_to_Datanode_Tags
 *
 * @param dn_tags
 * @param lr
 * @param dn_index
 * @return	0:		add OK
 * 			-1:		NULL ptr
 *
 * DESCRIPTION:
 * 		add one lr and dn_loc into the datanode tag it belongs.
 */
int Add_to_Datanode_Tags(struct DMS_Metadata *dmeta, struct Located_Request *lr, int dn_index){

	int retcode = -DMS_FAIL;
	struct DMS_Datanode_Tag *ddt = NULL;

	if(IS_LEGAL(PAL_MOD, dmeta) &&
			IS_LEGAL(PAL_MOD, lr) &&
			IS_LEGAL(PAL_MOD, lr->dn_locs[dn_index]) )
	{

		//get dn tag. create if not found.
		ddt = __Get_Datanode_Tag(dmeta, lr->dn_locs[dn_index]);

		if( CHECK_PTR(PAL_MOD, ddt) ){

			//create payload tag and add to this list
			struct DMS_Payload_Tag *dpt = Create_DMS_Payload_Tag(lr, dn_index);

			if(CHECK_PTR(PAL_MOD, dpt)){

				//add to payload tags list
				retcode = Insert_User_Data_to_DList_Tail(&ddt->payload_tags, (void *)dpt);

				if(retcode < 0){

					goto INSERT_FAIL;
				}

			}

			/*
			 * Optimization for generating dn_service_request body by
			 * reducing iterative dpt again to collect data.
			 */
			Integrate_to_DN_Service_Request(&ddt->dn_sreq, ddt);

		}
	}

	return retcode;

INSERT_FAIL:

	Release_DMS_Payload_Tag(dpt);

	return -DMS_FAIL;

}


void Remove_DMS_Datanode_Tag(struct DMS_Datanode_Tag *ddt){

	if( IS_LEGAL(PAL_MOD, ddt) )
	{
		//release all payload tags.
		Release_All_User_Data(&ddt->payload_tags, Release_DMS_Payload_Tag);

		//release datanode tag
		Release_DMS_Datanode_Tag(ddt);
	}

}


inline int Check_DN_Tag_by_ID(void *src, void *arg){

	if(IS_LEGAL(PAL_MOD, src) &&
			IS_LEGAL(PAL_MOD, arg) )
	{
		struct DMS_Datanode_Tag *ddt = (struct DMS_Datanode_Tag *)src;
		ulong64 dnrid = (ulong64)arg;

		if( ddt->dnrid == dnrid ){
			return true;
		}

		return false;

	}

}

/**
 * Remove_from_Datanode_Tags
 *
 * @param ip
 * @param port
 * @param dn_tags
 * @return	struct DMS_Datanode_Tag *ddt: if found and remove from list.
 * 			-1:		not found
 */
struct DMS_Datanode_Tag * Get_Datanode_Tag_from_Datanode_Tags(struct DList *dn_tags, ulong64 dnrid){

	struct DMS_Datanode_Tag *ddt = NULL;

	if( IS_LEGAL(PAL_MOD, dn_tags) )
	{
		//identify which one is exactly match by dn_id
		struct DMS_Datanode_Tag *ddt = Remove_from_DList_by_Check_Func(dn_tags, Check_DN_Tag_by_ID, (void *)dnrid);

		if(CHECK_PTR(PAL_MOD, ddt)){

			retcode = DMS_OK;
		}
	}

	return retcode;

}


int Put_Datanode_Tag_to_Datanode_Tags(struct DList *dn_tags, struct DMS_Datanode_Tag *ddt){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(PAL_MOD, dn_tags) &&
			IS_LEGAL(PAL_MOD, ddt) )
	{
		//add to payload tags list
		retcode = Insert_User_Data_to_DList_Tail(dn_tags, (void *)ddt);

		if(retcode < 0){

			goto INSERT_FAIL;
		}

	}

	return retcode;

}




/**
 * Default_Read_Policy
 *
 * @param data
 * @param nr_dn_locs
 * @return	0:		add OK
 * 			-1:		NULL ptr
 *
 * DESCRIPTION:
 * 		this policy using random to pick up a closer and available index.
 */
int Default_Read_Policy(void *data, int nr_dn_locs){

	int index = -1;
	struct Located_Request *lr = NULL;

	if(IS_LEGAL(PAL_MOD, data)){

		int i = 0;
		lr = (struct Located_Request *)data;

		//TODO maybe can record in lr, ex: index = (lr->cur_index != -1) ? lr->cur_index:get_random_int() % nr_dn_locs;
		index = get_random_int() % nr_dn_locs;

		for(i = index; i < lr->nr_dn_locs; i++){

			/*never be used dn.*/
			if(lr->dn_locs[i]->dn_result == 0){
				return i;
			}
		}

		for(i = 0; i < index; i++){

			/*never be used dn.*/
			if(lr->dn_locs[i]->dn_result == 0){
				return i;
			}
		}
	}


	DSTR_DECLARE(str);
	SPrint_One_Located_Requests(str, DSTR_LIMIT, lr);

	DMS_PRINTK(PALO_DBG, PAL_MOD, "no available index in this located request: %s \n", str);

	DSTR_FREE(str);


	return -DMS_FAIL;

}


/**
 * __Commit_Error_to_vIO_Commiter
 *
 * @param dmeta
 * @param lr
 * @param dn_index
 *
 * DESCRIPTION:
 * 		record error into dn_loc_result, it should be always success.
 */
static inline void __Commit_Error_to_vIO_Commiter(struct DMS_Metadata *dmeta, struct Located_Request *lr, int dn_index){

	//commit error
	struct DMS_Payload_Tag *dpt = Create_DMS_Payload_Tag(lr, dn_index);

	if(CHECK_PTR(PAL_MOD, dpt)){

		//add to payload tags list
		dmeta->volume->vIO_Commiter(dmeta->pdio, dpt, false);

	}

}


/**
 * Translate_Read_Located_Request_to_DN_Tags
 * @param dmeta
 * @param lr
 * @return	0:		OK
 * 			-1:		NULL ptr
 *			-1501:	all dn are invalidated.
 *
 * DESCRIPTION:
 * 		pick up one dn_location of lr and translate it into DN Tags by vIO_Read_Policy
 */
int Translate_Read_Located_Request_to_DN_Tags(struct DMS_Metadata *dmeta, struct Located_Request *lr){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(PAL_MOD, dmeta) &&
			IS_LEGAL(PAL_MOD, lr) )
	{
		int dn_index = 0;

		dn_index = dmeta->volume->vIO_Read_Policy(lr, lr->nr_dn_locs);

		if(IS_IN_LENGTH_RANGE(dn_index, 0, lr->nr_dn_locs)){

			retcode = Add_to_Datanode_Tags(dmeta, lr, dn_index);

			if(retcode < 0){

				//commit error to vIO_Commiter
				__Commit_Error_to_vIO_Commiter(dmeta, lr, dn_index);
			}

		}else{

			//this means there is no available index in this lr
			//TODO should we do something? ex: check whether end this request or not? of course timer will expire.

			Set_Located_Request_State(lr, LR_INVALID);

			retcode = -EDN_EDNS;

			//all DNs fail
			PRINT_DMS_ERR_LOG(EDN_EDNS, PAL_MOD, "");
		}
	}

	return retcode;

}


/**
 * Translate_Read_Located_Requests_to_DN_Tags
 * @param dmeta
 * @return	0:		OK
 * 			-1:		NULL ptr
 *
 * DESCRIPTION:
 * 		Tanslate all lrs of metadata into DN_Tags
 */
int Translate_Read_Located_Requests_to_DN_Tags(struct DMS_Metadata *dmeta){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(PAL_MOD, dmeta) )
	{
		int i = 0;
		struct Located_Request *lr = NULL;
		void *pdata = NULL;

		//go thru all located request
		for( i = 0; i < dmeta->nr_lrs; i++ ){

			lr = dmeta->lrs[i];

			retcode = Translate_Read_Located_Request_to_DN_Tags(dmeta, lr);

			if(retcode == -EDN_EDNS){
				//TODO do some error handling?
			}

			//record first one LBMD of LR
			pdata = Get_LBMD_from_Node_by_LBID(dmeta->pdio->LB_list, pdata, lr->slbid);
			Set_Located_Request_PData( lr, pdata );

		}

	}

	return retcode;

}


/**
 * Translate_Write_Located_Requests_to_DN_Tags
 * @param dmeta
 * @return	0:		OK
 * 			-1:		NULL ptr
 *
 * DESCRIPTION:
 * 		Tanslate all lrs of metadata into DN_Tags
 */
int Translate_Write_Located_Requests_to_DN_Tags(struct DMS_Metadata *dmeta){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(PAL_MOD, dmeta) )
	{
		int i = 0, dn_index = 0;
		struct Located_Request *lr = NULL;
		void *pdata = NULL;

		//go thru all located request
		for( i = 0; i < dmeta->nr_lrs; i++ ){

			lr = dmeta->lrs[i];

			for( dn_index = 0; dn_index < lr->nr_dn_locs; dn_index++){

				retcode = Add_to_Datanode_Tags(dmeta, lr, dn_index);

				if(retcode < 0){

					//commit error to vIO_Commiter
					__Commit_Error_to_vIO_Commiter(dmeta, lr, dn_index);
				}
			}

			//record first one LBMD of LR
			pdata = Get_LBMD_from_Node_by_LBID(dmeta->pdio->LB_list, pdata, lr->slbid);
			Set_Located_Request_PData( lr, pdata );

		}

		retcode = DMS_OK;

	}

	return retcode;

}

/**
 * Generate_All_DN_Tags
 *
 * @param dio
 * @return	0:	OK
 * 			-1:	Fail
 *
 * DESCRIPTION:
 * 		generate dn tags by
 */
int Generate_All_DN_Tags(struct DMS_IO *dio /*struct DMS_Metadata *dmeta*/){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(PAL_MOD, dio) )
	{

		struct DMS_Metadata *dmeta = dio->dmeta;

		switch(dmeta->op){

			case DMS_OP_READ:

				//choose one of dns
				retcode = Translate_Read_Located_Requests_to_DN_Tags(dmeta);

				break;

			case DMS_OP_WRITE:
			case DMS_OP_OVERWRITE:

				retcode = Translate_Write_Located_Requests_to_DN_Tags(dmeta);

				break;

			default:
				eprintk(PAL_MOD, "unknown op = %d, %s \n", dmeta->op, op2str(dmeta->op));
		}


	}

	DMS_PRINTK(PALO_DBG, PAL_MOD, "end, retcode = %d \n", retocde);

	return retcode;

}













/********************************************************************************/
/*																				*/
/*								Malloc and INIT									*/
/*																				*/
/********************************************************************************/

int Init_DMS_Payload_Tag(struct DMS_Payload_Tag *dpt, struct Located_Request *lr, int index){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(PAL_MOD, dpt)){

		//memset(dpt, 0, sizeof(struct DMS_Payload_Tag));

		dpt->lr = lr;

		dpt->dn_index = index;

	}

	return retcode;
}


struct DMS_Payload_Tag * Create_DMS_Payload_Tag(struct Located_Request *lr, int index){

	struct DMS_Payload_Tag *dpt = NULL;

	dpt = Malloc_DMS_Payload_Tag(GFP_KERNEL);

	if( Init_DMS_Payload_Tag(dpt, lr, index) < 0 ){
		goto Create_DPT_FAIL;
	}

	return dpt;

Create_DPT_FAIL:

	if(CHECK_PTR(PAL_MOD, dpt)){

		Release_DMS_Payload_Tag(dpt);
	}

	PRINT_DMS_ERR_LOG(EDEV_NOMEM, PAL_MOD, "");

	return NULL;
}


void Release_DMS_Payload_Tag(struct DMS_Payload_Tag *dpt){

	if(IS_LEGAL(PAL_MOD, dpt)){

		Free_DMS_Payload_Tag(dpt);
	}
}




int Init_DMS_Datanode_Tag(struct DMS_Datanode_Tag *ddt, struct DMS_Metadata *dmeta, struct SSocket *ssk){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(PAL_MOD, ddt)){

		memset(ddt, 0, sizeof(struct DMS_Datanode_Tag));

		//ddt->dnrid = 0;
		ddt->dmeta = dmeta;

		Copy_SSocket(&ddt->ssk, ssk);

		ddt->retry_count = 1;

		__Init_DList(&ddt->payload_tags);

		retcode = DMS_OK;

	}

	return retcode;
}


struct DMS_Datanode_Tag * Create_DMS_Datanode_Tag(struct DMS_Metadata *dmeta, struct SSocket *ssk){

	struct DMS_Datanode_Tag *ddt = NULL;

	ddt = Malloc_DMS_Datanode_Tag(GFP_KERNEL);

	if( Init_DMS_Datanode_Tag(ddt, dmeta, ssk) < 0 ){
		goto Create_DDT_FAIL;
	}

	return ddt;

Create_DDT_FAIL:

	if(CHECK_PTR(PAL_MOD, ddt)){

		Release_DMS_Datanode_Tag(ddt);
	}

	PRINT_DMS_ERR_LOG(EDEV_NOMEM, PAL_MOD, "");

	return NULL;
}


void Release_DMS_Datanode_Tag(struct DMS_Datanode_Tag *ddt){

	if(IS_LEGAL(PAL_MOD, ddt)){

		Free_DMS_Datanode_Tag(ddt);
	}
}








int SPrint_DMS_Payload_Tag(char *dbg_str, int len_limit, struct DMS_Payload_Tag *dpt){

	int len = 0;

	if( IS_LEGAL(PAL_MOD, dpt) &&
			IS_LEGAL(PAL_MOD, dbg_str) )
	{

		if(len < len_limit){

			len += sprintf(dbg_str+len, "\t dpt: { lr = %p, dn_index = %d\n", dpt->lr, dpt->dn_index);

			//more detail, if necessary.
			//len += SPrint_One_Datanode_Location(dbg_str+len, len_limit-len, dpt->lr[dpt->dn_index]);

			len += sprintf(dbg_str+len, "\n\t} \n");
		}

	}

	return len;

}


int SPrint_DMS_Datanode_Tag(char *dbg_str, int len_limit, struct DMS_Datanode_Tag *ddt){

	int len = 0;
	struct DMS_Payload_Tag *dpt = NULL;

	if( IS_LEGAL(PAL_MOD, ddt) &&
			IS_LEGAL(PAL_MOD, dbg_str) )
	{

		if(len < len_limit){

			len += sprintf(dbg_str+len, "\t ddt: { dnrid = %llu, dn_node_index = %d, dn_ip = %s, retry_count = %d, \n",
									ddt->dnrid, dn_node_index, Get_IP_Str(ddt->dn_node_index), ddt->retry_count);

			Reset_DList_Iterator(&ddt->payload_tags);

			while( (dpt = Get_Next_User_Data(&ddt->payload_tags)) ){

				if(len < len_limit){

					len += SPrint_DMS_Payload_Tag(dbg_str+len, len_limit-len, dpt);
				}
			}

			if(len < len_limit){

				len += SPrint_DN_Service_Request(dbg_str+len, len_limit-len, &ddt->dn_sreq);
			}

			len += sprintf(dbg_str+len, "\n\t} \n");
		}

	}

	return len;

}


int SPrint_DMS_Datanode_Tags(char *dbg_str, int len_limit, struct DList *dn_tags){

	int len = 0;
	struct DMS_Datanode_Tag *ddt = NULL;

	if( IS_LEGAL(PAL_MOD, dn_tags) &&
			IS_LEGAL(PAL_MOD, dbg_str) )
	{

		if(len < len_limit){

			Reset_DList_Iterator(dn_tags);

			while( (ddt = Get_Next_User_Data(dn_tags)) ){

				if(len < len_limit){

					len += SPrint_DMS_Datanode_Tag(dbg_str+len, len_limit-len, ddt);
				}
			}
		}
	}

	return len;

}





