/*
 * LogicalBlockList.c
 *
 *  Created on: 2011/5/3
 *      Author: Vis Lee
 */

#include <linux/list.h>

#include "LogicalBlock.h"
#include "DMS_Common.h"
//#include "DMS_type.h"



/********************************************************************************/
/*																				*/
/*								PARAMETERS										*/
/*																				*/
/********************************************************************************/
/* DMS Logical Block Module */
char *LB_MOD =				"LB_List: ";


/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/



/********************************************************************************/
/*																				*/
/*									FUNCS										*/
/*																				*/
/********************************************************************************/
//******************************************* struct LogicalBlock_MetaData start *****************************************

//TODO print block info
#if 0
if (unlikely(block_dump)) {
	char b[BDEVNAME_SIZE];
	printk(KERN_DEBUG "%s(%d): %s block %Lu on %s\n",
		current->comm, current->pid,
		(rw & WRITE) ? "WRITE" : "READ",
		(unsigned long long)bio->bi_sector,
		bdevname(bio->bi_bdev,b));
}
#endif

/*
 * create LB and return to user
 */
struct LogicalBlock_MetaData * Create_LogicalBlock_MetaData(void){

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! \n");

	//struct LogicalBlock_MetaData *lbmd = Malloc_LBMD(GFP_NOWAIT);
	struct LogicalBlock_MetaData *lbmd = Malloc_LBMD(GFP_KERNEL);

	Init_LogicalBlock_MetaData(lbmd);

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! LBMD = %p\n", lbmd);

	return lbmd;
}

/*
 * initialize related data structure
 */
void Init_LogicalBlock_MetaData(struct LogicalBlock_MetaData *lbmd){

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! LBMD = %p\n", lbmd);

	if(IS_LEGAL(LB_MOD, lbmd)){

		//set to 0
		memset(lbmd, 0, sizeof(struct LogicalBlock_MetaData));

		//allocate spin_lock
		//lbmd->LB_lock = (spinlock_t *)CCMA_malloc_NOWAIT(sizeof(spinlock_t));

		//init list_node.
		INIT_LIST_HEAD(&lbmd->list_node);

		//init lock of LogicalBlock_MetaData
		rwlock_init(&lbmd->LB_lock);

//		lbmd->fsm.lbmd = lbmd;
	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! LBMD = %p\n", lbmd);

}

/*
 * free memory of struct LogicalBlock_MetaData
 */
void Release_LogicalBlock_MetaData(struct LogicalBlock_MetaData *lbmd){

//	unsigned long flags = 0;
	ulong64	LBID = 0;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! LBMD = %p \n", lbmd);

	if(IS_LEGAL(LB_MOD, lbmd) /*&&
		IS_LEGAL(LB_MOD, lbmd->LB_lock)*/ )
	{

//		spinlock_t *LB_lock = lbmd->LB_lock;

//		spin_lock_irqsave(LB_lock, flags);

//		rwlock_acquire();
//		read_lock_irqsave()

		LBID = lbmd->LBID;

		Free_LBMD(lbmd);

//		spin_unlock_irqrestore(LB_lock, flags);

		//TODO if the other guy also want to get lock simultaneously. think about it.
//		kfree(LB_lock);

	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! LBMD = %p, LBID = %llu\n", lbmd, LBID);
}


//******************************************* struct LogicalBlock_MetaData end *******************************************




//***************************************************** LB_list start ****************************************************

/*
 * initialize related data structure
 */
int Init_LogicalBlock_List(/*struct DMS_IO_Request dms_io, */struct LogicalBlock_List *LB_list){

	int retcode = DMS_OK;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! LB_list = %p\n", LB_list);

	if(IS_LEGAL(LB_MOD, LB_list)){

		//init LB_list.
		 INIT_LIST_HEAD(&LB_list->LB_list_head);

		 //init lock of LB_list
		 rwlock_init(&LB_list->LB_list_lock);

		 /*LB_list->dms_io = dms_io;*/

	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! LB_list = %p\n", LB_list);

	return retcode;
}

/*
 * walk through the LogicalBlock_List and free all elements.
 */
void Release_LogicalBlock_List(struct LogicalBlock_List *LB_list){

	unsigned long flags = 0;
	struct LogicalBlock_MetaData *cur = NULL, *next = NULL;

	DMS_PRINTK(LB_DBG, LB_MOD, "LB_list = %p\n", LB_list);

	if(IS_LEGAL(LB_MOD, LB_list)){

		write_lock_irqsave(&LB_list->LB_list_lock, flags);

		list_for_each_entry_safe(cur, next, &LB_list->LB_list_head, list_node){

			Release_LogicalBlock_MetaData(cur);

		}

		write_unlock_irqrestore(&LB_list->LB_list_lock, flags);
	}

	Free_LB_List(LB_list);

	DMS_PRINTK(LB_DBG, LB_MOD, "LB_list = %p\n", LB_list);

}


struct LogicalBlock_List * Create_LogicalBlock_List(void){

	struct LogicalBlock_List *lb_list = NULL;

	lb_list = Malloc_LB_List(GFP_KERNEL);

	if( Init_LogicalBlock_List(lb_list) < 0){
		//fail
		goto LB_INIT_FAIL;
	}

	return lb_list;

LB_INIT_FAIL:

	if( CHECK_PTR(LB_MOD, lb_list) ){

		Free_LB_List(lb_list);
	}

	PRINT_DMS_ERR_LOG(EDEV_NOMEM, LB_MOD, "");

	return NULL;

}

#if 0
int __Create_and_Insert_LBMDs(struct LogicalBlock_List *lb_list, ulong64 sLBID, ulong64 nr_LBID){

	int retcode = DMS_OK;
	int i = 0;

	struct LogicalBlock_MetaData *lbmd = NULL;

	if( IS_LEGAL(LB_MOD, lb_list)){

		for( i = 0; i < nr_LBID; i++){

			//create LBMD
			lbmd = Create_LogicalBlock_MetaData();

			lbmd->LBID = sLBID + i;

			if( Add_to_LogicalBlock_List_by_Insertion_Sort(lb_list, lbmd) < 0){
				return -DMS_FAIL;
			}

		}

	}

	return retcode;
}

struct LogicalBlock_List * Create_LogicalBlock_List_and_LBMD(ulong64 sLBID, ulong64 nr_LBIDs){

	struct LogicalBlock_List *lb_list = NULL;

	lb_list = Create_LogicalBlock_List();

	if( __Create_and_Insert_LBMDs(lb_list, sLBID, nr_LBIDs) < 0 ){

		goto LB_CI_FAIL;

	}

	return lb_list;

LB_CI_FAIL:

	if( CHECK_PTR(lb_list) ){

		Release_LogicalBlock_List(lb_list);
	}

	return NULL;

}
#endif

/*
 * check whether LB_List is empty or not.
 * return true, if list is empty, otherwise false.
 */
int Is_LB_List_Empty(struct LogicalBlock_List *LB_list){

	int is_empty = false;
	unsigned long flags = 0;

	if(IS_LEGAL(LB_MOD, LB_list)){

		read_lock_irqsave(&LB_list->LB_list_lock, flags);
		is_empty = list_empty(&LB_list->LB_list_head);
		read_unlock_irqrestore(&LB_list->LB_list_lock, flags);
	}

	return is_empty;
}


/*
 * get corresponding LB_MetaData from Logical Block list by LBMD ptr,
 * This API returns the pointer of LogicalBlock_MetaData only. The LogicalBlock_MetaData won't be deleted from LB_List.
 */
struct LogicalBlock_MetaData * Get_LogicalBlock_By_LBMD_ptr(struct LogicalBlock_List *LB_list, struct LogicalBlock_MetaData *lbmd){

	unsigned long flags = 0;
	struct LogicalBlock_MetaData * tmp = NULL;
	struct LogicalBlock_MetaData * match = NULL;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! target LBMD = %p\n", lbmd);

	if( IS_LEGAL(LB_MOD, LB_list) &&
			IS_LEGAL(LB_MOD, lbmd) )
	{

		read_lock_irqsave(&LB_list->LB_list_lock, flags);

		list_for_each_entry(tmp, &LB_list->LB_list_head, list_node) {

			if( tmp == lbmd ) {

				DMS_PRINTK(LB_DBG, LB_MOD, "found LogicalBlock_MetaData by LBMD ptr = %p\n", lbmd);

				match = tmp;

				break;
			}
		}

		read_unlock_irqrestore(&LB_list->LB_list_lock, flags);

	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! target LBMD = %p, match ptr = %p\n", lbmd, match);

	return match;

}

/*
 * get corresponding LogicalBlock_MetaData from Logical Block list by LBID,
 * This API returns the pointer of LogicalBlock_MetaData only. The LogicalBlock_MetaData won't be deleted from LB_List.
 */
struct LogicalBlock_MetaData * Get_LogicalBlock_By_LBID(struct LogicalBlock_List *LB_list, ulong64 lbid){

	unsigned long flags = 0;
	struct LogicalBlock_MetaData * tmp = NULL;
	struct LogicalBlock_MetaData * match = NULL;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! LBID = %llu\n", lbid);

	if( IS_LEGAL(LB_MOD, LB_list) )
	{

		read_lock_irqsave(&LB_list->LB_list_lock, flags);

		list_for_each_entry(tmp, &LB_list->LB_list_head, list_node) {

			if( tmp->LBID == lbid ) {

				DMS_PRINTK(LB_DBG, LB_MOD, "found LogicalBlock_MetaData = %p, by LBID = %llu \n",
							   tmp, lbid);
				match = tmp;

				break;
			}
		}

		read_unlock_irqrestore(&LB_list->LB_list_lock, flags);

	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! LBID = %llu, match ptr = %p\n", lbid, match);

	return match;

}

#if 0
/*
 * add one LogicalBlock_MetaData to the Logical Block list
 */
int Add_to_LogicalBlock_List(struct LogicalBlock_List *LB_list, struct LogicalBlock_MetaData *lbmd){

	int retcode = -DMS_FAIL;
	unsigned long flags = 0, lbmd_flags = 0;
	struct LogicalBlock_MetaData *tmp = NULL, *match = NULL;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! LB_MD = %p\n", lbmd);

	if( IS_LEGAL(LB_MOD, LB_list) &&
			IS_LEGAL(LB_MOD, lbmd) )
	{

		spin_lock_irqsave(&LB_list->LB_list_lock, flags);

		//check the LogicalBlock_MetaData existence.
		list_for_each_entry(tmp, &LB_list->LB_list_head, list_node) {

			if( tmp->LBID == lbmd->LBID ) {

				match = tmp;

				break;
			}
		}

		//not in the list
		if( likely(match == NULL) ){

			//lock lbmd object
			spin_lock_irqsave(lbmd->LB_lock, lbmd_flags);

			list_add_tail( &(lbmd->list_node), &LB_list->LB_list_head);

			spin_unlock_irqrestore(lbmd->LB_lock, lbmd_flags);

			retcode = DMS_OK;

		}else{

			DMS_PRINTK(LB_DBG, LB_MOD, "duplicate LBMD ptr! LBMD = %p in LB_list, target LBMD = %p",
								duplicate, lbmd);
			DMS_PRINTK(LB_DBG, LB_MOD, "LBID = %llu in LB_list, target LBID = %llu \n",
								duplicate->LBID, lbmd->LBID);

#ifdef	LB_DBG_FLAG
			WARN_ON(duplicate);
#endif

		}

		spin_unlock_irqrestore(&LB_list->LB_list_lock, flags);

	}

	LB_DBG_PRINT(NOCON, KERN_DEBUG "%s%s, end~! LB_MD = %p, retcode = %d\n", lbmd, retcode);

	return retcode;

}
#endif


/*
 * add one LogicalBlock_MetaData to the Logical Block list in order by insertion sort
 */
int Add_to_LogicalBlock_List_by_Insertion_Sort(struct LogicalBlock_List *LB_list, struct LogicalBlock_MetaData *lbmd){

	int retcode = -DMS_FAIL;
	unsigned long flags = 0;
	struct list_head *pos_head = NULL;
	struct LogicalBlock_MetaData *list_lbmd = NULL, *duplicate = NULL;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! LB_MD = %p\n", lbmd);

	if( IS_LEGAL(LB_MOD, LB_list) &&
			IS_LEGAL(LB_MOD, lbmd) )
	{

		write_lock_irqsave(&LB_list->LB_list_lock, flags);

		//insertion sort. pos_head init to the tail at start.
		//pos_head stops at LB_list->LB_list_head eventually.
		list_for_each_prev(pos_head, &LB_list->LB_list_head){

			list_lbmd = list_entry(pos_head, struct LogicalBlock_MetaData, list_node);

			//check the LogicalBlock_MetaData existence, if duplicate, that's an error.
			if( list_lbmd->LBID == lbmd->LBID ) {

				duplicate = list_lbmd;

				break;

			}else if(list_lbmd->LBID < lbmd->LBID){

				//this position (pos_head) can insert the LBMD
				break;
			}

		}

		//not in the list
		if( likely(duplicate == NULL) ){

			//lock lbmd object
			write_lock(&lbmd->LB_lock);

			list_add( &(lbmd->list_node), pos_head);

			write_unlock(&lbmd->LB_lock);

			retcode = DMS_OK;

		}else{

			DMS_PRINTK(LB_DBG, LB_MOD, "duplicate LBMD ptr! LBMD = %p in LB_list, target LBMD = %p",
								duplicate, lbmd);
			DMS_PRINTK(LB_DBG, LB_MOD, "LBID = %llu in LB_list, target LBID = %llu \n",
								duplicate->LBID, lbmd->LBID);

			WARN_ON(duplicate);

		}

		write_unlock_irqrestore(&LB_list->LB_list_lock, flags);

#ifdef NECESSARY
		if(LB_DBG){
			Print_LogicalBlock_List(LB_list);
		}
#endif

	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! LB_MD = %p, retcode = %d\n", lbmd, retcode);

	return retcode;

}


/*
 * Retrieve and remove one LogicalBlock_MetaData from Logical Block list by LBMD ptr.
 * this API will check the existence of the LBMD first.
 * return NULL, if the LBMD is not exist. otherwise, return the LBMD ptr.
 *
 * empty is additional return code:
 * 			1, remove success, and list is empty
 * 			0, remove success, but list is not empty
 * 			-1, if fail (original value of empty, you should set -1 when you declare empty.)
 */
struct LogicalBlock_MetaData * Remove_from_LogicalBlock_List_by_LBMD(struct LogicalBlock_List *LB_list, struct LogicalBlock_MetaData *lbmd, int *empty){

	int retcode = -DMS_FAIL;
	unsigned long flags = 0;
	struct LogicalBlock_MetaData *tmp = NULL, *match = NULL;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! LBMD = %p\n", lbmd);

	if( IS_LEGAL(LB_MOD, LB_list) &&
			IS_LEGAL(LB_MOD, lbmd) )
	{

		write_lock_irqsave(&LB_list->LB_list_lock, flags);

		//check the LogicalBlock_MetaData existence.
		list_for_each_entry(tmp, &LB_list->LB_list_head, list_node) {

			if( tmp == lbmd ) {

				match = tmp;

				break;
			}
		}

		//in the list
		if( likely(match != NULL) ){

			//lock lbmd object
			write_lock(&lbmd->LB_lock);

			list_del( &(lbmd->list_node));

			if(empty != NULL){
				*empty = list_empty(&LB_list->LB_list_head);
			}

			write_unlock(&lbmd->LB_lock);

			retcode = DMS_OK;

		}else{

			DMS_PRINTK(LB_DBG, LB_MOD, "remove a non-exist LogicalBlock_MetaData! LBMD ptr = %p \n", lbmd);

#ifdef	LB_DBG_FLAG
			WARN_ON(match);
#endif

		}

		write_unlock_irqrestore(&LB_list->LB_list_lock, flags);

	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! LBMD = %p, retcode = %d\n", lbmd, retcode);

	return match;

}


/*
 * Retrieve and remove one LogicalBlock_MetaData from Logical Block list by LBID.
 * this API will check the existence of the LBID first.
 * return NULL, if the LBMD is not exist. otherwise, return the LBMD ptr.
 *
 * empty is additional return code:
 * 			1, remove success, and list is empty
 * 			0, remove success, but list is not empty
 * 			-1, if fail (original value of empty, you should set -1 when you declare empty.)
 */
struct LogicalBlock_MetaData * Remove_from_LogicalBlock_List_by_LBID(struct LogicalBlock_List *LB_list, ulong64 lbid, int *empty){

	int retcode = -DMS_FAIL;
	unsigned long flags = 0;
	struct LogicalBlock_MetaData *tmp = NULL, *match = NULL;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! LBID = %llu\n", lbid);

	if( IS_LEGAL(LB_MOD, LB_list) )
	{

		write_lock_irqsave(&LB_list->LB_list_lock, flags);

		//check the LogicalBlock_MetaData existence.
		list_for_each_entry(tmp, &LB_list->LB_list_head, list_node) {

			if( tmp->LBID == lbid ) {

				match = tmp;

				break;
			}
		}

		//in the list
		if( likely(match != NULL) ){

			//lock lbmd object
			write_lock(&match->LB_lock);

			list_del( &(match->list_node));

			if(empty != NULL){
				*empty = list_empty(&LB_list->LB_list_head);
			}

			write_unlock(&match->LB_lock);

			retcode = DMS_OK;

		}else{

			DMS_PRINTK(LB_DBG, LB_MOD, "remove a non-exist LogicalBlock_MetaData! LBID = %llu \n", lbid);

#ifdef	LB_DBG_FLAG
			WARN_ON(match);
#endif

		}

		write_unlock_irqrestore(&LB_list->LB_list_lock, flags);

	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! LBID = %llu, retcode = %d\n", lbid, retcode);

	return match;

}


/*
 * remove the LBMD from LB_List and free it by LBMD
 *
 * @return 	1, remove success, and list is empty
 * 			0, remove success, but list is not empty
 * 			-1, if fail
 */
int Free_LogicalBlock_MetaData_from_LB_List_by_LBMD(struct LogicalBlock_List *LB_list, struct LogicalBlock_MetaData *lbmd){

	int empty = -DMS_FAIL;
	int retcode = -DMS_FAIL;

	if( IS_LEGAL(LB_MOD, LB_list) &&
			IS_LEGAL(LB_MOD, lbmd) )
	{
		//remove from LB_List
		struct LogicalBlock_MetaData *lbmd = Remove_from_LogicalBlock_List_by_LBMD(LB_list, lbmd, &empty);

		if( IS_LEGAL(LB_MOD, lbmd) ){

			//free the LBMD
			Release_LogicalBlock_MetaData(lbmd);

			retcode = empty;

		}else{

			//do nothing, it means this lbmd has been gone.
		}

	}

	return retcode;
}


/*
 * remove the LBMD from LB_List and free it by LBID
 *
 * @return 	1, success, and list is empty
 * 			0, if success, but list is not empty
 * 			-1, if fail
 */
int Free_LogicalBlock_MetaData_from_LB_List_by_LBID(struct LogicalBlock_List *LB_list, ulong64 lbid){

	int empty = -DMS_FAIL;
	int retcode = -DMS_FAIL;

	if( IS_LEGAL(LB_MOD, LB_list) )
	{
		//remove from LB_List
		struct LogicalBlock_MetaData *lbmd = Remove_from_LogicalBlock_List_by_LBID(LB_list, lbid, &empty);

		if( IS_LEGAL(LB_MOD, lbmd) ){

			//free the LBMD
			Release_LogicalBlock_MetaData(lbmd);

			retcode = empty;

		}else{

			//do nothing, it means this lbmd has been gone.
		}

	}

	return retcode;
}

int Commit_LB_List_by_Range(struct LogicalBlock_List *LB_list, ulong64 slbid, int nr_lbids){

	int empty = -DMS_FAIL;

	unsigned long flags = 0;

	if( IS_LEGAL(LB_MOD, LB_list) )
	{
		int i = 0;
		struct LogicalBlock_MetaData *cur = NULL, *next = NULL, *match = NULL;

		write_lock_irqsave(&LB_list->LB_list_lock, flags);

		//check the LogicalBlock_MetaData existence.
		list_for_each_entry_safe(cur, next, &LB_list->LB_list_head, list_node) {

			if( cur->LBID == slbid+i ) {

				//lock lbmd object
				write_lock(&cur->LB_lock);

				list_del( &(cur->list_node));

				write_unlock(&cur->LB_lock);

				//free it
				Release_LogicalBlock_MetaData(cur);

				//go forward after commit.
				if( ++i == nr_lbids){
					break;
				}
			}
		}

		empty = list_empty(&LB_list->LB_list_head);

		write_unlock_irqrestore(&LB_list->LB_list_lock, flags);

		//error. if i isn't match nr_lbids
		if( i != nr_lbids){
			DSTR_PRINT(ALWAYS, LB_MOD, SPrint_Simple_LogicalBlock_List(DSTR_NAME, DSTR_LIMIT, LB_list),
					"totals = %d isn't match nr_lbids = %d while commit LB_List, ", i, nr_lbids);
		}

	}

	return empty;
}

/**
 * Get_Next_LBMD - iterative go through the list and return the next element you want.
 * @param lb_list		:your lb_list
 * @return	struct LogicalBlock_MetaData * 	:lbmd pointer
 * 			NULL 							:end of the list.
 *
 * Description:
 * 		get next lbmd from lb_list by iterator,
 * 		This API returns the pointer of lbmd only. The lbmd won't be remove and release from lb_list.
 *
 */
struct LogicalBlock_MetaData * Get_Next_LBMD(struct LogicalBlock_List *lb_list){

//	unsigned long flags = 0;
	struct LogicalBlock_MetaData *cur_node = NULL, *match_lbmd = NULL;

	if( IS_LEGAL(LB_MOD, lb_list) ){

		read_lock(&lb_list->LB_list_lock);

		cur_node = (lb_list->next_node);

		//if I am not end in head
		if( (&cur_node->list_node) != (&lb_list->LB_list_head) ){

			//get next
			match_lbmd = cur_node;

			//go forward
			lb_list->next_node = list_entry( (cur_node->list_node.next), struct LogicalBlock_MetaData, list_node );

		}

		read_unlock(&lb_list->LB_list_lock);

	}

	DMS_PRINTK(DLIST_DBG, LB_MOD, "match_lbmd = %p, lb_list = %p done! \n",
			match_lbmd, lb_list);


	return match_lbmd;
}


/**
 * Reset_DList_Iterator
 *
 * @param dlist
 * @return
 *
 * Description:
 * 		get next user data from DList by iterator,
 * 		This API returns the pointer of user_data only. The DList_Node won't be remove and release from DList.
 *
 */
int Reset_LB_List_Iterator(struct LogicalBlock_List *lb_list){

	int retcode = -DMS_FAIL;
//	unsigned long flags = 0;

	if(IS_LEGAL(LB_MOD, lb_list)){

		read_lock(&lb_list->LB_list_lock);

		//even the next point to head itself is OK, because the data structure has common order.
		lb_list->next_node = list_entry( (lb_list->LB_list_head.next), struct LogicalBlock_MetaData, list_node );

		read_unlock(&lb_list->LB_list_lock);

		retcode = DMS_OK;
	}

	return retcode;

}



struct LogicalBlock_MetaData * Get_LBMD_from_Node_by_LBID(struct LogicalBlock_List *lb_list, struct LogicalBlock_MetaData *cur, ulong64 lbid){

//	unsigned long flags = 0;
	struct LogicalBlock_MetaData *next = NULL, *match_lbmd = NULL;

	if( IS_LEGAL(LB_MOD, lb_list) ){

		//go back to first one
		if(cur == NULL)
			cur = list_entry( (lb_list->LB_list_head.next), struct LogicalBlock_MetaData, list_node );

		read_lock(&lb_list->LB_list_lock);

		list_for_each_entry_safe_from(cur, next, &lb_list->LB_list_head, list_node){

			if(cur->LBID == lbid){
				match_lbmd = cur;
				break;
			}
		}

		read_unlock(&lb_list->LB_list_lock);

	}

	DMS_PRINTK(DLIST_DBG, LB_MOD, "match_lbmd = %p, lb_list = %p done! \n",
			match_lbmd, lb_list);


	return match_lbmd;
}


#if 0
/*
 * walk through the LB_List and initialize all the FSM of LBMD
 */
int Init_All_FSM_in_List(struct LogicalBlock_List *LB_list/*, struct Full_Dn_Req *fdn*/){

	int retcode = DMS_OK;
	struct LogicalBlock_MetaData * tmp = NULL;
	struct LogicalBlock_MetaData * match = NULL;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! LB_list = %p\n", LB_list);

	if( IS_LEGAL(LB_MOD, LB_list) )
	{

		write_lock(&LB_list->LB_list_lock);

		list_for_each_entry(tmp, &LB_list->LB_list_head, list_node) {

				DMS_PRINTK(LB_DBG, LB_MOD, "init FSM in LogicalBlock_MetaData = %p, LBID = %llu \n",
							   tmp, tmp->LBID);

				tmp->fsm.fdn = fdn;

				//init the fsm of LBMD
				//initState( &tmp->fsm );

		}

		write_unlock(&LB_list->LB_list_lock);

	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! LB_list = %p\n", LB_list);

	return retcode;

}
#endif


#if 0
extern int digit2NumOfOne[];
/*
 * calculate the pay-load position offset of corresponding LBMD, the LB_list has to be a ordered list.
 */
int Get_Payload_Index(struct LogicalBlock_List *LB_list, ulong64 start_LBID, ulong64 end_LBID){

	//TODO we have changed the hwsec_size to 4KB, so we have to do corresponding changes.

	int i = 0, index = 0, sector_offset = 0;
	unsigned long flags = 0;
	struct LogicalBlock_MetaData *cur = NULL, *match = NULL;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! start_LBID = %llu, end_LBID = %llu\n", start_LBID, end_LBID);

	if( IS_LEGAL(LB_MOD, LB_list) )
	{

		spin_lock_irqsave(&LB_list->LB_list_lock, flags);

		list_for_each_entry(cur, &LB_list->LB_list_head, list_node) {

			if( cur->LBID == start_LBID + index++ ) {

				sector_offset += digit2NumOfOne[cur->LB_mask & 0xf] +
									digit2NumOfOne[((cur->LB_mask & 0xf0) >> 4)];

			}else if(cur->LBID == end_LBID){

				//the terminal condition, doesn't include end_LBID itself.
				break;

			}else{

				eprintk(LB_MOD, "the startLBID = %llu list doesn't consecutive!! \n", start_LBID);
				Print_LogicalBlock_List(LB_list);
			}
		}

		spin_unlock_irqrestore(&LB_list->LB_list_lock, flags);

	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! target sector_offset * KERNEL_SECTOR_SIZE = %d\n", sector_offset * KERNEL_SECTOR_SIZE);

	return sector_offset * KERNEL_SECTOR_SIZE;


	return 0;
}
#endif


int SPrint_Simple_LogicalBlock_List(char *buf, int len_limit, struct LogicalBlock_List *LB_list){

	int len = 0;

	unsigned long flags = 0;
	struct LogicalBlock_MetaData *cur = NULL;

	if( IS_LEGAL(LB_MOD, LB_list) &&
			IS_LEGAL(LB_MOD, buf) )
	{

		if(len_limit > 0){

			read_lock_irqsave(&LB_list->LB_list_lock, flags);

			len += sprintf(buf+len, "\tLBIDs = [");

			list_for_each_entry(cur, &LB_list->LB_list_head, list_node) {

				len += sprintf(buf+len, "%llu, ", cur->LBID);
			}

			len += sprintf(buf+len, "]\n");

			read_unlock_irqrestore(&LB_list->LB_list_lock, flags);

		}

	}

	return len;

}

/*
 * print the LBIDs array in the dn_request
 */
void Print_Simple_LogicalBlock_List(char *prestr, struct LogicalBlock_List *LB_list){


	unsigned long flags = 0;
	struct LogicalBlock_MetaData *cur = NULL;

	if( IS_LEGAL(LB_MOD, LB_list) )	{

		read_lock_irqsave(&LB_list->LB_list_lock, flags);

		DMS_PRINTK(ALWAYS, LB_MOD, "%s LBIDs = [", prestr);

		list_for_each_entry(cur, &LB_list->LB_list_head, list_node) {

			printk("%llu, ", cur->LBID);
		}

		printk("]\n");

		read_unlock_irqrestore(&LB_list->LB_list_lock, flags);

	}

}

/*
 * print LBID, status and mask array of the LB_List
 */
void Print_LogicalBlock_List(struct LogicalBlock_List *LB_list){

	unsigned long flags = 0;
	struct LogicalBlock_MetaData *cur = NULL;

	DMS_PRINTK(LB_DBG, LB_MOD, "start~! \n");

	if( IS_LEGAL(LB_MOD, LB_list) )
	{

		read_lock_irqsave(&LB_list->LB_list_lock, flags);

		list_for_each_entry(cur, &LB_list->LB_list_head, list_node) {

			DMS_PRINTK(ALWAYS, LB_MOD, "[LBID = %llu, bio = %p, vec_idx = %u]\n",
										   cur->LBID, cur->bio, cur->vec_idx);
		}

		read_unlock_irqrestore(&LB_list->LB_list_lock, flags);

	}

	DMS_PRINTK(LB_DBG, LB_MOD, "end~! \n");


}


//***************************************************** LB_list end ****************************************************
