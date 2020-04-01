/*
 * DIO.h
 *
 *  Created on: Mar 1, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef DIO_H_
#define DIO_H_

#include "Volume.h"
#include "Metadata.h"


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

struct LogicalBlock_MetaData;

/********************************************************************************/
/*																				*/
/*								Definitions	 									*/
/*																				*/
/********************************************************************************/

#define DIO_MAGIC					0xd10a116e	/*dioalive*/

#define INIT_DIO_RETRY_COUNT(c)		(*(c)=1)


/* Finite state machine states */
enum DIO_ST{

	DIO_INIT 			= 1,	//!< DIO_INIT
	DIO_WAIT_META_DATA, 		//!< WAIT_META_DATA
	DIO_WAIT_DN_ACK,    		//!< WAIT_DN_ACK
	DIO_COMMIT_TO_USER,
	DIO_END,        			//!< DIO_END
	DIO_COMMIT_TO_NAMENODE,

};


struct DMS_IO {

		ulong64 did;

		rwlock_t lock;

		struct DMS_Volume *volume;

		struct IO_Request *io_req;

		ulong64 sLBID;
		ulong64 nr_LBIDs;

		//op type: read/write/ovw
		char op;

#ifdef ENABLE_TCQ
		//the index in the TCQ
		int tag;
#endif

		//the LB List  //TODO use vector?
		struct LogicalBlock_List *LB_list;

		//io state
		atomic_t state;

		/*
		 * DMS Metadata section,
		 */
		struct DMS_Metadata *dmeta;
//		// number of located requests
//		short nr_lrs;
//
//		//located request point out where the data located.
//		struct Located_Request **lrs;

		//basically, initial ref count equal nr_LBIDs. then decrease to 0. used to prevent concurrency issue.
		//atomic_t ref_cnt;

		int retry_count;

		//result
		atomic_t result;

		//we can use this to check memory health, ie, is someone overflow me?
		u32		magic;


};



/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/

struct DMS_IO * Create_DIO(struct IO_Request *ior, char op);
void Release_DIO(struct DMS_IO *dio);

struct IO_Request * __Make_DIO_Requst(struct request *kreq);

int Commit_LB_to_DMS_IO(struct DMS_IO *dio, ulong64 lbid, int nr_lbids, int result);

int SPrint_DIO(char *buf, int len_limit, struct DMS_IO *dio);
void Print_DIO(struct DMS_IO *dio);

extern struct LogicalBlock_MetaData * Get_LBMD_from_Node_by_LBID(struct LogicalBlock_List *lb_list, struct LogicalBlock_MetaData *cur, ulong64 lbid);

#endif /* DIO_H_ */
