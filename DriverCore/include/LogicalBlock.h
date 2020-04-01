/*
 * LogicalBlock.h
 *
 *  Created on: 2011/5/1
 *      Author: Vis Lee
 */

#ifndef LOGICALBLOCK_H_
#define LOGICALBLOCK_H_

#include "DMS_type.h"
#include "DMS_Common.h"
//#include "DatanodeFiniteStateMachine.h"


/********************************************************************************/
/*																				*/
/*									Global variables							*/
/*																				*/
/********************************************************************************/

/*
 * dms vdd needs to random access the bio data structure and get the corresponding
 * page for committing IO w/r.
 */
//return struct page	*
#define bio_page_idx(bio, idx)			bio_iovec_idx((bio), idx)->bv_page
#define bio_offset_idx(bio, idx)		bio_iovec_idx((bio), idx)->bv_offset
#define bio_bvlen_idx(bio, idx)			bio_iovec_idx((bio), idx)->bv_len



/********************************************************************************/
/*																				*/
/*									data structures								*/
/*																				*/
/********************************************************************************/
/*
 * the data pay-load location on a data node
 */
//struct Physical_Location{
//
//	//the data node ip and port
//	char hostname[DMS_IP_STR_LEN];
//
//	//the data node port
//	int port;
//
//	//the combination of RBID, Length and offset;
//	ulong64 RBID_len_offset;
//
//};

/*
 * Logical Block meta data
 */
struct LogicalBlock_MetaData{

	//list head, DON'T CHANGE THE ORDER of THIS MEMBER
	struct list_head list_node;

	//lock pointer
	rwlock_t LB_lock;

	//Logical Block ID
	ulong64 LBID;

	//HBlock ID
	ulong64 HBID;

	//sector mask for this LB
//	char *LB_mask;

	//finite state machine for this LB
	//struct DN_Req_FSM fsm;

	//LB status
//	int status;

	//offset index
//	int payload_offset;

	//read/write pay-load pointer
	//char *payload;

	//physical locations
//	struct Physical_Location *locations;


	//TODO bio->bio_vect[i] is a page unit.
	struct bio	*bio;
	unsigned short vec_idx;


};

struct LogicalBlock_List{

	//list head of this LB_List
	struct list_head LB_list_head;

	//lock for the LB_List
	rwlock_t LB_list_lock;

	//always point to next node
	struct LogicalBlock_MetaData *next_node;


};



/********************************************************************************/
/*									Exported APIs								*/
/********************************************************************************/

/*
 * create and initialize LBMD and return to user
 * !!Remember to get lock if you want to use the LBMD!!
 */
struct LogicalBlock_MetaData * Create_LogicalBlock_MetaData(void);

/*
 * initialize related data structure
 */
void Init_LogicalBlock_MetaData(struct LogicalBlock_MetaData *lbmd);

/*
 * free memory of struct LogicalBlock_MetaData
 */
void Release_LogicalBlock_MetaData(struct LogicalBlock_MetaData *lbmd);


//******************************** LB_list APIs *********************************
/*
 * initialize list_head and lock of the data structure
 */
int Init_LogicalBlock_List(struct LogicalBlock_List *LB_list);

/*
 * walk through the LogicalBlock_List and free all elements.
 */
void Release_LogicalBlock_List(struct LogicalBlock_List *LB_list);

struct LogicalBlock_List * Create_LogicalBlock_List(void);

/*
 * check whether LB_List is empty or not.
 * return true, if list is empty, otherwise false.
 */
int Is_LB_List_Empty(struct LogicalBlock_List *lb_list);

/*
 * get corresponding LB_MetaData from Logical Block list by LBMD ptr,
 * This API returns the pointer of LogicalBlock_MetaData only. The LogicalBlock_MetaData won't be deleted from LB_List.
 */
struct LogicalBlock_MetaData * Get_LogicalBlock_By_LBMD_ptr(struct LogicalBlock_List *LB_list, struct LogicalBlock_MetaData *lbmd);

/*
 * get corresponding LogicalBlock_MetaData from Logical Block list by LBID,
 * This API returns the pointer of LogicalBlock_MetaData only. The LogicalBlock_MetaData won't be deleted from LB_List.
 */
struct LogicalBlock_MetaData * Get_LogicalBlock_By_LBID(struct LogicalBlock_List *LB_list, ulong64 lbid);

#if 0
/*
 * add one LogicalBlock_MetaData to the Logical Block list
 */
int Add_to_LogicalBlock_List(struct LogicalBlock_List *LB_list, struct LogicalBlock_MetaData * lbmd);
#endif

/*
 * add one LogicalBlock_MetaData to the Logical Block list in order by insertion sort
 */
int Add_to_LogicalBlock_List_by_Insertion_Sort(struct LogicalBlock_List *LB_list, struct LogicalBlock_MetaData *lbmd);

/*
 * Retrieve and remove one LogicalBlock_MetaData from Logical Block list by LBMD ptr.
 * return NULL, if the LBMD is not exist. otherwise, return the LBMD ptr.
 */
struct LogicalBlock_MetaData * Remove_from_LogicalBlock_List_by_LBMD(struct LogicalBlock_List *LB_list, struct LogicalBlock_MetaData * lbmd, int *empty);

/*
 * Retrieve and remove one LogicalBlock_MetaData from Logical Block list by LBID.
 * return NULL, if the LBMD is not exist. otherwise, return the LBMD ptr.
 */
struct LogicalBlock_MetaData * Remove_from_LogicalBlock_List_by_LBID(struct LogicalBlock_List *LB_list, ulong64 lbid, int *empty);


/*
 * remove the LBMD from LB_List and free it by LBMD
 */
int Free_LogicalBlock_MetaData_from_LB_List_by_LBMD(struct LogicalBlock_List *LB_list, struct LogicalBlock_MetaData * lbmd);

/*
 * remove the LBMD from LB_List and free it by LBID
 */
int Free_LogicalBlock_MetaData_from_LB_List_by_LBID(struct LogicalBlock_List *LB_list, ulong64 lbid);

/*
 * walk through the LB_List and initialize all the FSM of LBMD
 */
//int Init_All_FSM_in_List(struct LogicalBlock_List *LB_list, struct Full_Dn_Req *fdn);

/*
 * calculate the pay-load position offset of corresponding LBMD, the LB_list has to be a ordered list.
 */
//int Get_Payload_Index(struct LogicalBlock_List *LB_list, ulong64 start_LBID, ulong64 end_LBID);

int SPrint_Simple_LogicalBlock_List(char *buf, int len_limit, struct LogicalBlock_List *LB_list);
void Print_Simple_LogicalBlock_List(char *prestr, struct LogicalBlock_List *LB_list);

/*
 * print LBID, status and mask array of the LB_List
 */
void Print_LogicalBlock_List(struct LogicalBlock_List *LB_list);



int Reset_LB_List_Iterator(struct LogicalBlock_List *lb_list);
struct LogicalBlock_MetaData * Get_Next_LBMD(struct LogicalBlock_List *lb_list);

int Commit_LB_List_by_Range(struct LogicalBlock_List *LB_list, ulong64 slbid, int nr_lbids);

#endif /* LOGICALBLOCKMETADATA_H_ */
