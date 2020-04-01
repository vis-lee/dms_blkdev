/*
 * DIO.c
 *
 *  Created on: Mar 15, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include "DMS_Common.h"
#include "DIO.h"
#include "LogicalBlock.h"
#include "Allocation_Flag.h"
#include "Volume.h"
#include "IO_Request.h"
#include "DMS_VDDriver.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *DIO_MOD = "DIO: ";

static atomic64_t dio_id_gen = ATOMIC64_INIT(1);

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

int SPrint_DIO(char *buf, int len_limit, struct DMS_IO *dio){

	int len = 0;

	if( IS_LEGAL(DIO_MOD, dio) &&
			IS_LEGAL(DIO_MOD, buf) )
	{

		if(len_limit > 0){

			len += sprintf(buf+len, "\t%s, volume = %lld, did = %llu, sLBID = %llu, nr_LBIDs = %llu, op = %s, state = %d, dio->dmeta = %p, magic = %u \n",
					DIO_MOD, dio->volume->volumeID, dio->did, dio->sLBID, dio->nr_LBIDs, op2str(dio->op), atomic_read(&dio->state), dio->dmeta, dio->magic);

			if(len_limit-len > 0){
				//print LBs
				len += SPrint_Simple_LogicalBlock_List(buf+len, len_limit-len, dio->LB_list);
			}
		}

	}

	return len;

}

void Print_DIO(struct DMS_IO *dio){

	if(IS_LEGAL(DIO_MOD, dio) )
	{

		DMS_PRINTK(ALWAYS, DIO_MOD, "volume = %lld, did = %llu, sLBID = %llu, nr_LBIDs = %llu, op = %s, state = %d, dio->dmeta = %p, magic = %u \n",
				dio->volume->volumeID, dio->did, dio->sLBID, dio->nr_LBIDs, op2str(dio->op), atomic_read(&dio->state), dio->dmeta, dio->magic);

	}


}

inline int Init_DIO(struct IO_Request *ior, struct DMS_IO *dio, char op)
{

	int retcode = DMS_OK;

	if(IS_LEGAL(DIO_MOD, dio) &&
			IS_LEGAL(DIO_MOD, ior) )
	{
		struct request *kreq = ior->kreq;

		dio->did = atomic64_inc_return(&dio_id_gen);

		dio->volume = Get_DMS_Volume_from_KRequest(kreq);
		//dio->kreq = kreq;
		dio->io_req = ior;

		dio->sLBID = 0;
		dio->nr_LBIDs = 0;

		dio->op = op;

#ifdef ENABLE_TCQ
		//the index in the TCQ
		dio->tag = kreq->tag;
#endif

		//the LB List
		dio->LB_list = NULL;

		atomic_set(&dio->state, 0);

		dio->dmeta = NULL;

		dio->retry_count = 1;

		//set result to true
		atomic_set(&dio->result, true);

		//set magic num
		dio->magic = DIO_MAGIC;

	}

	return retcode;
}


struct DMS_IO * Create_DIO(struct IO_Request *ior, char op){

	struct DMS_IO *dio = NULL;

	dio = Malloc_DIO(GFP_KERNEL);

	if( Init_DIO(ior, dio, op) < 0){
		goto Init_DIO_Fail;
	}

	DMS_PRINTK(DIO_DBG, DIO_MOD, "dio = %p, dio_id = %llu \n", dio, dio->did);

	return dio;

Init_DIO_Fail:
	if( CHECK_PTR(DIO_MOD, dio) ){
		Release_DIO(dio);
	}

	PRINT_DMS_ERR_LOG(EDEV_NOMEM, DIO_MOD, "");

	return NULL;
}


void Release_DIO(struct DMS_IO *dio){

	ulong64 did = 0;

	if(IS_LEGAL(DIO_MOD, dio)){

		did = dio->did;

		DMS_PRINTK(DIO_DBG, DIO_MOD, "dio_id = %llu \n", did);

		//release LB_List
		Release_LogicalBlock_List(dio->LB_list);
		dio->LB_list = NULL;

		//reset magic num
		dio->magic = 0;

		Free_DIO(dio);

	}

	DMS_PRINTK(DIO_DBG, DIO_MOD, "dio_id = %llu, done! \n", did);

}


void __Check_BIO(struct bio *bio){

	unsigned int bio_size = bio->bi_size;

	//if the size isn't 4k aligned
	if( bio_size << (32-DMSBLK_SHIFT_BITS) ){
		__Print_BIO(-1, bio);
	}

}

struct LogicalBlock_List * Generate_All_LogicalBlocks(struct request *kreq){

	ulong64 bi_sLBID = 0;

	struct LogicalBlock_List *lb_list = NULL;
	struct LogicalBlock_MetaData *lbmd = NULL;

	struct bio *bio = NULL;
	int /*i = 0, */j = 0;

	lb_list = Create_LogicalBlock_List();

	if(IS_LEGAL(DIO_MOD, lb_list)){

		//walk thru all bio and create corresponding
		rq_for_each_bio(bio, kreq) {

			struct bio_vec *bvec = NULL;

			//they should be aligned to DMSBLK_SIZE
			bi_sLBID = KSECTORS_TO_LBID(bio->bi_sector);

			if(DIO_DBG){
				__Check_BIO(bio);
			}

			//start from 0
			__bio_for_each_segment(bvec, bio, j, 0) {

				lbmd = Create_LogicalBlock_MetaData();

				if(IS_LEGAL(DIO_MOD, lbmd)){

					lbmd->LBID = bi_sLBID + j;
					lbmd->bio = bio;
					lbmd->vec_idx = j;

					Add_to_LogicalBlock_List_by_Insertion_Sort(lb_list, lbmd);

					lbmd = NULL;
				}
			}
		}

		if(DIO_DBG)
			Print_Simple_LogicalBlock_List(__func__, lb_list);
	}



	return lb_list;

}

/*
 * check dio start LBID whether match sub_list or not
 */
void __Check_DIO_and_Sub_List(struct DMS_IO *dio){

	if(IS_LEGAL(DIO_MOD, dio)){

		struct LogicalBlock_List *lb_list = dio->LB_list;
		struct list_head *head = &lb_list->LB_list_head;

		struct LogicalBlock_MetaData *front = list_entry(head->next, struct LogicalBlock_MetaData, list_node);
		struct LogicalBlock_MetaData *tail = list_entry(head->prev, struct LogicalBlock_MetaData, list_node);

		ulong64 last_LBID = (dio->sLBID + dio->nr_LBIDs - 1);

		if( dio->sLBID != front->LBID ||  last_LBID != tail->LBID )
			eprintk(DIO_MOD, "Fatal Error! make_dio sLBID = %llu, isn't match LB_List's front_LBID = %llu; last_LBID = %llu, tail_LBID = %llu \n",
					dio->sLBID, front->LBID, last_LBID, tail->LBID);

	}

}

/**
 * Get_Continuous_Range - calculate how many continuous 0 or 1 bits within nr_LBIDs range by checking allocation flag.
 *
 * @param volume : used to get allocation flag map
 * @param op : used to decide check 0 or 1
 * @param sLBID : start LBID.
 * @param nr_LBIDs : the check range.
 * @return length : how many bits.
 *
 * DESCRIPTION:
 * 		if op is DMS_OP_WRITE, then we want to calculate how many 0 bits in allocation flag map. Otherwise,
 * 		DMS_OP_OVERWRITE check 1 bits. N.B., end_index present the contiguous end +1,
 * 		ex1: if flag = 01000000, op = DMS_OP_WRITE, sLBID = 1, nr_LBIDs = 6;
 * 		the result should be: next_index = 6 (start from 0), end_index = 5, range_index = 6.
 * 		therefore, the length should be: 5 - 1 + 1 = 5;
 *
 * 		ex2: if flag1 = 0xFF...FF (64bits), op = DMS_OP_OVERWRITE, sLBID = 0, nr_LBIDs = 128;
 * 		in the first round, the result should be: next_index = 64 (no zero_bit), end_index = 63, range_index = 127.
 * 		therefore, the length should be: 63 - 0 + 1 = 64; and recursive to next round.
 *
 * 		2nd round: flag2 = 0xFF...FF (64bits), op = DMS_OP_OVERWRITE, sLBID = 64, nr_LBIDs = 64, start_index = 0;
 * 		result: next_index = 64 (no zero_bit), end_index = 63, range_index = 63.
 * 		the length should be: 63 - 0 + 1 = 64; and end recursive.
 *
 * 		(btw, another case: if end_index over range_index it will shift back. ie: range_index = 1, end_index also became 1)
 *
 */
int __Get_Continuous_Range(struct DMS_Volume *volume, char op, ulong64 sLBID, ulong64 nr_LBIDs){

	int start_index = sLBID % CHUNK_SIZE;

	int length = 0, next_index = 0, end_index = 0, range_index = start_index + nr_LBIDs - 1;

	if(IS_LEGAL(DIO_MOD, volume)){

		ulong64 alf = Get_Allocation_Flag(volume, sLBID);


		switch(op){

			case DMS_OP_WRITE:
				//find continuous 0, until next "1" bit.
				next_index = find_next_bit(&alf, BITS_PER_LONG, start_index);
				break;

			case DMS_OP_OVERWRITE:
				next_index = find_next_zero_bit(&alf, BITS_PER_LONG, start_index);
				break;

			default:
				eprintk(DIO_MOD, "unknown op code!");
		}

		end_index = next_index - 1;

		/*
		 * end_index: 0~63
		 * range_index is the range boundary which I am interesting.
		 */
		if(end_index > range_index){
			end_index = range_index;
		}

		DMS_PRINTK(DIO_DBG, DIO_MOD, "sLBID = %llu, nr_LBIDs = %llu, alf = %16llx, next_index = %d, end_index = %d, range_index = %d \n",
				sLBID, nr_LBIDs, alf, next_index, end_index, range_index);

		length = (end_index - start_index) + 1;

		/* bits was continuously match op in this flag chunk,
		 * let us check next chunk by recursive call, if we haven't reach nr_LBIDs. */
		if( nr_LBIDs > length && next_index == BITS_PER_LONG ){

			//walk to next start LBID
			sLBID += length;

			nr_LBIDs -= length;

			//recursive to get next chunk
			length += __Get_Continuous_Range(volume, op, sLBID, nr_LBIDs);

		}
	}

	DMS_PRINTK(DIO_DBG, DIO_MOD, "end~ length = %d \n", length);


	return length;

}


int Get_Allocation_Range(struct request *kreq, ulong64 sLBID, ulong64 nr_LBIDs, char *op){

	ulong64 alf = 0;
	struct DMS_Volume *volume = NULL;

	volume = Get_DMS_Volume_from_KRequest(kreq);

	alf = Get_Allocation_Flag(volume, sLBID);

	(alf & (1 << (sLBID % CHUNK_SIZE))) ? (*op = DMS_OP_OVERWRITE) : (*op = DMS_OP_WRITE);

	DMS_PRINTK(DIO_DBG, DIO_MOD, "op = %s \n", op2str(*op));

	return __Get_Continuous_Range(volume, *op, sLBID, nr_LBIDs);

}


int __Move_Sub_List(struct list_head *orig_head, int length, struct list_head *new_head){

	int retcode = 0;

	struct list_head *cur = NULL;

	if(IS_LEGAL(DIO_MOD, orig_head) &&
			IS_LEGAL(DIO_MOD, new_head))
	{
		list_for_each(cur, orig_head){

			if(++retcode == length){

				//sub_list link to new_head
				new_head->next = orig_head->next;
				(new_head->next)->prev = new_head;

				//re-direct orig_head to link up remain list
				orig_head->next = cur->next;
				(cur->next)->prev = orig_head;

				//complete new_list
				new_head->prev = cur;
				cur->next = new_head;

				break;
			}
		}
	}

	return retcode;
}


struct LogicalBlock_List * Get_Next_DIO_Request_LB_List(struct LogicalBlock_List *lb_list, int length){

	int retcode = 0;
	unsigned long flags = 0;
	struct LogicalBlock_List *sub_list = Create_LogicalBlock_List();

	if(IS_LEGAL(DIO_MOD, sub_list)){

		if(DIO_DBG){
			Print_Simple_LogicalBlock_List("orig_list: ", lb_list);
		}

		//lock lb_list first
		write_lock_irqsave(&lb_list->LB_list_lock, flags);

		//needn't to lock new list
		retcode = __Move_Sub_List(&lb_list->LB_list_head, length, &sub_list->LB_list_head);

		write_unlock_irqrestore(&lb_list->LB_list_lock, flags);

		if(retcode != length){
			eprintk(DIO_MOD, "you ask me too much. you want length = %d, but I only have list_length = %d \n", length, retcode);
			Release_LogicalBlock_List(sub_list);
			sub_list = NULL;
		}

		if(DIO_DBG){
			Print_Simple_LogicalBlock_List("r_list: ", lb_list);
			Print_Simple_LogicalBlock_List("sub_list: ", sub_list);
			WARN_ON(sub_list == NULL);
		}

	}

	return sub_list;
}


/**
 * __Make_DIO_Requst - default volume vIO_func
 * @param volume
 * @param kreq
 * @return
 *
 * DESCRIPTION:
 *
 */
struct IO_Request * __Make_DIO_Requst(struct request *kreq){

	int length = 0;
	ulong64 sLBID = 0, nr_LBIDs = 0;
	char op = 0;

	struct DMS_IO *dio = NULL;
	struct LogicalBlock_List *lb_list = NULL;

	//create IO_Request
	struct IO_Request *ior = Create_IO_Request(kreq);

	if(IS_LEGAL(DIO_MOD, kreq) &&
			CHECK_PTR(DIO_MOD, ior) )
	{

		op = rq_data_dir(kreq);

		sLBID = KSECTORS_TO_LBID(kreq->sector);
		nr_LBIDs = KSECTORS_TO_DSECTORS(kreq->nr_sectors);

		DMS_PRINTK(DIO_DBG, DIO_MOD, "op = %s, sLBID = %llu, nr_LBIDs = %llu, sector = %llu, nr_sector = %lu \n",
				op2str(op), sLBID, nr_LBIDs, kreq->sector, kreq->nr_sectors);

		lb_list = Generate_All_LogicalBlocks(kreq);

		while(nr_LBIDs){

			//create dio
			dio = Create_DIO(ior, op);

			if( IS_LEGAL(DIO_MOD, dio) ){

				if(op){

					//get continuous range and op
					length = Get_Allocation_Range(kreq, sLBID, nr_LBIDs, &op);

					dio->op = op;

					/* get corresponding LB list */
					if(length == nr_LBIDs){

						dio->LB_list = lb_list;
						lb_list = NULL;

					}else{

						//get sub_list, TODO do we need to aggregate LBs into a dio to reduce nn communicate.
						dio->LB_list = Get_Next_DIO_Request_LB_List(lb_list, length);
					}

					if(dio->LB_list){

						dio->sLBID = sLBID;
						dio->nr_LBIDs = length;

						if(DIO_DBG)
							__Check_DIO_and_Sub_List(dio);

						//set reference count
						/*atomic_set(&dio->ref_cnt, length);*/

						sLBID += length;
						nr_LBIDs -= length;

						DMS_PRINTK(DIO_DBG, DIO_MOD, "did = %llu, sLBID = %llu, nr_LBIDs = %llu \n",
										dio->did, sLBID, nr_LBIDs);

					}else{

						goto CREATE_DIO_FAIL;
					}


				}else{

					/*
					 * op read, we just move lb_list to dio and that's all.
					 */
					dio->LB_list = lb_list;
					dio->sLBID = sLBID;
					dio->nr_LBIDs = nr_LBIDs;

					/*atomic_set(&dio->ref_cnt, nr_LBIDs);*/

					nr_LBIDs = 0;
				}

				atomic_set(&dio->state, DIO_INIT);

				//add to ior
				Add_DIO_to_IO_Reqeust(ior, dio);

			}

		}

		//add ior to kreq
		//Add_IO_Reqeust_to_KRequest(kreq, ior);

	}

	return ior;

CREATE_DIO_FAIL:

	//free IO_Req, it will release dio also
	if( ior ){
		Release_IO_Request(ior);
	}

	if( lb_list ){
		//it releases remained lbmds also.
		Release_LogicalBlock_List(lb_list);
	}

	//fail this kreq
	DMS_End_Request_Handler(kreq, kreq->nr_sectors, false);

	return NULL;
}




/**
 *
 * @param dio
 * @param lbid
 * @param result
 * @return	1:		 	success, and list is empty. dio will be release here.
 * 			0:			if success, but list is not empty
 * 			-1:			fail, null ptr.
 */
int Commit_LB_to_DMS_IO(struct DMS_IO *dio, ulong64 slbid, int nr_lbids, int result){

	int empty = -DMS_FAIL;
	int retcode = -DMS_FAIL;
	//unsigned long flags = 0;

	if(IS_LEGAL(DIO_MOD, dio) /*&&
			IS_LEGAL(DIO_MOD, lbmd)*/)
	{
		//TODO do we need lock?
		//write_lock_irqsave(&dio->lock, flags);

		//if old result is true then change, otherwise don't.
		atomic_cmpxchg(&dio->result, true, result);

		//TODO can we preform batch remove?
		//remove and free LBMD from list in IO_REQ
		//empty = Free_LogicalBlock_MetaData_from_LB_List_by_LBID(dio->LB_list, lbid);
		empty = Commit_LB_List_by_Range(dio->LB_list, slbid, nr_lbids);

		//if list empty or this dio can't be completed.
		if(empty == true /*|| result != true*/  /*&& (empty = atomic_dec_and_test(&dio->ref_cnt)) == true*/){

			//TODO state transition

			//commit to IO_Request when lb ref_cnt == 0
			retcode = Commit_DIO_to_IO_Request(dio, atomic_read(&dio->result));

		}

		//write_unlock_irqrestore(&dio->lock, flags);

	}

	return empty;

}



#ifdef DMS_UTEST

EXPORT_SYMBOL(__Make_DIO_Requst);
EXPORT_SYMBOL(Commit_LB_to_DMS_IO);

#endif





