/*
 * Volume.h
 *
 *  Created on: Feb 8, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef VOLUME_H_
#define VOLUME_H_


#include "DMS_Common.h"
#include "DIO.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/
extern char *VOLUME_MOD;


/********************************************************************************/
/*																				*/
/*							CCMA Volume State	 								*/
/*																				*/
/********************************************************************************/
#define ATTACHED_BIT	0	//the disk has been attached
#define DETACHING_BIT	1	//the disk is detaching

#define	 ATTACHED	(1 << ATTACHED_BIT)
#define	 DETACHING	(1 << DETACHING_BIT)


/********************************************************************************/
/*																				*/
/*								Definitions	 									*/
/*																				*/
/********************************************************************************/
//#define NUM_VOLUMES 100

#define SET_DS_ATTACHED(disk)			( set_bit(ATTACHED_BIT, &disk->volume_state) )
#define SET_DS_DETACHING(disk)			( set_bit(DETACHING_BIT, &disk->volume_state) )
#define CLEAR_DS_ATTACHED(disk)			( clear_bit(ATTACHED_BIT, &disk->volume_state) )
#define CLEAR_DS_DETACHING(disk)		( clear_bit(DETACHING_BIT, &disk->volume_state) )
#define SET_DS_NOT_ATTACH(disk)			  CLEAR_DS_ATTACHED(disk); CLEAR_DS_DETACHING(disk);

#define IS_DS_ATTACHED(disk)			( test_bit(ATTACHED_BIT, &disk->volume_state) )
#define IS_DS_DETACHING(disk)			( test_bit(DETACHING_BIT, &disk->volume_state) )


#define OPEN_COUNTER_IN_VOL(vol) 		( atomic_read(&vol->open_counter) )
#define NUM_LINGER_IO_IN_VOL(st) 		( atomic_read(&st.wreq_counter) + atomic_read(&st.rreq_counter) + atomic_read(&st.ovwreq_counter) )

#define IS_IOREQ_EMPTY_IN_VOL(st)			( atomic_read(&st.wreq_counter) == 0 && atomic_read(&st.rreq_counter) == 0 && atomic_read(&st.ovwreq_counter) == 0 )
#define IS_OPEN_COUNTER_EMPTY_IN_VOL(vol)	( OPEN_COUNTER_IN_VOL(vol) == 0 )
#define IS_ALL_IO_DONE(st)					IS_IOREQ_EMPTY_IN_VOL(st)

/*
 * volume statistic macro
 */
#define INCREASE_WST(st)				( atomic_inc(&st.wreq_counter) )
#define INCREASE_RST(st)				( atomic_inc(&st.rreq_counter) )
#define INCREASE_OVWST(st)				( atomic_inc(&st.ovwreq_counter) )

#define DECREASE_WST(st)				( atomic_dec(&st.wreq_counter) )
#define DECREASE_RST(st)				( atomic_dec(&st.rreq_counter) )
#define DECREASE_OVWST(st)				( atomic_dec(&st.ovwreq_counter) )

/*
 * linger IO statistic
 */
#define INC_LWST(vol)				( INCREASE_WST(vol->linger_io) )
#define INC_LRST(vol)				( INCREASE_RST(vol->linger_io) )
#define INC_LOVWST(vol)				( INCREASE_OVWST(vol->linger_io) )



//@for reference
//typedef int (vIO_Handler) (request_queue_t *, struct request *);

/********************************************************************************/
/*																				*/
/*							DMS VDD VOLUME INFO 								*/
/*																				*/
/********************************************************************************/


struct Request_Statistic {

	//counter for counting linger write nn_requests to supporting flush command.
    atomic_t wreq_counter;

    //counter for counting linger read nn_requests to supporting flush command.
    atomic_t rreq_counter;

    //counter for counting linger over_write nn_requests to supporting flush command.
    atomic_t ovwreq_counter;

};


struct DMS_Volume{

	//volume owner
	char *v_owner;

	//volume ID
	long64 volumeID;

	char replica_factor;

	//volume state,
	int volume_state;

	//minor number index
	int minor_index;

    //count how many user open this disk
    atomic_t open_counter;

	//provide to IO_Scheduler for using to lock request_queue
	spinlock_t r_queue_lock;

	//disk request queue
	struct request_queue *r_queue;

	//generic disk data structure
	struct gendisk *disk;

	//volume capacity in sectors by hwsec_size, ie, VOL_SIZE_IN_BYTES/BYTES_PER_DMSBLK
	sector_t dsectors;

	//IO statistic
	struct Request_Statistic io_statistic;

	//Linger IO statistic
	struct Request_Statistic linger_io;

	//IO request hash table

	//request handling function
	int (*vIO_Handler)(struct DMS_Volume *volume, struct request *kreq);

	int (*vIO_Commiter)(struct DMS_Metadata *dmeta, void *data, int result);

	int (*vIO_Read_Policy)(void *data, int param);

	//int (*vIO_Ack_Policy)(void *data, int result);

//	int (*Metadata_Handler)(void *data, int result);
//	int (*Metadata_Commit_Handler)(void *data, int result);
//	int (*Payload_Handler)(void *data, int result);
//	int (*Payload_Ack_Handler)(void *data, int result);

	/* TODO flow control, use nr_on_fly_req to control the request  */
	atomic_t max_reqs;

	//commit to namenode queue
	struct DList *cmt2nn_q;

};


/********************************************************************************/
/*																				*/
/*								VOLUME FUNCS 									*/
/*																				*/
/********************************************************************************/
/*
 * get volume ptr by volume ID
 */
//struct DMS_Volume * Get_Volume_ptr_by_VolumeID(long64 volumeID );
//
//struct DMS_Volume * Get_Volume_ptr_by_gendisk(struct gendisk *disk );

/*
 * check the volume already exist or not,
 * return 1 if exist, 0 otherwise
 */
//int Check_Volume_Existence(long64 volumeID);

/*
 * init io request statistic data structure
 */
int Init_Volume_Statistic(struct Request_Statistic *rs);

/*
 * init dms volume data structure
 */
int Init_DMS_Volume(struct DMS_Volume *volume, long64 volumeID, ulong64 capacity);

/*
 * create one dms volume data structure, the caller should check the return ptr.
 * !!This API only for testing used, don't use it in normal case. Because it won't insert volume to array.
 */
struct DMS_Volume * __Create_Volume(long64 volumeID, ulong64 capacity, int index);

/*
 * create a dms volume and occupy one position in all_volumes[]
 */
//struct DMS_Volume * Create_and_Insert_DMS_Volume(long64 volumeID, ulong64 capacity);

/*
 * flush remain IO in volume and free the memory
 */
void __Release_Volume(struct DMS_Volume *volume);


/*
 * Identify how many remain ios by keeping three counters to tracking remaining read and write requests.
 * if all the three counters are 0, wake up this process and return.
 */
int __Flush_Volume_IO(struct DMS_Volume *volume);



/*
 * wake up the wait queue
 */
//int Wakeup_Volume_WQ(long64 volumeID);

//TODO implement counter in volume
#if 0
/*
 * decrease the linger rw counter
 */
int Decrease_RW_Counter(struct NN_req *nn_req, char *caller);

/*
 * count the linger_rw_request based on number of nn_request
 */
int Increase_RW_Counter(struct NN_req *nn_req, char *caller);
#endif

/*
 * attach volume, setup dms volume status
 */
int __Attach_Volume(struct DMS_Volume *volume);

/*
 * call this API first to prepare to detach a volume, set dms volume status to detaching
 */
int __Detach_Volume_Prep(struct DMS_Volume *volume);

/*
 * detach a volume, set dms volume status to detached
 */
int __Detach_Volume(struct DMS_Volume *volume);


int __DMS_getgeo(struct block_device *bd, struct hd_geometry *hg);

/*
 * open a dms volume, increase open counter
 *
 * find the corresponding volume by minor number
 */
int __Open_Volume(struct DMS_Volume *volume);


/*
 * close a dms volume, decrease open counter
 *
 * find the corresponding volume by minor number
 */
int __Close_Volume(struct DMS_Volume *volume);



/*
 * get gendisk ptr in the dms volume
 */
static inline struct gendisk * Get_gendisk_in_DMS_Volume(struct DMS_Volume *volume){

	struct gendisk *gd = NULL;

	if(IS_LEGAL(VOLUME_MOD, volume)){
		gd = volume->disk;
	}

	return gd;
}


/*
 * get request_queue ptr in the dms volume
 */
static inline struct request_queue * Get_request_queue_in_DMS_Volume(struct DMS_Volume *volume){

	struct request_queue *r_queue = NULL;

	if(IS_LEGAL(VOLUME_MOD, volume)){
		r_queue = volume->r_queue;
	}

	return r_queue;
}


static inline struct DMS_Volume * Get_DMS_Volume_from_KRequest(struct request *kreq){

	struct DMS_Volume *volume = NULL;

	if(IS_LEGAL(VOLUME_MOD, kreq)){
		volume = (struct DMS_Volume *)kreq->rq_disk->private_data;
	}

	return volume;
}



#endif /* VOLUME_H_ */
