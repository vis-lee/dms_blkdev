/*
 * IO_Request.h
 *
 *  Created on: Mar 19, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef IO_REQUEST_H_
#define IO_REQUEST_H_



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


/**
 * struct IO_Request - use to trace all the DMS_IOs
 *
 * Description:
 *    when dms vdd receives a request from kernel, it maybe split the request to multiple
 *    dms_ios for different io operation. Therefore, we need this data structure for tracing.
 */
struct IO_Request {

		ulong64 rid;

		struct request *kreq;

		//link list for linking related DMS IOs
		struct DList *diolist;

		//result
		atomic_t result;

		//TODO finite state machine
		struct work_struct timer;

		//do we need donelist?
		//struct DList *donelist;

		//current index
		//int cur_index;

		//number of dios, used to prevent concurrency issue
		atomic_t ref_cnt;


};



/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/

int Add_IO_Reqeust_to_KRequest(struct request *kreq, struct IO_Request *ior);
//int Remove_IO_Reqeust_to_KRequest(struct request *kreq);


int Add_DIO_to_IO_Reqeust(struct IO_Request *ior, struct DMS_IO *dio);
struct DMS_IO * Remove_DIO_from_IO_Reqeust(struct IO_Request *ior, struct DMS_IO *dio, int *empty);
int Remove_and_Release_DIO_from_IO_Reqeust(struct IO_Request *ior, struct DMS_IO *dio);


int Reset_IO_Request_Iterator(struct IO_Request *ior);
struct DMS_IO * Get_Next_DIO_from_IO_Request(struct IO_Request *ior);


int Commit_DIO_to_IO_Request(struct DMS_IO *dio, int result);


int SPrint_IO_Request(char *buf, int len_limit, struct IO_Request *ior);
void Print_IO_Request(struct IO_Request *ior);

inline int Init_IO_Request(struct request *kreq, struct IO_Request *ior);
struct IO_Request * Create_IO_Request(struct request *kreq);
void Release_IO_Request(struct IO_Request *ior);

#endif /* IO_REQUEST_H_ */
