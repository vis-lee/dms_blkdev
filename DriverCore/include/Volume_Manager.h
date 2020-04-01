/*
 * Volume_Manager.h
 *
 *  Created on: 2011/7/13
 *      Author: 980263
 */

#ifndef VOLUME_MANAGER_H_
#define VOLUME_MANAGER_H_



#include "DMS_Common.h"
#include "Volume.h"


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/



/********************************************************************************/
/*																				*/
/*							CCMA Volume State	 								*/
/*																				*/
/********************************************************************************/
//#define ATTACHED_BIT	0	//the disk has been attached
//#define DETACHING_BIT	1	//the disk is detaching
//
//#define	 ATTACHED	(1 << ATTACHED_BIT)
//#define	 DETACHING	(1 << DETACHING_BIT)


/********************************************************************************/
/*																				*/
/*								Definitions	 									*/
/*																				*/
/********************************************************************************/
//#define NUM_VOLUMES 100

//#define SET_DS_ATTACHED(disk)			( set_bit(ATTACHED_BIT, &disk->volume_state) )
//#define SET_DS_DETACHING(disk)			( set_bit(DETACHING_BIT, &disk->volume_state) )
//#define CLEAR_DS_ATTACHED(disk)			( clear_bit(ATTACHED_BIT, &disk->volume_state) )
//#define CLEAR_DS_DETACHING(disk)		( clear_bit(DETACHING_BIT, &disk->volume_state) )
//#define SET_DS_NO_ATTACH(disk)			  CLEAR_DS_ATTACHED(disk); CLEAR_DS_DETACHING(disk);
//
//#define OPEN_COUNTER_IN_VOL(vol) ( atomic_read(&vol->open_counter) )
//#define NUM_LINGER_IO_IN_VOL(st) ( atomic_read(&st.wreq_counter) + atomic_read(&st.rreq_counter) + atomic_read(&st.ovwreq_counter) )
//
//#define IS_IOREQ_EMPTY_IN_VOL(st)	( atomic_read(&st.wreq_counter) == 0 && atomic_read(&st.rreq_counter) == 0 && atomic_read(&st.ovwreq_counter) == 0 )
//#define IS_OPEN_COUNTER_EMPTY_IN_VOL(vol)	( OPEN_COUNTER_IN_VOL(vol) == 0 )
//#define IS_ALL_IO_DONE(st)			IS_IOREQ_EMPTY_IN_VOL(st)


/********************************************************************************/
/*																				*/
/*							MINOR NUMBER RELATED								*/
/*																				*/
/********************************************************************************/

// CCMA disk drives minor number start index
#define DMS_MINOR_START		1

// DISK_MINORS defined how many followed number from first_minor this disk can use for partition. MINOR_BITS is 20.
#define DMS_DISK_MINORS		16

/* CCMA_MINOR separates every 16 numbers for partition of the disk drive use. */
#define GET_DMS_MINOR(m)									(m * DMS_DISK_MINORS)
#define GET_DMS_VOLUMES_INDEX_BY_MINOR(m)					(m / DMS_DISK_MINORS)


#define IS_IN_ALL_VOLUME_RANGE(index) IS_IN_RANGE(index, DMS_MINOR_START, (dms_max_nr_volumes - 1) )


/********************************************************************************/
/*																				*/
/*						CCMA BLOCK DEVICE DRIVER INFO 							*/
/*																				*/
/********************************************************************************/

struct DMS_Volume_Manager{

	//IO request hash table
//	struct list_head node;

	//
};



/********************************************************************************/
/*																				*/
/*							VOLUME MANAGER FUNCS 								*/
/*																				*/
/********************************************************************************/
/*
 * get volume ptr by volume ID
 */
struct DMS_Volume * Get_Volume_ptr_by_VolumeID(long64 volumeID );

struct DMS_Volume * Get_Volume_ptr_by_gendisk(struct gendisk *disk );

/*
 * check the volume already exist or not,
 * return 1 if exist, 0 otherwise
 */
int Check_Volume_Existence(long64 volumeID);


/*
 * create a dms volume and occupy one position in all_volumes[]
 */
struct DMS_Volume * Create_and_Insert_DMS_Volume(long64 volumeID, ulong64 capacity);

/*
 * free the memory and set the content of all_volumes[index] to NULL
 */
void Remove_and_Release_DMS_Volume(struct DMS_Volume *volume);

/*
 * init Volume_Manager
 */
int Init_DMS_Volume_Manager(void);

/*
 * release Volume_Manager
 */
void Release_DMS_Volume_Manager(void);

/*
 * Identify how many remain ios by keeping three counters to tracking remaining read and write requests.
 * if all the three counters are 0, wake up this process and return.
 */
int Flush_DMS_Volume_IO(struct DMS_Volume *volume);

int Flush_IO_by_VolumeID(long64 volumeID);


/*
 * wake up the wait queue
 */
int Wakeup_Volume_WQ(long64 volumeID);

//TODO
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
int DMS_Attach_Volume(long64 volumeID, ulong64 capacity);


/*
 * detach a volume, set dms volume status to detached
 */
int DMS_Detach_Volume(long64 volumeID);

/*
 * open a dms volume, increase open counter
 *
 * find the corresponding volume by minor number
 */
int Open_DMS_Volume(unsigned unit);


/*
 * close a dms volume, decrease open counter
 *
 * find the corresponding volume by minor number
 */
int Close_DMS_Volume(unsigned unit);




#endif /* VOLUME_MANAGER_H_ */
