/*
 * Payload.h
 *
 *  Created on: May 17, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef PAYLOAD_H_
#define PAYLOAD_H_

#include "Metadata.h"
#include "Volume.h"
#include "Datanode_Protocol.h"


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
 * DN Tags used to trace which payload belong to which datanode,
 * if datanode reply ack, we can record and commit result by go
 * through this tag list.
 *
 * ex:
 *
 * LocatedRequest example:
 *
 * volumeID = 123;
 *
 *  LBID    1            2            3            4        |    5            6        |    7        |    8
 *  -----------------------------------------------------------------------------------------------------
 *  HBID    11           12           16           17       |    18           21       |    22       |    23
 *  -----------------------------------------------------------------------------------------------------
 *  physical locations:                                     |                          |             |
 *                                                          |                          |             |
 *  DN:RB   DN1:100     DN1:100     DN1:100 |   DN1:200     |
 *  offset      386         387         388 |       165     |
 *                                          |               |
 *          DN2:200     DN2:200     DN2:200 |   DN2:300     |   DN2:200     DN2:200    |   NULL      |   DN2:200
 *              612         613         614 |       234     |       566         567    |       568   |       569
 *                                          |               |                          |             |
 *          DN3:300     DN3:300     DN3:300 |   DN3:400     |   DN3:300     DN3:300    |   NULL      |   DN3:300
 *              886         887         888 |       363     |       435         436    |       437   |       438
 *                                                          |                          |             |
 *                                                              DN4:400     DN4:400    |   NULL      |   DN4:400
 *                                                                  764         765    |       766   |       767
 *                                                                                     |             |
 *
 *
 *	dn_tags = {
 *
 *		DN1: (lr = 1, dnidx = 1)
 *		DN2: (lr = 1, dnidx = 2) <=> (lr = 2, dnidx = 1) <=> (lr = 4, dnidx = 1)
 *		DN3: (lr = 1, dnidx = 3) <=> (lr = 2, dnidx = 2) <=> (lr = 4, dnidx = 2)
 *		DN4: (lr = 2, dnidx = 3) <=> (lr = 4, dnidx = 3)
 *	}
 *
 */


/**
 * used to tag DN to LB mapping
 */
struct DMS_Datanode_Tag {

	ulong64 dnrid;

	int dn_node_index;

	struct DMS_Metadata *dmeta;

	struct SSocket ssk;

	struct DList payload_tags;

	struct DN_Service_Request dn_sreq;

	int retry_count;

	//number of waiting mem/disk ack
	atomic_t ack_state;

};

struct DMS_Payload_Tag {

	struct Located_Request *lr;

	/*
	 * why do we use index rather than ptr,
	 * becuase we want to free and reset the ptr in lr after dn reply disk ack.
	 */
	int dn_index;

	//we don't record LBMD here because it would be duplicated three times. so we record in lr.

};


/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/

int SPrint_DMS_Datanode_Tag(char *dbg_str, int len_limit, struct DMS_Datanode_Tag *ddt);

#endif /* PAYLOAD_H_ */
