/*
 * DList.c
 *
 *  Created on: Mar 8, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include "DMS_Common.h"
#include "DList.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *DLIST_MOD = "DList: ";



/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/
#define DL_ADD_HEAD			0
#define DL_ADD_TAIL			1




/********************************************************************************/
/*																				*/
/*								DList_Node APIs									*/
/*																				*/
/********************************************************************************/

inline int __Init_DLNode(struct DList_Node *dln, void *user_data){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(DLIST_MOD, dln) )
	{
		INIT_LIST_HEAD(&dln->dlist_node);

		spin_lock_init(&dln->dlnlock);

		dln->user_data = user_data;

		retcode = DMS_OK;

	}

	return retcode;

}


inline struct DList_Node * __Create_DLNode(void *user_data){

	int retcode = DMS_OK;

	struct DList_Node *dln = NULL;

	dln = Malloc_DLN(GFP_KERNEL);

	retcode = __Init_DLNode(dln, user_data);

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "create DLN = %p done! init retcode = %d \n",
						dln, retcode);

	return dln;

}

inline void __Release_DLNode(struct DList_Node *dln){

	Free_DLN(dln);

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "release DLN = %p done! \n", dln);

}


/********************************************************************************/
/*																				*/
/*									DList APIs									*/
/*																				*/
/********************************************************************************/
/**
 * Is_DList_Empty - check whether DList is empty or not.
 *
 * @param dlist
 * @return true - list is empty, otherwise false.
 *
 * Description:
 */
int Is_DList_Empty(struct DList *dlist){

	int is_empty = false;
	unsigned long flags = 0;

	if( IS_LEGAL(DLIST_MOD, dlist) ){

		spin_lock_irqsave(&dlist->dlist_lock, flags);
		is_empty = list_empty(&dlist->dlist_head);
		spin_unlock_irqrestore(&dlist->dlist_lock, flags);
	}

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "list_empty = %s \n",
				is_empty?"true":"false");


	return is_empty;
}


/** TODO performance seems no good, improve it.
 *
 * Get_Next_User_Data - iterative go through the list and return the next element you want.
 * @param dlist		:your dlist
 * @return void * 	:user_data pointer or NULL if not found.
 *
 * Description:
 * 		get next user data from DList by iterator,
 * 		This API returns the pointer of user_data only. The DList_Node won't be remove and release from DList.
 *
 */
void * Get_Next_User_Data(struct DList *dlist){

	unsigned long flags = 0;
	struct DList_Node *cur_node = NULL;

	void * match_ud = NULL;

	if( IS_LEGAL(DLIST_MOD, dlist) ){

		spin_lock_irqsave(&dlist->dlist_lock, flags);

		cur_node = (dlist->next_node);

		//if I am not end in head
		if( (&cur_node->dlist_node) != (&dlist->dlist_head) ){

			//get next
			match_ud = (void *)( cur_node->user_data );

			//go forward
			dlist->next_node = list_entry( (cur_node->dlist_node.next), struct DList_Node, dlist_node );

		}

		spin_unlock_irqrestore(&dlist->dlist_lock, flags);

	}

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "match_ud = %p, dlist = %p done! \n",
			match_ud, dlist);


#if 0 //def DMS_DEBUG //this is normal case

	WARN_ON( (match_ud == NULL) );

#endif


	return match_ud;
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
int Reset_DList_Iterator(struct DList *dlist){

	int retcode = -DMS_FAIL;
	unsigned long flags = 0;

	if(IS_LEGAL(DLIST_MOD, dlist)){

		spin_lock_irqsave(&dlist->dlist_lock, flags);

		//even the next point to head itself is OK, because the data structure has common order.
		dlist->next_node = list_entry( (dlist->dlist_head.next), struct DList_Node, dlist_node );

		spin_unlock_irqrestore(&dlist->dlist_lock, flags);

		retcode = DMS_OK;
	}

	return retcode;

}

/**
 * Get_User_Data_By_Check_Func
 * @param dlist
 * @param checkfn - user specified check function
 * @return void * - user_data
 *
 * Description:
 * 		get corresponding user data from DList by user specified check function,
 * 		This API returns the pointer of user_data only. The DList_Node won't be remove and release from DList.
 *
 * N.B. - don't sleep at check function!!
 */
void * Get_User_Data_By_Check_Func(struct DList *dlist, DL_Check_Fn checkfn, void *args){

	unsigned long flags = 0;
	struct DList_Node *tmp = NULL;

	void * match_ud = NULL;

	if(IS_LEGAL(DLIST_MOD, dlist) &&
			IS_LEGAL(DLIST_MOD, checkfn)){

		spin_lock_irqsave(&dlist->dlist_lock, flags);

		list_for_each_entry(tmp, &dlist->dlist_head, dlist_node){

			//call check function, if return true, remove it from list.
			if( checkfn((void *)(tmp->user_data), args) ){

				match_ud = (void *)(tmp->user_data);
				break;
			}

		}

		spin_unlock_irqrestore(&dlist->dlist_lock, flags);

	}

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "match_ud = %p, dlist = %p done! \n",
			match_ud, dlist);

#if (DMS_DEBUG == 1)

	WARN_ON( (match_ud == NULL) );

#endif


	return match_ud;
}


/**
 * __Check_Duplicate - by user_data
 *
 * @param dlist
 * @param user_data
 * @return match - duplicated dln ptr
 *
 * DESCRIPTION:
 * 		go thru whole list to check whether there is duplicated item or not.
 *
 * N.B. you have to acquire list_lock before you call into this func.
 */
#if (DMS_DEBUG == 1)
inline struct DList_Node * __Check_Duplicate(struct DList *dlist, void *user_data){

	struct DList_Node *tmp = NULL, *match = NULL;

	//if enable debug flag
	if(DLIST_DBG && IS_LEGAL(DLIST_MOD, dlist) ){

		//check the user_data existence.
		list_for_each_entry(tmp, &dlist->dlist_head, dlist_node) {

			if( tmp->user_data == user_data ) {

				match = tmp;

				break;
			}
		}

		//you got it!
		if(match != NULL){

			DMS_PRINTK(DLIST_DBG, DLIST_MOD, "you got a duplicate add!! user_data = %p, match_ud = %p, dlist = %p \n",
						user_data, match->user_data, dlist);

		}

		WARN_ON(match != NULL);

	}

	return match;

}
#endif


/**
 * Insert_User_Data_to_DList
 *
 * @param dlist
 * @param user_data
 * @return	0 - insert ok
 *
 * Description:
 * 		create a DList_Node with user data and add to the DList.
 * 		why we don't return DList_Node pointer to user and provide a delete list node function?
 * 		because, we can't prevent a user delete one node twice, that's pretty dangerous.
 * 		if we want, we have to scan whole list, it's identical to our provided API: "remove_by_?"
 */
int __Insert_User_Data_to_DList(struct DList *dlist, void *user_data, int head_or_tail){

	int retcode = -DMS_FAIL;
	unsigned long flags = 0;
	struct DList_Node *match = NULL, *new_node = NULL;

	if( IS_LEGAL(DLIST_MOD, dlist) )
	{

		new_node = __Create_DLNode(user_data);

		spin_lock_irqsave(&dlist->dlist_lock, flags);

#if (DMS_DEBUG == 1)
		//debug, check duplicate
		match = __Check_Duplicate(dlist, user_data);
#endif

		//not in the list. match will take effect when DLIST_DBG enabled.
		if( likely(match == NULL) ){

			switch(head_or_tail){

				case DL_ADD_HEAD:
					list_add( &(new_node->dlist_node), &(dlist->dlist_head) );
					retcode = DMS_OK;
					break;

				case DL_ADD_TAIL:
					list_add_tail( &(new_node->dlist_node), &(dlist->dlist_head) );
					retcode = DMS_OK;
					break;

				default:
					wprintk(DLIST_MOD, "unknow DList opcode = %d ", head_or_tail);
			}

		}else{

			__Release_DLNode(new_node);

#if (DMS_DEBUG == 1)
#ifdef DMS_UTEST
			WARN_ON( DLIST_DBG );
#else
			BUG_ON( DLIST_DBG );
#endif
#endif

		}

		spin_unlock_irqrestore(&dlist->dlist_lock, flags);

	}

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "user_data = %p, dlist = %p, retcode = %d done! \n",
				user_data, dlist, retcode);

	return retcode;

}


/**
 * Insert_User_Data_to_DList_Head
 *
 * @param dlist
 * @param user_data
 * @return	0 - insert ok
 *
 * Description:
 * 		create a DList_Node with user data and add to the DList head.
 */
int Insert_User_Data_to_DList_Head(struct DList *dlist, void *user_data){

	return __Insert_User_Data_to_DList(dlist, user_data, DL_ADD_HEAD);

}



/**
 * Insert_User_Data_to_DList_Tail
 *
 * @param dlist
 * @param user_data
 * @return	0 - insert ok
 *
 * Description:
 * 		create a DList_Node with user data and add to the DList tail.
 */
int Insert_User_Data_to_DList_Tail(struct DList *dlist, void *user_data){

	return __Insert_User_Data_to_DList(dlist, user_data, DL_ADD_TAIL);

}

//TODO we can provide some function like "insert by user-specified function" if necessary.



inline void __Remove_DLNode_from_DList(struct DList_Node *dln){

	list_del_init( &dln->dlist_node );

	//free dln
	__Release_DLNode(dln);
}


/**
 * Remove_from_DList_by_User_Data
 *
 * @param dlist
 * @param user_data
 * @return	void * - user_data
 *
 * Description:
 * 		Retrieve and remove one DList_Node from DList by user_data ptr.
 * 		return NULL, if the user_data is not exist. otherwise, return the user_data ptr.
 * 		in addition, if you want to check whether the list is empty or not after remove,
 * 		you can pass an integer pointer into this function.
 */
void * Remove_from_DList_by_User_Data(struct DList *dlist, void *user_data, int *empty){

	unsigned long flags = 0;
	struct DList_Node *cur = NULL, *next = NULL;

	void *match_ud = NULL;


	if( IS_LEGAL(DLIST_MOD, dlist) ){

		spin_lock_irqsave(&dlist->dlist_lock, flags);

		list_for_each_entry_safe(cur, next, &dlist->dlist_head, dlist_node){

			//call check function, if return true, remove it from list.
			if( cur->user_data == user_data ){

				match_ud = cur->user_data;

				//check DList next_node, prevent user get released node.
				if(dlist->next_node == cur){
					dlist->next_node = next;
				}

				__Remove_DLNode_from_DList(cur);

				break;
			}

		}

		//check list empty
		if(empty != NULL){
			*empty = list_empty(&dlist->dlist_head);
		}

		//check DList next_node, prevent user get released node.
		if(dlist->next_node == cur){
			dlist->next_node = next;
		}

		spin_unlock_irqrestore(&dlist->dlist_lock, flags);

	}

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "user_data = %p, match_ud = %p, dlist = %p, empty = %d done! \n",
			user_data, match_ud, dlist, (empty==NULL)?0:*empty);

#if (DMS_DEBUG == 1)

	//I think we can accept remove a non-exist user_data
	WARN_ON( (match_ud == NULL) );

#endif


	return match_ud;

}


/**
 * Remove_from_DList_by_Check_Func
 *
 * @param dlist
 * @param checkfn
 * @return	void * - user_data
 *
 * Description:
 * 		Retrieve and remove one DList_Node from DList by checkfn.
 * 		return NULL, if checkfn find nothing. otherwise, return the user_data ptr.
 */
void * Remove_from_DList_by_Check_Func(struct DList *dlist, DL_Check_Fn checkfn, void *args, int *empty){

	unsigned long flags = 0;
	struct DList_Node *cur = NULL, *next = NULL;

	void * match_ud = NULL;

	if(IS_LEGAL(DLIST_MOD, dlist) &&
			IS_LEGAL(DLIST_MOD, checkfn)){

		spin_lock_irqsave(&dlist->dlist_lock, flags);

		list_for_each_entry_safe(cur, next, &dlist->dlist_head, dlist_node){

			//call check function, if return true, remove it from list.
			if( checkfn((void *)(cur->user_data), args) ){

				match_ud = (void *)(cur->user_data);

				//check DList next_node, prevent user get released node.
				if(dlist->next_node == cur){
					dlist->next_node = next;
				}

				__Remove_DLNode_from_DList(cur);
				break;
			}

		}

		//check list empty
		if(empty != NULL){
			*empty = list_empty(&dlist->dlist_head);
		}

		spin_unlock_irqrestore(&dlist->dlist_lock, flags);

	}

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "match_ud = %p, dlist = %p, empty = %d done! \n",
			match_ud, dlist, (empty==NULL)?0:*empty);

#if (DMS_DEBUG == 1)

	WARN_ON( (match_ud == NULL) );

#endif


	return match_ud;

}


/**
 * Remove_from_DList_Head
 *
 * @param dlist
 * @param empty
 * @return	void *	- user_data
 * 			NULL	- not found
 *
 * Description:
 * 		Retrieve and remove one DList_Node from DList head.
 * 		return NULL, if find nothing. otherwise, return the user_data ptr.
 */
void * Remove_from_DList_Head(struct DList *dlist, int *empty){

	unsigned long flags = 0;
	struct DList_Node *cur = NULL, *next = NULL;

	void * match_ud = NULL;

	if(IS_LEGAL(DLIST_MOD, dlist)){

		spin_lock_irqsave(&dlist->dlist_lock, flags);

		list_for_each_entry_safe(cur, next, &dlist->dlist_head, dlist_node){

			//get first one and out
			match_ud = cur->user_data;

			//check DList next_node, prevent user get released node.
			if(dlist->next_node == cur){
				dlist->next_node = next;
			}

			__Remove_DLNode_from_DList(cur);
			break;

		}

		//check list empty
		if(empty != NULL){
			*empty = list_empty(&dlist->dlist_head);
		}

		spin_unlock_irqrestore(&dlist->dlist_lock, flags);

	}

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "match_ud = %p, dlist = %p, empty = %d done! \n",
			match_ud, dlist, (empty==NULL)?0:*empty);

#if (DMS_DEBUG == 1)

	WARN_ON( (match_ud == NULL) );

#endif


	return match_ud;

}

/**
 * Print_DList_by_UPrint_Func
 *
 * @param dlist
 * @param pfn
 *
 * DESCRIPTION:
 * 		print out user_data by user specified print function.
 */
void Print_DList_by_UPrint_Func(struct DList *dlist, DL_UPrint_Fn pfn){

	unsigned long flags = 0;
	struct DList_Node *cur = NULL;

	if(IS_LEGAL(DLIST_MOD, dlist) &&
			IS_LEGAL(DLIST_MOD, pfn)){

		spin_lock_irqsave(&dlist->dlist_lock, flags);

		list_for_each_entry(cur, &dlist->dlist_head, dlist_node){

			//call user specified print user_data function.
			pfn((void *)(cur->user_data));

		}

		spin_unlock_irqrestore(&dlist->dlist_lock, flags);

	}

}

/**
 * __Init_DList
 *
 * @param dlist
 * @return
 */
int __Init_DList(struct DList *dlist){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(DLIST_MOD, dlist)){

		INIT_LIST_HEAD(&dlist->dlist_head);

		spin_lock_init(&dlist->dlist_lock);

		dlist->next_node = list_entry( (dlist->dlist_head.next), struct DList_Node, dlist_node );

		retcode = DMS_OK;

	}

	return retcode;
}

/**
 * Create_DList
 *
 * @return dlist
 *
 * Description:
 * 		create and init the list_head and lock
 */
struct DList * Create_DList(){

	struct DList *dlist = NULL;

	dlist = Malloc_DList(GFP_KERNEL);

	if( __Init_DList(dlist) < 0 ){

		goto CREATE_DLIST_FAIL;
	}


	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "create dlist = %p done! \n", dlist);

	return dlist;

CREATE_DLIST_FAIL:

	if(CHECK_PTR(DLIST_MOD,dlist)){
		Free_DList(dlist);
	}

	PRINT_DMS_ERR_LOG(EDEV_NOMEM, DLIST_MOD, "");

	return NULL;

}


/**
 * Release_All_User_Data
 *
 * @param dlist
 * @param ud_release  - user data release function. in general, you can pass NULL to here.
 *
 * DESCRIPTION:
 * 		if user wants to release all the remained user data, but keep DList head,
 * 		call this API with a user specified user_data release function.
 */
void Release_All_User_Data(struct DList *dlist, DL_User_Data_Release_Fn ud_release){

	unsigned long flags = 0;
	struct DList_Node *cur = NULL, *next = NULL;

	if(IS_LEGAL(DLIST_MOD, dlist) &&
			IS_LEGAL(DLIST_MOD, ud_release)){

		spin_lock_irqsave(&dlist->dlist_lock, flags);

		list_for_each_entry_safe(cur, next, &dlist->dlist_head, dlist_node){

			DMS_PRINTK(DLIST_DBG, DLIST_MOD, "cur = %p, cur->user_data = %p \n",
					cur, cur->user_data );

			//release user_data
			ud_release(cur->user_data);

			//remove DLN
			__Remove_DLNode_from_DList(cur);

			DMS_PRINTK(DLIST_DBG, DLIST_MOD, "cur = %p call ud_release done! \n", cur);

		}

		spin_unlock_irqrestore(&dlist->dlist_lock, flags);
	}

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "release user_data done! dlist = %p \n", dlist);

}


/**
 * Release_DList
 *
 * @param dlist - the list you want to release
 *
 * Description:
 * 		release DList if empty.
 */
void Release_DList(struct DList *dlist){

	int is_empty = false;
	unsigned long flags = 0;

	if(IS_LEGAL(DLIST_MOD, dlist)){

		spin_lock_irqsave(&dlist->dlist_lock, flags);

		is_empty = list_empty(&dlist->dlist_head);

		spin_unlock_irqrestore(&dlist->dlist_lock, flags);

		if( likely(is_empty) ){

			Free_DList(dlist);

		} else {

			wprintk(DLIST_MOD, "list = %p isn't empty! \n", dlist);
		}

	}

	DMS_PRINTK(DLIST_DBG, DLIST_MOD, "release dlist = %p done!\n", dlist);

}

/**
 * Release_DList_and_User_Data
 *
 * @param dlist
 * @param ud_release  - user data release function. in general, you can pass NULL to here.
 *
 * DESCRIPTION:
 * 		if user knows the list has remained something and wants to release them when release DList,
 * 		call this API with a user specified user_data release function.
 */
void Release_DList_and_User_Data(struct DList *dlist, DL_User_Data_Release_Fn ud_release){

	//release all user data
	Release_All_User_Data(dlist, ud_release);

	//release dlist
	Release_DList(dlist);

}


#ifdef DMS_UTEST
EXPORT_SYMBOL(Is_DList_Empty);
EXPORT_SYMBOL(Get_User_Data_By_Check_Func);
EXPORT_SYMBOL(Insert_User_Data_to_DList_Head);
EXPORT_SYMBOL(Insert_User_Data_to_DList_Tail);
EXPORT_SYMBOL(Remove_from_DList_by_User_Data);
EXPORT_SYMBOL(Remove_from_DList_by_Check_Func);
EXPORT_SYMBOL(Create_DList);
EXPORT_SYMBOL(Release_DList);
EXPORT_SYMBOL(Release_DList_and_User_Data);
EXPORT_SYMBOL(Print_DList_by_UPrint_Func);
#endif
