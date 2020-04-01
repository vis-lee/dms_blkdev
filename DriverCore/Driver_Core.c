/*
 * Driver_Core.c
 *
 *  Created on: Apr 5, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 *  This file is a central flow manager for integrating DIO module / Metadata_Manager module / Payload_Manager module.
 *
 */

#include "DMS_Common.h"
#include "volume.h"

#include "DIO.h"
#include "IO_Request.h"
#include "Metadata.h"
#include "Metadata_Manager.h"
#include "Payload_Manager.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *DC_MOD = "DCore: ";

//TODO add a woker module for process dn reply, because it will take a long time to handle commit

/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/


/* ACTION: commit to user/namenode */
#define WAITING				0
#define ACT_CUSER			1
#define ACT_LRDONE			2
#define ACT_CNAMENODE		3
#define ACT_BOTH_COMPLETE	4

/********************************************************************************/
/*																				*/
/*							FUNCS Implementations								*/
/*																				*/
/********************************************************************************/


/**
 * Commit_LBs_to_DMS_IO
 * @param dmeta
 * @param lr
 * @param result
 * @return	1:		 	success, and list is empty. dio will be release here.
 * 			0:			if success, but list is not empty
 * 			-1:			fail, null ptr.
 */
int Commit_LBs_to_DMS_IO(struct DMS_Metadata *dmeta, struct Located_Request *lr, int result){

	int retcode = -DMS_FAIL;

	int i = 0;
//	ulong64 slbid = 0;

	if( IS_LEGAL(DC_MOD, lr) )
	{
		//commit to DMS_Metadata, if haven't committed
		if(!test_and_set_bit(COMMITTED_USER_BIT, &lr->commit_state)){

//			slbid = lr->slbid;

//			for(i = 0; i < lr->nr_lbids; i++){

				//TODO we can commit a bunch of lbids at once.
				retcode = Commit_LB_to_DMS_IO( dmeta->pdio, lr->slbid, lr->nr_lbids, result);

				if(retcode == true){

					//dio has been removed. set it to null in dmeta.
					Set_Pdio_in_DMS_Metadata(dmeta, NULL);
				}
//			}
		}
	}

	return retcode;
}


/**
 * Update_Mem_Ack
 * @param dmeta
 * @param lr
 * @param ack
 * @return	1:		ACT_CUSER
 * 			0:		WAITING
 * 			-1:		NULL PTR
 */
static inline int Update_Mem_Ack(struct DMS_Metadata *dmeta, struct Located_Request *lr, int ack){

	int retcode = -DMS_FAIL;
	struct Datanode_Location *dnl = NULL;


	if( IS_LEGAL(DC_MOD, dmeta) &&
			IS_LEGAL(DC_MOD, lr) )
	{

		if( (Dec_WMem_Ack(lr)) ){

			//this lr has n mem_ack, commit to user
			retcode = ACT_CUSER;

		} else {

			retcode = WAITING;
		}
	}

	return retcode;
}


/**
 * Update_Disk_Ack
 * @param dmeta
 * @param lr
 * @param ack
 * @param cuser		indicate whether need to commit to user or not.
 * @return	2:		ACT_LRDONE
 * 			1:		ACT_CUSER
 * 			0:		WAITING
 * 			-1:		NULL PTR
 *
 * DESCRIPTION:
 * 		why we can replace the retcode if Dec_WDisk_Ack(lr) return true, because it shouldn't happen
 * 		that not commit to user yet if remain only 1 wmem_ack and 1 wdisk_ack of the lr.
 */
static inline int Update_Disk_Ack(struct DMS_Metadata *dmeta, struct Located_Request *lr, int ack, int cuser){

	int retcode = -DMS_FAIL;
	struct Datanode_Location *dnl = NULL;


	if( IS_LEGAL(DC_MOD, dmeta) &&
			IS_LEGAL(DC_MOD, lr) )
	{
		if(cuser){

			retcode = ACT_CUSER;;

		}else{

			retcode = WAITING;
		}

		//update wdisk_ack of lr
		if( (Dec_WDisk_Ack(lr)) ){

			//it should been committed to user in the earlier stage.
			retcode = ACT_LRDONE;
		}
	}

	return retcode;
}

/**
 * Default_Commit_Policy
 * @param dmeta
 * @param lr
 * @param dnl_index
 * @param ack
 * @return	2:		ACT_LRDONE
 * 			1:		ACT_CUSER
 * 			0:		WAITING
 * 			-1:		NULL PTR
 *
 * DESCRIPTION:
 * 		rule:
 * 			recv ack && !time out:
 *			receive - 1. first disk ack, 2. memory ack == replicaFactor	--> commit to user
 *					- 3. disk ack == replicaFactor,						--> commit to NN:	success
 *
 *			time out:		partial success - 1. disk ack < replicaFactor
 *							retry			- 1. disk ack == 0
 *
 *
 */
int __Update_and_Commit_to_User_Policy(struct DMS_Metadata *dmeta, struct Located_Request *lr, int dnl_index, int ack){

	int retcode = -DMS_FAIL;
	struct Datanode_Location *dnl = NULL;


	if( IS_LEGAL(DC_MOD, dmeta) &&
			IS_LEGAL(DC_MOD, lr) &&
			IS_LEGAL(DC_MOD, lr->dn_locs[dnl_index]) )
	{

		dnl = lr->dn_locs[dnl_index];

		//Update_Datanode_Location_Result(dnl, ack);

		//set result, don't overwrite disk_ack if it reply first.
		//no concurrency, only one receiver for now.
		if(atomic_read(&dnl->dn_result) != WDISK_ACK){
			atomic_set(&dnl->dn_result, ack);
		}


		switch(ack){

			case WMEM_ACK:
			case PWMEM_ACK:

				retcode = Update_Mem_Ack(dmeta, lr, ack);
				break;

			case WDISK_ACK:
			case PWDISK_ACK:

				//commit to user if recv any disk ack
				retcode = Update_Disk_Ack(dmeta, lr, ack, true);
				break;

			case READ_ACK:

				//commit to user anyway
				retcode = Update_Disk_Ack(dmeta, lr, ack, true);
				break;

			/*read/write case:*/
			case UNDEFINED:
			case EXCEPTION:
			case IOEXCEPTION:
			case IOFAIL_BUT_LUN_OK_EXCEPTION:
			case FAILED_LUN_EXCEPTION:

				DSTR_PRINT( ALWAYS, DC_MOD,
							SPrint_DMS_Datanode_Tag(DSTR_NAME, DSTR_LIMIT, lr->dn_locs[dnl_index]),
							"commit error to dmeta, failed location: ");

				if(dmeta->op == DMS_OP_READ){

					//TODO HBID not found, means metadata out of date, so we should refresh from namenode.
					//TODO commit to namenode and re-send by metadata_handler

				} else {

					retcode = Update_Mem_Ack(dmeta, lr, ack);

					retcode = Update_Disk_Ack(dmeta, lr, ack, retcode);
				}

				break;

//			case FSM_TIME_OUT:
//
//				wprintk(DC_MOD, "TIME_OUT: \n");
//
//				//print located request and dio
//
//				//check commit state, if not commit to user, commit it, then commit to namenode
//
//				break;

			default:
				eprintk(DC_MOD, "unknown ack type = %d \n", ack);
		}

	}

	return retcode;

}


/**
 * Default_Update_and_Commit_to_User_Policy
 * @param dmeta
 * @param ddt
 * @param ack
 * @return	>=0:		Done_LRs
 * 			-1:		NULL PTR
 */
int Default_Update_and_Commit_to_User_Policy(struct DMS_Metadata *dmeta, struct DMS_Datanode_Tag *ddt, int ack){

	int retcode = -DMS_FAIL;
	int i = 0, done_lrs = 0;

	if( IS_LEGAL(DC_MOD, dmeta) &&
			IS_LEGAL(DC_MOD, ddt) )
	{

		struct DMS_Payload_Tag *dpt = NULL;

		Reset_DList_Iterator(&ddt->payload_tags);

		//iterate all payload tags
		while( (dpt = Get_Next_User_Data(&ddt->payload_tags)) ){

			if(IS_LEGAL(DC_MOD, dpt)){

				//commit this dpt
				struct Located_Request *lr = dpt->lr;
				int dn_index = dpt->dn_index;

				//commit to user policy
				retcode = __Update_and_Commit_to_User_Policy(dmeta, lr, dn_index, ack);

				switch(retcode){

					case ACT_LRDONE:

						done_lrs++;
						//do commit bellow for /*special case: replica == 1*/

					case ACT_CUSER:

						//complete, commit to user
						Commit_LBs_to_DMS_IO(dmeta, lr, ack);

						break;

					default:
						DMS_PRINTK(DC_DBG, DC_MOD, "un-processed retcode = %d\n", retcode);

				}
			}

			retcode = done_lrs;
		}
	}

	return retcode;

}


int Default_Update_and_Commit_to_Namenode_Policy(struct DMS_Metadata *dmeta, int done_lrs){

	int retcode = -DMS_FAIL;
	int i = 0;

	if( IS_LEGAL(DC_MOD, dmeta) )
	{
		DMS_PRINTK(DC_DBG, DC_MOD, "before update: %d", atomic_read(&dmeta->nr_waiting_lrs));

		for( i = 0; i < done_lrs; i++){

			if( Dec_Waiting_LRs(dmeta) ){

				retcode = ACT_CNAMENODE;
			}
		}

		DMS_PRINTK(DC_DBG, DC_MOD, "after update: %d", atomic_read(&dmeta->nr_waiting_lrs));
	}

	return retcode;

}


int __Process_Payload(struct DMS_IO *dio){

	int retcode = -DMS_FAIL;

	//handle payload
	if( Payload_Handler(dio) ){

		//TODO return something, we need to handle.

	}

	return retcode;

}


/**
 * __VolumeIO_Handler - per volume request entry point
 * @param volume
 * @param kreq
 * @return
 */
int __VolumeIO_Handler(struct DMS_Volume *volume, struct request *kreq){

	int retcode = -DMS_FAIL;
	struct DMS_IO *dio = NULL;

	if(IS_LEGAL(DC_MOD, volume) &&
			IS_LEGAL(DC_MOD, kreq) )
	{
		struct IO_Request *ior = __Make_DIO_Requst(kreq);

		Add_IO_Reqeust_to_KRequest(kreq, ior);

		//go back to head
		Reset_IO_Request_Iterator(ior);

		while( (dio = Get_Next_DIO_from_IO_Request(ior)) )
		{
#if 0
			//get meta-data
			if( Metadata_Handler(dio) ){

				//cache hit, handle payload
				if( __Process_Payload(dio) ){

					//TODO return something, we need to handle.

				}
			}
#endif
		}

		retcode = DMS_OK;
	}



	return retcode;
}


/**
 * __VolumeIO_Commiter
 * @param dmeta
 * @param data
 * @param result
 * @return
 *
 * DESCRIPTION:
 * 		Commit flow:
 * 			1. update Located Request state
 * 			2. decide what commit to do according to which state we are,
 * 				i. ( (lr->wmem_ack == 0 || result == WDISK_ACK) && lr->commit_state != commit_to_user ), then commit to user;
 * 				ii. (lr->wdisk_ack == 0) then update dmeta->nr_waiting_lrs.
 * 			3. update DMS_Metadata state
 * 				i if(dmeta->nr_waiting_lrs == 0) then commit to namenode by lingered DMS_Datanode_Tag
 *
 * N.B. Commit flow shouldn't used dio, because dio will be released while it got n mem_acks.
 *
 */
int __VolumeIO_Commiter(struct DMS_Metadata *dmeta, void *data, int result){

	int retcode = -DMS_FAIL;
	int done_lrs = 0;

	if(IS_LEGAL(DC_MOD, data)){

		struct DMS_Datanode_Tag *ddt = (struct DMS_Datanode_Tag *)data;

		done_lrs = Default_Update_and_Commit_to_User_Policy(dmeta, ddt, result);

		if(done_lrs > 0){

			retcode = Default_Update_and_Commit_to_Namenode_Policy(dmeta, done_lrs);
		}

		Commit_Datanode_Tags(dmeta, ddt, result);

		if(retcode == ACT_CNAMENODE){

			Commit_to_Namenode(dmeta);
		}

	}

	return retcode;
}





#ifdef DMS_UTEST

EXPORT_SYMBOL(__VolumeIO_Handler);
EXPORT_SYMBOL(__VolumeIO_Commiter);

#endif



















