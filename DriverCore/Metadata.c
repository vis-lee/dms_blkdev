/*
 * Metadata.c
 *
 *  Created on: Apr 3, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include "Metadata.h"
#include "Volume.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *META_MOD = "META: ";



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


//int Update_Ack_State(struct Located_Request *lr){
//
//	int retcode = -DMS_FAIL;
//
//	if(IS_LEGAL(META_MOD, lr)){
//
//		write_lock(&lr->lock);
//
//		lr->wmem_ack;
//
//		write_unlock(&lr->lock);
//	}
//
//	return retcode;
//}


struct Datanode_Location ** Create_Datanode_Locations(int nr_dns){

	int i = 0;

	struct Datanode_Location *dn = NULL;

	struct Datanode_Location **dns = (struct Datanode_Location **) kzalloc( sizeof(struct Datanode_Location *) * nr_dns, GFP_KERNEL );

	for(i = 0; i < nr_dns; i++){

		dn = Malloc_Datanode_Location(GFP_KERNEL);

		//FIXME take me off after stable.
		memset(dn, 0, sizeof(struct Datanode_Location));

		atomic_set(&dn->dn_result, 0);

		if(dn){

			dns[i] = dn;
			dn = NULL;

		}else{

			goto INIT_DNS_FAIL;
		}
	}


	return dns;

INIT_DNS_FAIL:

	Release_Datanode_Locations(dns, nr_dns);

	PRINT_DMS_ERR_LOG(EDEV_NOMEM, META_MOD, "");

	return NULL;

}

void Release_Datanode_Locations(struct Datanode_Location **dns, int nr_dns){

	int i = 0;

	if(IS_LEGAL(META_MOD, dns)){

		for( i = 0; i < nr_dns; i++){

			if(dns[i]){

				Free_Datanode_Location(dns[i]);
			}

		}

		kfree(dns);
	}
}


/**
 * Create_Located_Requests - create located request array by length nr_lrs
 *
 * @param nr_lrs
 * @return Located_Request ** lrs - lr array
 */
struct Located_Request ** Create_Located_Requests(int nr_lrs){

	int i = 0;

	struct Located_Request *lr = NULL;

	struct Located_Request **lrs = (struct Located_Request **) kzalloc( sizeof(struct Located_Request *) * nr_lrs, GFP_KERNEL );

	for(i = 0; i < nr_lrs; i++){

		lr = Malloc_Located_Request(GFP_KERNEL);

		//FIXME take me off after stable.
		memset(lr, 0, sizeof(struct Located_Request));

		if(lr){

			lrs[i] = lr;
			lr = NULL;

		}else{

			goto INIT_LRS_FAIL;
		}
	}


	return lrs;

INIT_LRS_FAIL:

	Release_Located_Requests(lrs, nr_lrs);

	PRINT_DMS_ERR_LOG(EDEV_NOMEM, META_MOD, "");

	return NULL;

}

/**
 * Release_Located_Requests - release located request array
 *
 * @param lrs
 * @param nr_lrs
 */
void Release_Located_Requests(struct Located_Request **lrs, int nr_lrs){

	int i = 0;

	if(IS_LEGAL(META_MOD, lrs)){

		for( i = 0; i < nr_lrs; i++){

			if( CHECK_PTR(META_MOD, lrs[i]) ){

				Release_Datanode_Locations(lrs[i]->dn_locs, lrs[i]->nr_dn_locs);

			}

			Free_Located_Request(lrs[i]);

		}

		kfree(lrs);
	}
}


/**
 * Init_DMS_Metadata
 *
 * @param dio
 * @param dmeta
 * @return	0 - OK
 * 			-1 - create lrs fail
 */
int Init_DMS_Metadata(struct DMS_IO *dio, struct DMS_Metadata *dmeta){

	int retcode = - DMS_FAIL;

	if(IS_LEGAL(META_MOD, dmeta)){

		memset(dmeta, 0, sizeof(struct DMS_Metadata));

		dmeta->op = dio->op;

		dmeta->volume = dio->volume;

		//parent dio
		dmeta->pdio = dio;

		__Init_DList(&dmeta->dn_tags);

		retcode = DMS_OK;
	}

	return retcode;
}


struct DMS_Metadata * Create_DMS_Metadata(struct DMS_IO *dio){

	int retcode = -DMS_FAIL;

	struct DMS_Metadata *dmeta = Malloc_DMS_Metadata( GFP_KERNEL );

	if( Init_DMS_Metadata(dio, dmeta) < 0 ){

		goto CREATE_DMETA_FAIL;
	}

	return dmeta;

CREATE_DMETA_FAIL:

	if(CHECK_PTR(META_MOD, dmeta)){
		Free_DMS_Metadata(dmeta);
	}

	PRINT_DMS_ERR_LOG(EDEV_NOMEM, META_MOD, "");

	return NULL;

}


void Release_DMS_Metadata(struct DMS_Metadata *dmeta){

	if(IS_LEGAL(META_MOD, dmeta)){

		Release_Located_Requests(dmeta->lrs, dmeta->nr_lrs);

		Free_DMS_Metadata(dmeta);
	}
}




int SPrint_One_Datanode_Location(char *dbg_str, int len_limit, struct Datanode_Location *dn_loc){

	int len = 0;
	int i = 0;

	if( IS_LEGAL(META_MOD, dbg_str) &&
			IS_LEGAL(META_MOD, dn_loc) )
	{
		int rbid = 0, rb_len = 0, offset = 0;
		char *ip_c = (char *)&dn_loc->ssk.ip;

		//print locs
		if(len < len_limit)
			len += sprintf(dbg_str+len, "\t\tnode_ip = %d.%d.%d.%d, port = %d\n",
				(int)ip_c[3],
				(int)ip_c[2],
				(int)ip_c[1],
				(int)ip_c[0], dn_loc->ssk.port);

		for( i = 0; i < dn_loc->nr_triplets; i++){

			//decode RBID triplet
			decompose_triple(dn_loc->triplets[i], &rbid, &rb_len, &offset);

			if(len < len_limit)
				len += sprintf(dbg_str+len, "\t\tTriplets[%d] = %llu : rbid = %d, len = %d, offset = %d \n",
												i, dn_loc->triplets[i], rbid, rb_len, offset);
		}

	}

	return len;
}



int SPrint_One_Located_Requests(char *dbg_str, int len_limit, struct Located_Request *lr){

	int len = 0;
	int i = 0;

	if( IS_LEGAL(META_MOD, dbg_str) &&
			IS_LEGAL(META_MOD, lr) )
	{
		int nr_HBIDs = lr->nr_hbids;

		if(len < len_limit)
			len += sprintf(dbg_str+len, "\tslbid = %llu, nr_lbids = %d \n\tLBIDs = [ ",
											lr->slbid, lr->nr_lbids);

		//print LBIDs
		for(i = 0; i < nr_HBIDs; i++){
			if(len < len_limit) len += sprintf(dbg_str+len, "%llu, ", lr->slbid + i);
		}
		if(len < len_limit)
			len += sprintf(dbg_str+len, "]\n \tHBIDs = [ ");


		//print HBIDs
		for(i = 0; i < nr_HBIDs; i++){
			if(len < len_limit) len += sprintf(dbg_str+len, "%llu, ", lr->HBIDs[i]);
		}
		if(len < len_limit)
			len += sprintf(dbg_str+len, "]\n ");


		if(len < len_limit)
			len += sprintf(dbg_str+len, "\tnr_dn_locs = %d, dn_loc[%d] = { ",
											lr->nr_dn_locs, i);
		//print locs
		for(i = 0; i < lr->nr_dn_locs; i++){

			len += SPrint_One_Datanode_Location(dbg_str+len, len_limit-len, lr->dn_locs[i]);

		}

		if(len < len_limit) len += sprintf(dbg_str+len, "\t}\n");

	}

	return len;

}




int SPrint_Located_Requests(char *dbg_str, int len_limit, struct Located_Request **lrs, int nr_lrs){

	int len = 0;
	int i = 0;

	if( IS_LEGAL(META_MOD, dbg_str) &&
			IS_LEGAL(META_MOD, lrs) )
	{

		for( i = 0; i < nr_lrs; i++){

			if(len < len_limit){

				len += sprintf(dbg_str+len, "\n\tlrs[%d] {\n", i);

				len += SPrint_One_Located_Requests(dbg_str+len, len_limit-len, lrs[i]);

				len += sprintf(dbg_str+len, "}\n");
			}
		}
	}

	return len;

}

int SPrint_DMS_Metadata(char *dbg_str, int len_limit, struct DMS_Metadata *dmeta){

	int len = 0;

	if( IS_LEGAL(META_MOD, dmeta) &&
			IS_LEGAL(META_MOD, dbg_str) )
	{

		if(len < len_limit){
			len += sprintf(dbg_str+len, "vis_dbg: commit_id = %llu,", dmeta->commitID);
		}

		len += SPrint_Located_Requests(dbg_str+len, len_limit-len, dmeta->lrs, dmeta->nr_lrs);
	}

	return len;

}





#if 0	//malloc direct
struct DMS_Metadata * Create_DMS_Metadata(int nr_lrs){

	int retcode = -DMS_FAIL;

	struct DMS_Metadata *dmeta = (struct DMS_Metadata *) kzalloc( sizeof(struct DMS_Metadata), GFP_KERNEL );

	if(IS_LEGAL(META_MOD, dmeta)){

		retcode = Init_DMS_Metadata(nr_lrs, dmeta);

		if(retcode != DMS_OK){
			kfree(dmeta);
			dmeta = NULL;
		}
	}

	return dmeta;

}


void Release_DMS_Metadata(struct DMS_Metadata *dmeta){

	if(IS_LEGAL(META_MOD, dmeta)){

		int i = 0;
		int nr_lrs = dmeta->nr_lrs;

		Release_Located_Requests(dmeta->lrs);

		kfree(dmeta);
	}
}
#endif







