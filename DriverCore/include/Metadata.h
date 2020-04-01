/*
 * Metadata.h
 *
 *  Created on: Apr 3, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef METADATA_H_
#define METADATA_H_

#include "DMS_Common.h"
#include "DList.h"

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

#define		METADATA_INIT_BIT 			0 /*(1 << 0)*/
#define		COMMITTED_USER_BIT			1 /*(1 << 1)*/
#define		COMMITTED_NN_BIT			2 /*(1 << 2)*/

#define		METADATA_INIT 				(1 << METADATA_INIT_BIT)
#define		COMMITTED_USER				(1 << COMMITTED_USER_BIT)
#define		COMMITTED_NN				(1 << COMMITTED_NN_BIT)


//#define SET_DN_RESULT(lr, dn_index, result)			(lr->dn_locs[dn_index]->dn_result = result)
//#define DN_RESULT



typedef enum LR_State{

	LR_INVALID = 0,
	LR_VALID,

} LR_State_t;


/**
 * LocatedRequest example:
 *
 *	volumeID = 123;
 *
 *		LBID    1            2            3            4        |    5            6        |    7        |    8
 *		-----------------------------------------------------------------------------------------------------
 *		HBID    11           12           16           17       |    18           21       |    22       |    23
 *		-----------------------------------------------------------------------------------------------------
 *		physical locations:                                     |                          |             |
 *																|                          |             |
 *		DN:RB   DN1:100     DN1:100     DN1:100 |   DN1:200     |   DN2:200     DN2:200    |   NULL      |   DN2:200
 *		offset      386         387         388 |       165     |       566         567    |       568   |       569
 *												|               |                          |             |
 *				DN2:200     DN2:200     DN2:200 |   DN2:300     |   DN3:300     DN3:300    |   NULL      |   DN3:300
 *					612         613         614 |       234     |       435         436    |       437   |       438
 *												|               |                          |             |
 *				DN3:300     DN3:300     DN3:300 |   DN3:400     |   DN4:400     DN4:400    |   NULL      |   DN4:400
 *					886         887         888 |       363     |       764         765    |       766   |       767
 *                                                              |                          |             |
 *
 */


/**
 * indicate block location of datanode
 */
struct Datanode_Location {

	//ip and port
	struct SSocket ssk;

	//number of triplets
	short nr_triplets;

	//triplets combined by rbid offset and length to indicate physical locations.
	ulong64 triplets[KSECTORS_TO_DSECTORS(MAX_KSECTORS)];

	//the result of data node response
	atomic_t dn_result;

	//dn_req of the dn_location
	ulong64 dn_req_id;

}__attribute__ ((__packed__));



struct Located_Request {

	//start lbid
	ulong64 slbid;
	short nr_lbids;

	//indicate this lr_state is never allocate, or all fail after allocated.
	short lr_state;

	short nr_hbids;

	ulong64 HBIDs[KSECTORS_TO_DSECTORS(MAX_KSECTORS)];

	short nr_dn_locs;

	struct Datanode_Location **dn_locs;


	/*
	 * moved to Datanode_Tag. it would be much better to record over there.
	 */
	//number of waiting mem/disk ack
	atomic_t wmem_ack;
	atomic_t wdisk_ack;

	//rwlock
	rwlock_t lock;

	//commit state
	int commit_state;

	//private data, we used to record start LBMD, but not used in Metadata.
	void *private_data;


}__attribute__ ((__packed__));


/*
 * why we need this layer of data structure?
 * Because we want to separate dio and metadata relation.
 * it's easier to replace the metadata module.
 */
struct DMS_Metadata {

	//op type: read/write/ovw
	char op;

	//number of located requests
	int nr_lrs;

	struct Located_Request **lrs;

	//two dimension double linked list.
	struct DList dn_tags;

	ulong64 commitID;

	struct DMS_Volume *volume;

	//parent dio
	struct DMS_IO *pdio;

	//waiting time
	ulong64 wait_time;

	//how many lr is processing.
	atomic_t nr_waiting_lrs;

};


/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/

struct Datanode_Location ** Create_Datanode_Locations(int nr_dns);
void Release_Datanode_Locations(struct Datanode_Location **dns, int nr_dns);

struct Located_Request ** Create_Located_Requests(int nr_lrs);
void Release_Located_Requests(struct Located_Request **lrs, int nr_lrs);

int Init_DMS_Metadata(struct DMS_IO *dio, struct DMS_Metadata *dmeta);
struct DMS_Metadata * Create_DMS_Metadata(struct DMS_IO *dio);
void Release_DMS_Metadata(struct DMS_Metadata *dmeta);


int SPrint_DMS_Metadata(char *dbg_str, int len_limit, struct DMS_Metadata *dmeta);
int SPrint_One_Datanode_Location(char *dbg_str, int len_limit, struct Datanode_Location *dn_loc);
int SPrint_One_Located_Requests(char *dbg_str, int len_limit, struct Located_Request *lr);
int SPrint_Located_Requests(char *dbg_str, int len_limit, struct Located_Request **lrs, int nr_lrs);



static inline unsigned long long compose_triple(int rbid, int len, int offset) {

	unsigned long long val = 0;

	val = ((unsigned long long)rbid) << 32 | ((unsigned long long)len) << 20 | offset;
	return val;
}

/*
 * datanode path format: <Mount Point>/<Datanode ID>-<LUN>/<(Local ID & 0xffc00)>>10>/<RBID>-<Local ID>.dat
 *   ex: /usr/cloudos/data/dms/dms-data/lun/carrier3/15-3/0/1009778692-4.dat
 *       <Mount Point> 			: /usr/cloudos/data/dms/dms-data/lun/carrier3
 *       <Datanode ID>-<LUN> 	: 15-3
 *       <(Local ID & 0xffc00)>>10> : 0
 *   	 <RBID>-<Local ID>.dat	: 1009778692-4.dat
 * RBID: DNID 6 bits / LUN 6 bits / LocalID 20 bits
 * RB's base unit: HBID 8 bytes + data-payload 4096 bytes
 */
static inline void decompose_triple(unsigned long long val, int *rbid, int *len,
		int *offset) {
	unsigned long long tmp;
	tmp = val;
	*rbid = (int) (val >> 32);
	tmp <<= 32;
	*len = (int) (tmp >> 32) >> 20;
	*offset = (int) (val & 0x000000000000ffffLL);

}


static inline void Set_Located_Request_State(struct Located_Request *lr, int state) {

	write_lock(&lr->lock);

	lr->lr_state = state;

	write_unlock(&lr->lock);

}

static inline void Set_Located_Request_PData(struct Located_Request *lr, void *pdata) {

	write_lock(&lr->lock);

	lr->private_data = pdata;

	write_unlock(&lr->lock);

}

/**
 *
 * @param lr
 * @return	true: if no more mem_ack is needed.
 * 			false: waiting for remains.
 */
static inline int Dec_WMem_Ack(struct Located_Request *lr){

	return atomic_dec_and_test(&lr->wmem_ack);
}


/**
 *
 * @param lr
 * @return	true: if no more disk_ack is needed.
 * 			false: waiting for remains.
 */
static inline int Dec_WDisk_Ack(struct Located_Request *lr){

	return atomic_dec_and_test(&lr->wdisk_ack);
}


/**
 *
 * @param lr
 * @return	true: if no more disk_ack is needed.
 * 			false: waiting for remains.
 */
static inline int Dec_Waiting_LRs(struct DMS_Metadata *dmeta){

	return atomic_dec_and_test(&dmeta->nr_waiting_lrs);
}


//static inline void Update_Datanode_Location_Result(struct Datanode_Location *dnl, state){
//
//	//set result, don't overwrite disk_ack if it reply first.
//	//no concurrency, only one receiver for now.
//	if(atomic_read(&dnl->dn_result) != WDISK_ACK){
//		atomic_set(&dnl->dn_result, state);
//	}
//
//}


static inline void Set_Pdio_in_DMS_Metadata(struct DMS_Metadata *dmeta, struct DMS_IO *pdio){

	dmeta->pdio = pdio;
	barrier();

}

#endif /* METADATA_H_ */
