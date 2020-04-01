/*
 * Test_DMS_DList.c
 *
 *  Created on: Mar 12, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */
#include <linux/kernel.h>
#include <linux/spinlock.h>

#include "DMS_Common.h"
#include "UTest_Common.h"
#include "DList.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *TDLIST_MOD = "TDList: ";

static struct DList *g_tdlist = NULL;

spinlock_t g_tdlist_lock;


/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/
struct Test_DList_Data{
		int tid;
		int eid;
};




/********************************************************************************/
/*																				*/
/*							FUNCS Implementations								*/
/*																				*/
/********************************************************************************/

struct DList * Get_Global_DList(void){

		unsigned long flags = 0;

		if(!g_tdlist){

			//get lock first
			spin_lock_irqsave(&g_tdlist_lock, flags);

			if(!g_tdlist){
				g_tdlist = Create_DList();
			}

			spin_unlock_irqrestore(&g_tdlist_lock, flags);
		}

		return g_tdlist;
}


void Print_Test_DList_Data(void *src){

	struct Test_DList_Data *my_data = src;

	printk("%s%s, tid = %d, eid = %d \n", TDLIST_MOD, __func__, my_data->tid, my_data->eid);

}

struct Test_DList_Data ** Prepare_Test_Data(int tid, int nr_elements){

		int i = 0, offset = 0;
		struct Test_DList_Data **tdd = NULL;

		tdd = (struct Test_DList_Data **)kmalloc( sizeof(struct Test_DList_Data *) * nr_elements, GFP_KERNEL);

		offset = (tid-1) * nr_elements;

		for(i = 0; i < nr_elements; i++){

			tdd[i] = (struct Test_DList_Data *)kmalloc( sizeof(struct Test_DList_Data), GFP_KERNEL);
			tdd[i]->tid = tid;
			tdd[i]->eid = offset + i;

		}

		return tdd;
}

#if 0
void Free_Test_Data(struct DList *dlist, struct Test_DList_Data ** tdd, int nr_elements){

	int i = 0;

	for(i = 0; i < nr_elements; i++){

		if(tdd[i] != NULL){

			struct Test_DList_Data *my_data = NULL;

			my_data = Remove_from_DList_by_User_Data(dlist, tdd[i]);

			//it should be NULL
			if( CHECK_PTR(my_data) ){
				eprintk(TDLIST_MOD, "Test FAIL!! user data shouldn't exist in the list!! tid = %d, eid = %d \n", tdd[i]->tid, tdd[i]->eid);
				kfree(tdd[i]);
				my_data = NULL;
			}
		}

	}

	kfree(tdd);

}
#endif


void I_have_ptr_which_removed_from_list_already(struct DList *dlist, struct Test_DList_Data ** tdd, int nr_elements){

	int i = 0;

	for(i = 0; i < nr_elements; i++){

		if(tdd[i] != NULL){

			struct Test_DList_Data *my_data = NULL;

			my_data = Remove_from_DList_by_User_Data(dlist, tdd[i], NULL);

			//it should be NULL
			if( CHECK_PTR(TDLIST_MOD, my_data) ){

				eprintk(TDLIST_MOD, "Test FAIL!! user data shouldn't exist in the list!! tid = %d, eid = %d \n", tdd[i]->tid, tdd[i]->eid);

				my_data = NULL;
			}
		}

	}

	kfree(tdd);

}


void Add_to_DList(struct DList *dlist, struct Test_DList_Data ** tdd, int nr_elements){

	int i = 0;

	for(i = 0; i < nr_elements; i++){

		Insert_User_Data_to_DList_Tail(dlist, tdd[i]);

	}

//	Print_DList_by_UPrint_Func(dlist, Print_Test_DList_Data);

}

int Test_DList_Check_Func(void *src, void *arg){

	struct Test_DList_Data *my_data = src;
	int eid = (int)arg;

	//iprintk("%s%s, src = %p, arg = %p, my_data->eid = %d, eid = %d \n", TDLIST_MOD, __func__, src, arg, my_data->eid, eid);

	if(my_data->eid == eid){
		DMS_PRINTK(TDLIST_DBG, TDLIST_MOD, "my_data->eid = %d, arg = %d \n", my_data->eid, eid);
		return true;
	}

	return false;
}

void Test_Get_User_Data_By_Check_Func(struct DList *dlist, struct Test_DList_Data ** tdd, int nr_elements){

	int i = 0;

	for(i = 0; i < nr_elements; i++){

		struct Test_DList_Data *my_data = Get_User_Data_By_Check_Func(dlist, Test_DList_Check_Func, (void *)tdd[i]->eid);

		if(my_data != tdd[i]){
			eprintk(TDLIST_MOD, "oh~ my god! my_data = %p isn't match tdd[%d] = %p \n", my_data, i, tdd[i]);
			Print_DList_by_UPrint_Func(dlist, Print_Test_DList_Data);

		}

	}

	iprintk(TDLIST_MOD, "tid = %d, validate DONE! \n", tdd[0]->tid);
}


void Free_Test_DList_Data(void *src){

	kfree(src);
}

void Test_Remove_By_Check_Func(struct DList *dlist, struct Test_DList_Data ** tdd, int nr_elements, int tid){

	int i = 0;
	int last = nr_elements-1;

	for(i = 0; i <= last; i++){

		//remove from back;
		struct Test_DList_Data *my_data = Remove_from_DList_by_Check_Func(dlist, Test_DList_Check_Func, (void *)last-i, NULL);

		if(my_data == tdd[last-i]){

			//I should set tdd[last-i] to NULL here, but I want to test another case bellow. so it's not a bug.
			if( tid != 1 )
				tdd[last-i] = NULL;

			Free_Test_DList_Data(my_data);

		}else {

			eprintk(TDLIST_MOD, "oh~ my god! my_data = %p isn't match tdd[%d] = %p \n", my_data, i, tdd[last-i]);
			Print_DList_by_UPrint_Func(dlist, Print_Test_DList_Data);

		}

	}

	iprintk(TDLIST_MOD, "tid = %d, validate DONE! \n", tid);
}


int Test_DMS_DList(TWorker_t *tw)
{

	int retcode = -DMS_FAIL;
	struct DList *dlist = NULL;
	Test_Workers_t *tws = NULL;

	if(IS_LEGAL(TDLIST_MOD, tw)){

		int tid = tw->tid;
		unsigned int nr_elements = 0;
		struct Test_DList_Data **tdd = NULL;

		tws = (Test_Workers_t *)tw->tws_mgr;

		nr_elements = tws->ut_arg->nr_elements;


		DMS_PRINTK(TDLIST_DBG, TDLIST_MOD, "nr_elements = %u \n", nr_elements);

		tdd = Prepare_Test_Data(tid, nr_elements);

		//get g_dlist
		dlist = Get_Global_DList();

		if( likely( IS_LEGAL(TDLIST_MOD, dlist) &&
				IS_LEGAL(TDLIST_MOD, tdd)) ) {

			//test normal op
			Add_to_DList(dlist, tdd, nr_elements);

			msleep(500);

			//test duplicate, only test one time
			if(tid == 1){
				iprintk(TDLIST_MOD, "test duplicate check! \n");
				Add_to_DList(dlist, tdd, nr_elements);
			}

			//test Get_User_Data_By_Check_Func
			Test_Get_User_Data_By_Check_Func(dlist, tdd, nr_elements);

			//release some obj, not all.
			Test_Remove_By_Check_Func(dlist, tdd, nr_elements/2, tid);

			I_have_ptr_which_removed_from_list_already(dlist, tdd, nr_elements/2);
			iprintk(TDLIST_MOD, "I_have_ptr_which_removed_from_list_already, tid = %d, validate DONE! \n", tid);

			//test release when remian some obj in the list
			if( tws->nr_workers <= 1 ){

				Release_DList_and_User_Data(dlist, Free_Test_DList_Data);

				//we don't need teardown.
				tws->env_apis.teardown = NULL;
				tws->env_apis.verify = NULL;

				//reset g_dlist
				g_tdlist = NULL;
			}

			msleep(500);

			//Free_Test_Data(dlist, tdd, nr_elements);

			DMS_PRINTK(TDLIST_DBG, TDLIST_MOD, "free tdd DONE, test end! \n");

			//done
			if( CHECK_PTR(TDLIST_MOD, tws->done_func) ){
				tws->done_func(tw, retcode);
			}

		}


	}


	return retcode;

}




void Free_Test_Data_Multi_Threads(struct DList *dlist, struct Test_DList_Data ** tdd, int nr_elements){

	int i = 0;

	for(i = 0; i < nr_elements; i++){

		if(tdd[i] != NULL){

			struct Test_DList_Data *my_data = NULL;

			my_data = Remove_from_DList_by_User_Data(dlist, tdd[i], NULL);

			if( CHECK_PTR(TDLIST_MOD, my_data) ){

				kfree(tdd[i]);
				tdd[i] = NULL;
				my_data = NULL;
			}
		}

	}

	kfree(tdd);

}

int Test_DMS_DList_Multi_Threads(TWorker_t *tw)
{

	int retcode = DMS_OK;
	struct DList *dlist = NULL;
	Test_Workers_t *tws = NULL;

	if( IS_LEGAL(TDLIST_MOD, tw) &&
			IS_LEGAL(TDLIST_MOD, tw->tws_mgr) )
	{

		int tid = tw->tid;
		unsigned int nr_elements = 0;
		struct Test_DList_Data **tdd = NULL;

		tws = tw->tws_mgr;
		nr_elements = tws->ut_arg->nr_elements;

		DMS_PRINTK(TDLIST_DBG, TDLIST_MOD, "nr_elements = %u \n", nr_elements);

		tdd = Prepare_Test_Data(tid, nr_elements);

		//get g_dlist
		dlist = Get_Global_DList();

		if( likely( IS_LEGAL(TDLIST_MOD, dlist) &&
				IS_LEGAL(TDLIST_MOD, tdd)) ) {

			//test normal op
			Add_to_DList(dlist, tdd, nr_elements);

			msleep(500);


			//test duplicate, only test one time
			if(tid == 1){
				iprintk(TDLIST_MOD, "test duplicate check! \n");
				Add_to_DList(dlist, tdd, nr_elements);
			}

			//test Get_User_Data_By_Check_Func
			Test_Get_User_Data_By_Check_Func(dlist, tdd, nr_elements);

			//release some obj, not all.
			Test_Remove_By_Check_Func(dlist, tdd, nr_elements/2, tid);

			msleep(500);

			Free_Test_Data_Multi_Threads(dlist, tdd, nr_elements);
			iprintk(TDLIST_MOD, "Free_Test_Data_Multi_Threads, tid = %d, validate DONE! \n", tid);

			//test.... multiple free, result: the kmem_cache free-ed 2 times.
//			Release_DList(dlist);

			DMS_PRINTK(TDLIST_DBG, TDLIST_MOD, "free tdd DONE, test end! \n");

		}

		if( CHECK_PTR(TDLIST_MOD, tws->done_func) ){
			tws->done_func(tw, retcode);
		}


	}


	return retcode;

}




/********************************************************************************/
/*																				*/
/*									Env APIs									*/
/*																				*/
/********************************************************************************/

int Setup_TDList_Env(unsigned long param){

	spin_lock_init(&g_tdlist_lock);

	Get_Global_DList();

	return DMS_OK;

}

void Teardown_TDList_Env(unsigned long param){

	Release_DList_and_User_Data(g_tdlist, Free_Test_DList_Data);

	//reset g_dlist
	g_tdlist = NULL;

}

int Verify_Test_TDList_Result(unsigned long param){

	int retcode = DMS_OK;

	if( Is_DList_Empty(g_tdlist) ){

		iprintk(TDLIST_MOD, "success!! \n");

	}else{

		retcode = -DMS_FAIL;

		//print content
		Print_DList_by_UPrint_Func(g_tdlist, Print_Test_DList_Data);
	}

	return retcode;
}





