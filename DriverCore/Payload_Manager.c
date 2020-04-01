/*
 * Payload_Manager.c
 *
 *  Created on: May 17, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include <linux/socket.h>

#include "LogicalBlock.h"
#include "Datanode_Protocol.h"


/********************************************************************************/
/*																				*/
/*							Global Variables 									*/
/*																				*/
/********************************************************************************/

char *PAM_MOD = "PALOMGR: ";



/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/





/********************************************************************************/
/*																				*/
/*							Datanode Request Handler							*/
/*																				*/
/********************************************************************************/

/**
 * __Xmit_LB_Paylaod
 * @param node_index
 * @param op
 * @param lbmd
 * @param flags
 * @return	>0:		send/recv length
 * 			-1:		NULL ptr
 * 			-32:	-EPIPE. pipe broken, or someone stop me.
 * 			-1601:	connection error
 *
 * DESCRIPTION:
 * 		Transmit one logical block payload.
 */
static inline int __Xmit_LB_Paylaod(int node_index, char op, struct LogicalBlock_MetaData *lbmd, int flags)
{
	int i = 0, retcode = 0;

	if(IS_LEGAL(PAM_MOD, lbmd)){

		switch(op){

			case DMS_OP_READ:

				struct bio_vec *bvec = bio_iovec_idx((lbmd->bio), lbmd->vec_idx);

				void *kaddr = kmap(bvec->bv_page);

				//receive from datanode
				retcode = Recv_Msg(node_index, kaddr + bvec->bv_offset, bvec->bv_len, flags);

				kunmap(bvec->bv_page);

				break;

			case DMS_OP_WRITE:
			case DMS_OP_OVERWRITE:

#ifdef TCP_SENDPAGE
				//TODO we can try to change to tcp_sendpage(); which won't copy data once again in kernel.
				//so it needn't translate bv_page to kaddr.
#else
				struct bio_vec *bvec = bio_iovec_idx((lbmd->bio), lbmd->vec_idx);

				void *kaddr = kmap(bvec->bv_page);

				//send to datanode
				retcode = Send_Msg(node_index, kaddr + bvec->bv_offset, bvec->bv_len, flags);

				kunmap(bvec->bv_page);
#endif
				break;

			default:
				DMS_PRINTK(PALO_DBG, PAL_MOD, "unknown op = %s \n", );
		}

		DMS_PRINTK(PALO_DBG, PAM_MOD, "LBID = %llu, flags = %d \n", lbmd->LBID, flags);

	}

	return retcode;

}

/**
 *
 * @param node_index
 * @param op
 * @param lb_list
 * @param dpt
 * @param flags
 * @return	>0:		send/recv length
 * 			-1:		NULL ptr
 * 			-32:	-EPIPE. pipe broken, or someone stop me.
 * 			-1601:	connection error
 *
 * DESCRIPTION:
 * 		Transmit LBs of one payload tag.
 */
static int __Xmit_Payload_Tag(int node_index, char op, struct LogicalBlock_List *lb_list, struct DMS_Payload_Tag *dpt, int flags)
{
	int result = 0;
	int i = 0, retcode = 0;
	struct LogicalBlock_MetaData * lbmd = NULL;
	struct Located_Request *lr = NULL;


	if(IS_LEGAL(PAM_MOD, lb_list) &&
			IS_LEGAL(PAL_MOD, dpt) )
	{

		if(PALO_DBG){
			DMS_PRINTK(PALO_DBG, PAM_MOD, "%s payload from sLB = %llu to eLB = %llu to/from DN: %s \n",
					op?"send":"recv",lr->slbid, (lr->slbid + lr->nr_lbids - 1), Get_IP_Str(node_index));
		}

		lr = dpt->lr;

		lbmd = (struct LogicalBlock_MetaData *)lr->private_data;

		for( i = 0; i < lr->nr_lbids; i++ ){

			//get lb metadata from lb_list
			lbmd = Get_LBMD_from_Node_by_LBID(lb_list, lbmd, lr->slbid+i);

			if(CHECK_PTR(PAL_MOD, lbmd)){

				flags |= ( i < lr->nr_lbids - 1 ) ? MSG_MORE:0;

				result = __Xmit_LB_Paylaod(node_index, op, lbmd, flags);

				if(result < 0){

					goto Xmit_PT_FAIL;
				}

				retcode += result;

			} else {

				eprintk(PAM_MOD, "NO~~ FATAL ERROR~! I can't find lbmd by LBID = %llu \n",
							lr->slbid+i);
			}
		}
	}


	DMS_PRINTK(PALO_DBG, PAM_MOD, "result = %d, aligned to DMSBLK = %d \n", retcode, retcode/BYTES_PER_DMSBLK);

	return retcode;


//fail out
Xmit_PT_FAIL:

	retcode = result;

	return retcode;

}

/**
 * TODO IMPROVE IT, because it will walk thru LR once again, 1. parse LR, 2. create DN_Tags (Can reduce this one?), 3. Xmit_payload_vect
 * Xmit_Payload_by_Datanode_Tag
 * @param dio
 * @param ddt
 * @return	>0:		send/recv length
 * 			-1:		NULL ptr
 * 			-32:	-EPIPE. pipe broken, or someone stop me.
 * 			-1601:	connection error
 *
 * DESCRIPTION:
 * 		Transmit payload tags of datanode tag.
 */
static int Xmit_Payload_by_Datanode_Tag(int node_index, struct DMS_IO *dio, struct DMS_Datanode_Tag *ddt)
{
	int result = 0;
	int flags = 0;
	int total_len = 0;

	if(IS_LEGAL(PAM_MOD, dio)){

		struct LogicalBlock_List *lb_list = NULL;
		struct DMS_Payload_Tag *dpt = NULL;

		lb_list = dio->LB_list;

		Reset_DList_Iterator(&ddt->payload_tags);

		while( (dpt = Get_Next_User_Data(&ddt->payload_tags)) ){

			result = __Xmit_Payload_Tag(node_index, dio->op, lb_list, dpt, flags);

			if(result < 0){

				DSTR_PRINT(ALWAYS, PAM_MOD, SPrint_DN_Service_Request(DSTR_NAME, DSTR_LIMIT, ddt), "");

				//return error directly, wait for time out and retry again.
				return result;
			}

			total_len += result;
		}


#ifdef DMS_DEBUG
		Simple_Check_Num_of_Payloads(ddt, total_len);
#endif

	}


	DMS_PRINTK(PALO_DBG, PAM_MOD, "nr_HBIDs = %d, result = %d \n",
				ddt->dn_sreq.nr_hbids, CAL_NR_DMSBLKS(total_len));

	return total_len;

}




/**
 * Get_DN_Node_Index
 * @param ssk
 * @return 	0~DMS_MAX_NODES-1	:node index
 * 			-1					:NULL ptr or improper state
 * 			-1602				:ip format error
 *
 * DESCRIPTION:
 * 		get node container index from DNC MGR. create new node if not found.
 */
int Get_DN_Node_Index(struct SSocket *ssk){

	//TODO get node container index
	int node_index = Get_Node_Index_by_SSocket(ssk);

	if(node_index == -DMS_FAIL){

		//build node connection
		node_index = Build_DMS_Node_Container(ssk->ip,
												ssk->port,
												Create_Datanode_Receiver,
												Stop_Datanode_Receiver,
												NULL,
												NULL,
												NULL );
	}

	return node_index;

}

#ifdef DMS_DEBUG
static inline void Simple_Check_Num_of_Payloads(struct DMS_Datanode_Tag *ddt, int total_len){

	int expect = ddt->dn_sreq.nr_hbids;
	int retcode = 0;
	int remain = 0;

	if(total_len > 0){

		//check total_size aligned to DMSBLK_SIZE or not.
		remain = (total_len & (~DMSBLK_MASK));

		//not DMSBLK_Size algned
		if(remain){
			eprintk(PAM_MOD, "FALTAL ERROR~! not aligned to DMSBLK_SIZE = %llu, remainder = %d \n", BYTES_PER_DMSBLK, remain);
		}

		//check whether match or not
		retcode = (expect == CAL_NR_DMSBLKS(total_len)) ? true:false;

		if(!retcode){
			eprintk(PAM_MOD, "FALTAL ERROR~! transmit nr_hbid = %d isn't match I expect = %d \n",
					CAL_NR_DMSBLKS(total_len), expect);
		}
	}

}
#endif

int Send_Request_to_Datanode(struct DMS_IO *dio, struct DMS_Datanode_Tag *ddt, struct DMS_Protocol_Header *header, union DN_Protocol_Body *body){

	int retcode = -DMS_FAIL;
	int node_index = -1, msg_flag = 0;

	if(IS_LEGAL(PAM_MOD, dio) &&
			IS_LEGAL(PAM_MOD, ddt) &&
			IS_LEGAL(PAM_MOD, header) &&
			IS_LEGAL(PAM_MOD, body) )
	{
		ddt->dn_node_index = Get_DN_Node_Index(&ddt->ssk);
		node_index = ddt->dn_node_index;

		//socket send header
		retcode = Send_Msg(node_index, (char *)header, sizeof(struct DMS_Protocol_Header), MSG_MORE);

		if(retcode < 0){
			goto SEND_FOUT;
		}

		msg_flag = (dio->op) ? MSG_MORE:0;

		//socket send body
		retcode = Send_Msg(node_index, (char *)body, header->body_length, msg_flag);

		if(retcode < 0){
			goto SEND_FOUT;
		}

		//send payload vect, if op == write/overwrite
		if(dio->op){

			retcode = Xmit_Payload_by_Datanode_Tag(node_index, dio, ddt);

		}

SEND_FOUT:
		DMS_PRINTK(PALO_DBG, PAM_MOD, "did = %llu, dn_serviceID = %llu, body_len = %d, retcode = %d \n",
					dio->did, header->serviceID, header->body_length, retcode);

	}

	return retcode;

}


//int Make_Datanode_Request(union DN_Protocol_Body *body, struct DMS_Datanode_Tag *ddt /*struct Located_Request *lr, struct DList *dn_tags*/){
//
//	int retcode = -DMS_FAIL;
//
//	if(IS_LEGAL(PAM_MOD, body) &&
//			IS_LEGAL(PAM_MOD, ddt) )
//	{
//
//		struct DMS_Payload_Tag *dpt = NULL;
//		struct Located_Request *lr = NULL;
//		short nr_hbids = 0, nr_triplets = 0;
//		int dn_index = 0;
//
//		Reset_DList_Iterator(&ddt->payload_tags);
//
//		while( (dpt = Get_Next_User_Data(&ddt->payload_tags)) ){
//
//			//struct DMS_Payload_Tag *dpt = Create_DMS_Payload_Tag(GFP_KERNEL);
//
//			lr = dpt->lr;
//			dn_index = dpt->dn_index;
//
//			if(IS_LEGAL(PAL_MOD, lr)){
//
//				nr_hbids += lr->nr_hbids;
//				nr_triplets += lr->dn_locs[dn_index];
//
//			}
//		}
//
//		Generate_DN_Request_Body(body, lr->nr_hbids, lr->dn_locs[i]->nr_triplets, &lr->HBIDs, &lr->dn_locs[j]->triplets);
//	}
//
//
//	retcode = DMS_OK;
//
//	return retcode;
//}


/**
 * Generate_Request_of_Datanode
 *
 * @param dio
 * @param ddt
 * @param header
 * @param body
 * @return	>0: header size,
 * 			-1: fail
 *
 * DESCRIPTION:
 * 		Generate dn service request header and body.
 */
int Generate_Request_of_Datanode(struct DMS_Metadata *dmeta, struct DMS_Datanode_Tag *ddt, struct DMS_Protocol_Header *header, union DN_Protocol_Body *body){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(PAM_MOD, dmeta) &&
			IS_LEGAL(PAM_MOD, ddt) &&
			IS_LEGAL(PAM_MOD, header) &&
			IS_LEGAL(PAM_MOD, body) )
	{
		//generate body
		retcode = Generate_DN_Request_Body(&ddt->dn_sreq, body);

		if(retcode < 0)
			goto GEN_DN_REQ_FAIL;

		/*
		 * we don't record ddt in private data due to we want to check it's existence again when dn reply back.
		 */
		//generate head
		retcode = Generate_DN_Protocol_Header(header, ddt->retry_count++, dmeta->op, retcode, dmeta);

		ddt->dnrid = header->serviceID;

		if(retcode < 0)
			goto GEN_DN_REQ_FAIL;


	} else {

		retcode = -DMS_FAIL;
	}

GEN_DN_REQ_FAIL:

	DMS_PRINTK(META_DBG, METAMGR_MOD, "end~!, retcode = %d \n", retcode);

	return retcode;

}

#if 0
static void __Commit_Datanode_Tag(struct DMS_Metadata *dmeta, struct DMS_Datanode_Tag *ddt, int result){


	if( IS_LEGAL(PAM_MOD, ddt) )
	{

		struct DMS_Payload_Tag *dpt = NULL;

		Reset_DList_Iterator(&ddt->payload_tags);

		//iterate all payload tags
		while( (dpt = Get_Next_User_Data(&ddt->payload_tags)) ){

			if(IS_LEGAL(PAL_MOD, dpt)){

				//commit this dpt
				dmeta->volume->vIO_Commiter(dmeta->pdio, dpt, result);

			}
		}
	}

	return;
}
#endif

int Make_and_Send_Datanode_Requests(struct DMS_IO *dio){

	int retcode = -DMS_FAIL;
	int node_index = -1;

	struct DMS_Protocol_Header header;
	union DN_Protocol_Body body;

	struct DMS_Metadata *dmeta = NULL;
	struct DMS_Datanode_Tag *ddt = NULL;

	if(IS_LEGAL(PAM_MOD, dio) &&
			IS_LEGAL(PAM_MOD, dio->dmeta) )
	{

		memset(&body, 0, sizeof(union DN_Protocol_Body));

		dmeta = dio->dmeta;

		Reset_DList_Iterator(&dmeta->dn_tags);

		while( (ddt = Get_Next_User_Data(&dmeta->dn_tags)) ){

			if( Generate_Request_of_Datanode(dmeta, ddt, &head, &body) ){

				//TODO Set FSM state in dio

				//send to datanode
				if( Send_Request_to_Datanode(dio, ddt, &head, &body) < 0 ){

					//FIXME we should retry again.
					//error handling
					dmeta->volume->vIO_Commiter(dmeta->pdio, ddt, false);
				}

			} else {

				goto FAIL_OUT;
			}
		}
	}


	retcode = DMS_OK;

	return retcode;

FAIL_OUT:

	return -DMS_FAIL;

}

int Resend_Datanode_Requests(){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(PAL_MOD, dio) )
	{

//		struct DMS_Metadata *dmeta = dio->dmeta;
//
//		switch(dio->op){
//
//			case DMS_OP_READ:
//
//				//choose one of dns
//				retcode = Translate_Read_Located_Requests_to_DN_Tags(dmeta);
//
//				break;
//
//			case DMS_OP_WRITE:
//			case DMS_OP_OVERWRITE:
//
//				retcode = Translate_Write_Located_Requests_to_DN_Tags(dmeta);
//
//				break;
//
//			default:
//				DMS_PRINTK(PALO_DBG, PAL_MOD, "unknown op = %s \n", );
//		}


	}

	DMS_PRINTK(PALO_DBG, PAL_MOD, "end, retcode = %d \n", retocde);

	return retcode;

}

int Payload_Handler(struct DMS_IO *dio){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(PAM_MOD, dio)){

		//generate dn tags first
		retcode = Generate_All_DN_Tags(dio);

		//generate and send dn_requests
		Make_and_Send_Datanode_Requests(dio);

	}

	retcode = DMS_OK;

	return retcode;
}




/********************************************************************************/
/*																				*/
/*								Datanode Receiver								*/
/*																				*/
/********************************************************************************/

int DN_Req_Sanity_Check(struct DMS_Protocol_Header *header){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(PAM_MOD, header) &&
			IS_LEGAL(PAM_MOD, header->extention) )
	{
		struct DMS_Metadata *dmeta = header->extention;

		if( IS_LEGAL(PAM_MOD, header) )
		{
			struct DMS_IO *dio = dmeta->pdio;

			//lock dio
			write_lock(&dio->lock);

			//TODO check dio state(in send_nn?), time stamp(within available time), did and magic_num of dio

			//TODO check timestamp

			//TODO check dio existence

			//TODO I shouldn't remove anything, includes my ddt from dmeta->dn_tags. because timeout_worker will do it for me.

			//TODO cancel fsm timer

			//TODO set fsm state

			//			DMS_PRINTK(META_DBG, METAMGR_MOD, "did = %d, serviceID = %d \n",
			//								dio->did, header->serviceID);


			//unlock dio
			write_unlock(&dio->lock);

		}
	}


	return retcode;

}


int Do_Read_Ack(int node_index, struct DMS_Metadata *dmeta, struct DMS_Datanode_Tag * ddt){

	int retcode = 0;

	union DN_Protocol_Body body;
	struct DN_Service_SResponse sresp;

	if(IS_LEGAL(PAM_MOD, sresp)){

		//read body
		struct DMS_IO *dio = dmeta->pdio;

		//recv DN_Service_SResponse
		Recv_Msg(node_index, &body, sizeof(struct DN_Service_SResponse), MSG_WAITALL);

		Parse_DN_Service_SResponse(&body, &sresp);

		//recv payload
		retcode = Xmit_Payload_by_Datanode_Tag(node_index, dio, ddt);

	}


	return retcode;
}

/**
 *
 * @param data
 * @param result
 * @return	1:	commit true.
 * 			0:	waiting.
 * 			-1:	commit fail.
 */
int Default_Ack_Policy(void *data, int result){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(PAM_MOD, data)){

		struct DMS_Datanode_Tag * ddt = (struct DMS_Datanode_Tag *)data;


	}

	return retcode;

}

int Commit_Datanode_Tags(struct DMS_Metadata *dmeta, struct DMS_Datanode_Tag * ddt, int ack){

	int retcode = -DMS_FAIL;

	if( CHECK_PTR(PAM_MOD, ddt) )
	{

		if( ack == WMEM_ACK || ack == PWMEM_ACK || ack < 0 ){

			/*
			 * keep ddt for
			 * 1. put ddt back to dn_tags for waiting disk ack or
			 * 2. committing error to namenode.
			 */
			retcode = Put_Datanode_Tag_to_Datanode_Tags(&dmeta->dn_tags, ddt);

			//read error and we needn't refresh metadata. ex: time out.
			if(dmeta->op == DMS_OP_READ){
				//TODO get from different dn,
				//new approach: commit fail to upper layer and resend from upper layer when dn reply HBID_ERROR, ex: meta-data mgr
			}

		} else {

			//success, remove and release the tag.
			Remove_DMS_Datanode_Tag(ddt);

			retcode = DMS_OK;
		}

	}


	return ack;

}

int Process_Datanode_Response(int node_index, struct DMS_Protocol_Header *header){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(PAM_MOD, header) &&
			IS_LEGAL(PAM_MOD, header->extention) )
	{

		struct DMS_Metadata *dmeta = header->extention;

		struct DMS_Datanode_Tag *ddt = Get_Datanode_Tag_from_Datanode_Tags(&dmeta->dn_tags, header->serviceID);

		if( CHECK_PTR(PAM_MOD, ddt) )
		{

			switch(header->sub_type){

				case READ_ACK:

					retcode = Do_Read_Ack(node_index, dmeta, ddt);

					break;

				case WMEM_ACK:
				case PWMEM_ACK:

					//commit
					//__Commit_Datanode_Tag(dmeta, ddt, header->sub_type);

					//put ddt back to dn_tags for waiting disk ack
					//Put_Datanode_Tag_to_Datanode_Tags(&dmeta->dn_tags, ddt);


					break;

				case WDISK_ACK:
				case PWDISK_ACK:

					//commit
					//__Commit_Datanode_Tag(dmeta, ddt, header->sub_type);

					//add ddt back to dn_tags for waiting disk ack
					//Remove_DMS_Datanode_Tag(ddt);


					break;


				case PARTIAL_FAIL:
#if 0
					//recv nr_SHBIDs
					short nr_ftriplets = 0;
					Recv_Msg(node_index, &nr_ftriplets, sizeof(nr_ftriplets), MSG_WAITALL);
					nr_ftriplets = READ_NET_SHORT(&nr_ftriplets, 0);

					retcode = false;
#endif

					break;

				case UNDEFINED:
				case EXCEPTION:
				case IOEXCEPTION:
				case IOFAIL_BUT_LUN_OK_EXCEPTION:
				case FAILED_LUN_EXCEPTION:

				default:

					eprintk(PAM_MOD, "FATAL ERROR~! ack type = %d, %s, from DN = %s \n",
							header->sub_type, __DN_Type_ntoc(header->sub_type), Get_IP_Str(ddt->dn_node_index));


					DSTR_PRINT( ALWAYS, PAL_MOD, SPrint_DMS_Datanode_Tag(DSTR_NAME, DSTR_LIMIT, ddt), "");


			}

		}

	}


	return retcode;
}


int Datanode_Receiver(void *data){

	int node_index = (int) data;
	int retcode = 0;

	struct DMS_Protocol_Header header;
	struct DMS_Metadata *dmeta = NULL;

	DMS_PRINTK(DNH_DBG, PAM_MOD, "%s, ip = %s \n", current()->comm, Get_IP_Str(node_index));

	while( !kthread_should_stop() ){

		//receive from socket
		retcode = Recv_Msg(node_index, (char *)&header, sizeof(header), MSG_WAITALL);

		//proccess result
		if(retcode < 0){

			goto OUT;
		}

		//parse header
		retcode = Parse_DN_Protocol_Header(&header);

		if( retcode < 0 ){

			goto OUT;
		}

		dmeta = DN_Req_Sanity_Check(&header);

		if( CHECK_PTR(PAM_MOD, dmeta) ){

			Process_Datanode_Response(node_index, &header);

			dmeta->volume->vIO_Commiter(dmeta->pdio, ddt, false);

		}else{

			//TODO go forward to next magic number
		}



//		if(header->body_length > buf_size){
//
//			//grow up the buffer
//			buf_size = __Grow_up_Buffer(buf_ptr, buf_size);
//
//		}

		//receive body from socket
//		retcode = Recv_Msg(node_index, &body, header->body_length, 0);

//		if(retcode < 0){
//
//			goto OUT;
//		}

		//parse body protocol
//		retcode = Parse_DN_Protocol_Body(header, *buf_ptr, body);

	}

OUT:

	DMS_PRINTK(PALO_DBG, PAM_MOD, "retcode = %d \n", retcode);

	return retcode;
}

struct task_struct * Create_Datanode_Receiver(int node_index, void *data){

	int port = -1;
	char *ip_str = NULL;

	struct task_struct *receiver = NULL;

	ip_str = Get_IP_Str(node_index);

	DMS_PRINTK(DNH_DBG, PAM_MOD, "name = dn_receiver[%d], addr = %s \n", node_index, ip_str);

	receiver = kthread_create(Datanode_Receiver, (void *)node_index, "dn_recver[%d]", node_index);

	return receiver;
}


void Stop_Datanode_Receiver(struct task_struct *receiver){

	if(pid_alive(receiver)){

		DMS_PRINTK(DNH_DBG, PAM_MOD, "stopping %s \n", &receiver->comm);

		kthread_stop(receiver);

		DMS_PRINTK(DNH_DBG, PAM_MOD, "stopped %s \n", &receiver->comm);

	}else{

		DMS_PRINTK(DNH_DBG, PAM_MOD, "thread has dead... dn_receiver = %p \n", receiver);
	}
}









int Init_Payload_Handler(void){

	int retcode = -DMS_FAIL;

	//TODO I need a data structure to record what sockets I have. ie, key = ip:port, value is node_index.

	retcode = DMS_OK;

	return retcode;
}

void Release_Payload_Handler(void){

}

