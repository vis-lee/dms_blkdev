/*
 * DList.h
 *
 *  Created on: Mar 7, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef DLIST_H_
#define DLIST_H_

//#include <linux/list.h>

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/





/********************************************************************************/
/*																				*/
/*								Definitions	 									*/
/*																				*/
/********************************************************************************/

/* !! Attention : DON'T change the order of the DList and DList_Node */

struct DList_Node{

		//list head
		struct list_head dlist_node;

		//lock pointer
		spinlock_t dlnlock;

		void *user_data;

};

/**
 * DList.dlist_head only used to link up the DList_Node.
 * once you want to walk thru a list, the useful macro in list.h always walk from head->next,
 */
struct DList{

		//list head of this DList
		struct list_head dlist_head;

		//lock for the DList
		spinlock_t dlist_lock;

		//always point to next node
		struct DList_Node *next_node;

		//TODO improve performance by cur_entry_ptr and cur_entry_index

};


/**
 * DL_Check_Func : check function prototype, user specified check function for getting data.
 *
 * @param void * src- the user's data structure is passed from DList
 * @param void * arg- the comparison condition arguments
 * @return	true - this user_data is what I want. otherwise, false.
 *
 * Description:
 * 		DList would traverse the list and pass user_data into check function,
 * 		user can specify what condition or parameter he/she want to check.
 * 		ex: check id.
 *
 * N.B. - don't sleep at check function!!
 */
typedef int (*DL_Check_Fn)(void *src, void *arg);

typedef void (*DL_User_Data_Release_Fn)(void *user_data);

typedef void (*DL_UPrint_Fn)(void *user_data);



/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/

/**
 * Create_DList
 *
 * @return dlist
 *
 * Description:
 * 		create and init the list_head and lock
 */
struct DList * Create_DList(void);

int __Init_DList(struct DList *dlist);


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
void Release_All_User_Data(struct DList *dlist, DL_User_Data_Release_Fn ud_release);


/**
 * Release_DList
 *
 * @param dlist - the list you want to release
 * @return	0 - if release success
 * 			negative - error code
 *
 * Description:
 * 		release only DList if empty.
 */
void Release_DList(struct DList *dlist);


/**
 * Release_DList_and_User_Data
 *
 * @param dlist
 * @param ud_release  - user data release function.
 *
 * DESCRIPTION:
 * 		if user knows the list has remained something and wants to release them when release DList,
 * 		call this API with a user specified user_data release function.
 */
void Release_DList_and_User_Data(struct DList *dlist, DL_User_Data_Release_Fn ud_release);


/**
 * Is_DList_Empty - check whether DList is empty or not.
 *
 * @param dlist
 * @return true - list is empty, otherwise false.
 *
 * Description:
 */
int Is_DList_Empty(struct DList *dlist);


/**
 * Get_User_Data_By_Check_Func
 * @param dlist
 * @param checkfn - user specified check function
 * @return void * - user_data
 *
 * Description:
 * 		get corresponding user data from DList by user specified check function,
 * 		This API returns the pointer of user_data only. The DList_Node won't be remove and release from DList.
 */
void * Get_User_Data_By_Check_Func(struct DList *dlist, DL_Check_Fn checkfn, void *args);

void * Get_Next_User_Data(struct DList *dlist);
int Reset_DList_Iterator(struct DList *dlist);
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
int Insert_User_Data_to_DList_Head(struct DList *dlist, void *user_data);


/**
 * Insert_User_Data_to_DList_Tail
 *
 * @param dlist
 * @param user_data
 * @return	0 - insert ok
 *
 * Description:
 */
int Insert_User_Data_to_DList_Tail(struct DList *dlist, void *user_data);

//TODO we can provide some function like "insert by user-specified function" if necessary.


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
 */
void * Remove_from_DList_by_User_Data(struct DList *dlist, void *user_data, int *empty);


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
void * Remove_from_DList_by_Check_Func(struct DList *dlist, DL_Check_Fn checkfn, void *args, int *empty);


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
void * Remove_from_DList_Head(struct DList *dlist, int *empty);


/**
 * Print_DList_by_UPrint_Func
 *
 * @param dlist
 * @param pfn
 *
 * DESCRIPTION:
 * 		print out user_data by user specified print function.
 */
void Print_DList_by_UPrint_Func(struct DList *dlist, DL_UPrint_Fn pfn);



#endif /* DLIST_H_ */
