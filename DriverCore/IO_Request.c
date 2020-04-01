/*
 * IO_Request.c
 *
 *  Created on: Mar 19, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include "DMS_Common.h"
#include "IO_Request.h"
#include "DIO.h"
#include "DList.h"
#include "DMS_VDDriver.h"
#include "linux/workqueue.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *IOR_MOD = "IOR: ";

static atomic64_t ior_id_gen = ATOMIC64_INIT(1);


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

int Add_IO_Reqeust_to_KRequest(struct request *kreq, struct IO_Request *ior){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(IOR_MOD, kreq) &&
			IS_LEGAL(IOR_MOD, ior) )
	{
		kreq->special = (void *)ior;
	}

	DMS_PRINTK(IOR_DBG, IOR_MOD, "ior_id = %llu, done\n", ior->rid);

	return retcode;

}

//int Remove_IO_Reqeust_to_KRequest(struct request *kreq){
//
//	int retcode = DMS_OK;
//
//	if( IS_LEGAL(IOR_MOD, kreq) )
//	{
//		kreq->special = NULL;
//	}
//
//	DMS_PRINTK(IOR_DBG, IOR_MOD, "done\n");
//
//	return retcode;
//
//}


/**
 * Add_DIO_to_IO_Reqeust - add to diolist of io_request
 *
 * @param ior
 * @param dio
 * @return 0: OK, -1: fail;
 */
int Add_DIO_to_IO_Reqeust(struct IO_Request *ior, struct DMS_IO *dio){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(IOR_MOD, ior) &&
			IS_LEGAL(IOR_MOD, dio) )
	{
		retcode = Insert_User_Data_to_DList_Tail(ior->diolist, (void *)dio);

		if(retcode == DMS_OK){
			atomic_inc(&ior->ref_cnt);
		}
	}

	DMS_PRINTK(IOR_DBG, IOR_MOD, "ior_id = %llu, dio_id = %llu, done\n", ior->rid, dio->did);

	return retcode;

}

/**
 * Remove_DIO_from_IO_Reqeust - remove from list and return DIO ptr
 *
 * @param ior
 * @param dio
 * @return struct DMS_IO * dio: if dio != NULL means found and removed from list
 */
struct DMS_IO * Remove_DIO_from_IO_Reqeust(struct IO_Request *ior, struct DMS_IO *dio, int *empty){

	struct DMS_IO *ret_dio = NULL;

	if( IS_LEGAL(IOR_MOD, ior) &&
			IS_LEGAL(IOR_MOD, dio) )
	{
		ret_dio = (struct DMS_IO *) Remove_from_DList_by_User_Data(ior->diolist, (void *)dio, empty);
	}

	DMS_PRINTK(IOR_DBG, IOR_MOD, "ior_id = %llu, dio_id = %llu, ret_dio = %p, empty = %d \n", ior->rid, dio->did, ret_dio, *empty);

	return ret_dio;

}

/**
 * Remove_and_Release_DIO_from_IO_Reqeust - remove from list and free the dio
 * @param ior
 * @param dio
 * @return	1, remove success, and list is empty
 * 			0, remove success, but list is not empty
 * 			-1, not found
 */
int Remove_and_Release_DIO_from_IO_Reqeust(struct IO_Request *ior, struct DMS_IO *dio){

	int empty = -DMS_FAIL;
	int retcode = -DMS_FAIL;
	ulong64 rid = 0, did = 0;

	if( IS_LEGAL(IOR_MOD, ior) &&
			IS_LEGAL(IOR_MOD, dio) )
	{
		rid = ior->rid;
		did = dio->did;

		DMS_PRINTK(IOR_DBG, IOR_MOD, "ior_id = %llu, dio_id = %llu, dio = %p \n", rid, did, dio);

		dio = Remove_DIO_from_IO_Reqeust(ior, (void *)dio, &empty);

		if(dio){

			Release_DIO(dio);

			retcode = empty;

		}

	}

	DMS_PRINTK(IOR_DBG, IOR_MOD, "ior_id = %llu, dio_id = %llu, retcode = %d \n", rid, did, retcode);

	return retcode;

}


#if 0
int Get_DIO_by_Index(void *src, void *arg){

	int retcode = false;

	if( IS_LEGAL(IOR_MOD, src) ){

		struct DMS_IO *dio = (struct DMS_IO *)src;

		int i = 0, target_index = (int)arg;


	}

	return retcode;
}
#endif

/**
 * Set_IO_Request_Current_Index - Set current index
 *
 * @param ior
 * @param index
 * @return 	0 if success
 * 			-1 if fail.
 */
int Reset_IO_Request_Iterator(struct IO_Request *ior){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(IOR_MOD, ior) ){

		retcode = Reset_DList_Iterator(ior->diolist);

	}

	return retcode;

}

/**
 * Get_Next_DIO_from_IO_Request - get next element.
 * @param ior
 * @return 	DMS_IO *dio
 * 			NULL if not found.
 *
 * DESCRIPTION:
 * 		Reset iterator by calling "Reset_IO_Request_Iterator()" first,
 * 		and this function always return next element if exist.
 */
struct DMS_IO * Get_Next_DIO_from_IO_Request(struct IO_Request *ior){

	struct DMS_IO *dio = NULL;

	if( IS_LEGAL(IOR_MOD, ior) )
	{

		dio = Get_Next_User_Data(ior->diolist);

	}

	return dio;

}


int Commit_DIO_to_IO_Request(struct DMS_IO *dio, int result){

	int empty = -DMS_FAIL;
	int retcode = -DMS_FAIL;

	if(IS_LEGAL(IOR_MOD, dio)){

		struct IO_Request *ior = (struct IO_Request *)dio->io_req;
		struct request *kreq = ior->kreq;

		atomic_cmpxchg(&ior->result, true, result);

		empty = Remove_and_Release_DIO_from_IO_Reqeust(ior, dio);

		//ior->ref_cnt used to solve concurrency problem, the last and the only one who get 0 should commit to user.
		if( empty == true /*|| result != true*/ /*&& atomic_dec_and_test(&ior->ref_cnt)*/ ){

			//commit to request queue, TODO we can walk thru the dio list to commit different result.
			retcode = DMS_End_Request_Handler( kreq, kreq->nr_sectors, atomic_read(&ior->result) );

			if(!retcode){

				//krequest complete
				//Release_IO_Request(ior);

				//TODO Add_to_Commit_NN_Queue(dio->volume, ior);

				//TODO SET FSM STATE

			}else{

				eprintk(IOR_MOD, "Something remained, it's weird, end io FAIL!! \n");

				DSTR_PRINT(ALWAYS, IOR_MOD, SPrint_IO_Request(DSTR_NAME, DSTR_LIMIT, ior), "IO_Request remains content: ");
			}

		}

		DMS_PRINTK(IOR_DBG, IOR_MOD, "th = %s, retcode = %d \n", (char *)&current->comm, empty);

	}

	return empty;

}

int SPrint_IO_Request(char *buf, int len_limit, struct IO_Request *ior){

	int len = 0;

	if( IS_LEGAL(IOR_MOD, ior) &&
			IS_LEGAL(IOR_MOD, buf) )
	{
		struct DMS_IO *dio = NULL;

		if(len < len_limit){

			len += sprintf(buf+len, "%s, rid = %llu, ref_count = %d \n",
					IOR_MOD, ior->rid, atomic_read(&ior->ref_cnt));

			//go back to head
			Reset_IO_Request_Iterator(ior);

			while( (dio = Get_Next_DIO_from_IO_Request(ior)) )
			{
				if(len_limit-len > 0){
					//print dios
					len += SPrint_DIO(buf+len, len_limit-len, dio);
				}

			}

		}

	}

	return len;

}


void Print_IO_Request(struct IO_Request *ior){

	if(IS_LEGAL(IOR_MOD, ior)){

		DMS_PRINTK(ALWAYS, IOR_MOD, "rid = %llu, ref_count = %d \n", ior->rid, atomic_read(&ior->ref_cnt));

	}

}

inline int Init_IO_Request(struct request *kreq, struct IO_Request *ior){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(IOR_MOD, ior)){

		ior->rid = atomic64_inc_return(&ior_id_gen);

//		ior->volume = Get_DMS_Volume_from_KRequest(kreq);
		ior->kreq = kreq;

		//link list for linking related DMS IOs
		ior->diolist = Create_DList();

		//TODO INIT_WORK(&ior->timer, IO_Timer_Handler, ior);

		//current index
		//ior->cur_index = 0;

		//result set to true
		atomic_set(&ior->result, true);

		atomic_set(&ior->ref_cnt, 0);

		retcode = DMS_OK;

	}

	return retcode;
}


struct IO_Request * Create_IO_Request(struct request *kreq){

	struct IO_Request *ior = NULL;

	ior = Malloc_IO_Request(GFP_KERNEL);

	if( Init_IO_Request(kreq, ior) < 0 ){

		goto INIT_IOR_FAIL;
	}

	DMS_PRINTK(IOR_DBG, IOR_MOD, "ior = %p, ior_id = %llu \n", ior, ior->rid);

	return ior;

INIT_IOR_FAIL:

	if( CHECK_PTR(IOR_MOD, ior) ){
		Free_IO_Request(ior);
	}

	PRINT_DMS_ERR_LOG(EDEV_NOMEM, IOR_MOD, "");

	return NULL;

}


void Release_IO_Request(struct IO_Request *ior){

	ulong64 ior_id = 0;

	if( IS_LEGAL(IOR_MOD, ior) ){

		ior_id = ior->rid;

		ior->rid = 0;

		DMS_PRINTK(IOR_DBG, IOR_MOD, "ior_id = %llu \n", ior_id);

		//free dlist and dios
		Release_DList_and_User_Data(ior->diolist, Release_DIO);
		ior->diolist = NULL;

		ior->kreq = NULL;

		//free dio
		Free_IO_Request(ior);

	}

	DMS_PRINTK(IOR_DBG, IOR_MOD, "ior_id = %llu, done! \n", ior_id);

}





#ifdef DMS_UTEST

EXPORT_SYMBOL(Reset_IO_Request_Iterator);
EXPORT_SYMBOL(Add_IO_Reqeust_to_KRequest);
EXPORT_SYMBOL(Get_Next_DIO_from_IO_Request);
EXPORT_SYMBOL(SPrint_IO_Request);

#endif





