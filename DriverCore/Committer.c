/*
 * Committer.c
 *
 *  Created on: Apr 4, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include "Metadata.h"
#include "Datanode_Handler.h"


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *COMT_MOD = "COMT: ";



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

#if 0
int Commit_to_Namenode(struct DMS_IO *dio, struct Located_Request *lr){

	int retcode = -DMS_FAIL;

	int i = 0;
	ulong64 slbid = 0;

	if( IS_LEGAL(COMT_MOD, lr) )
	{
		//complete, commit to DMS_Metadata, if haven't commit
		if(!test_and_set_bit(COMMITTED_NN_BIT, &lr->commit_state)){

			//TODO commit to NN

			//TODO compare replica factor with nr_dn_locs, if less than report to namenode

			//TODO release DMS_Metadata
		}

	}

	return retcode;
}


int Commit_to_DMS_Metadata(struct DMS_IO *dio, struct Located_Request *lr){

	int retcode = -DMS_FAIL;

	int i = 0;
	ulong64 slbid = 0;

	if( IS_LEGAL(COMT_MOD, lr) )
	{
		//commit to DMS_Metadata, if haven't commit
		if(!test_and_set_bit(COMMITTED_USER_BIT, &lr->commit_state)){

			slbid = lr->slbid;

			for(i = 0; i < lr->nr_lbids; i++){

				Commit_LB_to_DMS_IO( dio, slbid + i );
			}
		}
	}

	return retcode;
}

int Commit_DNL_Ack(struct DMS_IO *dio, struct Located_Request *lr, int dnl_index, int ack){

	int retcode = -DMS_FAILl;
	struct Datanode_Location *dnl = NULL;


	if( IS_LEGAL(COMT_MOD, lr) &&
			IS_LEGAL(COMT_MOD, lr[dnl_index]) )
	{
		dnl = lr[dnl_index];

		//set result, don't overwrite disk_ack if it reply first. no concurrency due to one receiver.
		if(dnl->dn_result != ACK_DISK_ACK){
			dnl->dn_result = ack;
		}

		switch(ack){

			case ACK_MEM_ACK:
			case ACK_PARTIAL_WRITE_MEM_ACK:

				//complete
				if(atomic_dec_and_test(&lr->wmem_ack)){

					//commit to user
					Commit_to_DMS_Metadata(dio, lr);
				}
				break;

			case ACK_DISK_ACK:

				if(atomic_dec_and_test(&lr->wdisk_ack)){

					//commit to Namenode
					Commit_to_Namenode(dio, lr);

				}else{

					//check commit to user
					Commit_to_DMS_Metadata(dio, lr);
				}
				break;

			case FSM_TIME_OUT:

				wprintk(COMT_MOD, "TIME_OUT: \n");

				//print located request and dio

				//check commit state, if not commit to user, commit it, then commit to namenode

				break;

			default:
				eprintk(COMT_MOD, "unknown ack type = %d \n", ack);
		}

	}

	return retcode;

}

#endif











