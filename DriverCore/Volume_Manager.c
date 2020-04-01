/*
 * Volume_Manager.c
 *
 *  Created on: 2011/7/13
 *      Author: Vis Lee
 *      		Lego Lin
 *
 * 1. Manage volumes information in an array
 * 2. Create Volume_IO_Reqeust per Volume
 * 3. translate start_sector and sector_num to LBID and LBID_Len
 * 4. Check re-entrance LB, ie. overlapping, if found, add it to lock_list until the previous one done.
 *
 */

//#include <linux/kernel.h>
//#include <linux/spinlock.h>
//#include <linux/wait.h>
#include <linux/delay.h>

#include "DMS_Common.h"
#include "Volume_Manager.h"
//#include "DMS_Mem_Pool.h"
//#include "DMS_Debug.h"
//#include "DMS_Error.h"
#include "DMS_IOCTL.h"
//#include "Volume.h"


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

//TODO export to procfs for configurable
int dms_max_nr_volumes=32;

/* volumes array, indexed by minor number*/
struct DMS_Volume **all_volumes;

/* all volumes array's lock*/
spinlock_t all_volumes_lock = SPIN_LOCK_UNLOCKED;

/* wait queue for flushing volume */
wait_queue_head_t volume_wq;

/* number of detach cmds */
atomic_t nr_detach_cmds;

/* state of volume manager */
static atomic_t vmgr_state;


extern char *IOCTL_MOD;


//TODO useful function: find_first_bit(), we can use bit map to search empty position.

/********************************************************************************/
/*																				*/
/*							ALL_VOLUME UTILS 									*/
/*																				*/
/********************************************************************************/
/*
 *
 */
//inline int IS_IN_ALL_VOLUME_RANGE(ulong64 volumeID, int index, char *funcname){
//
//	if( IS_IN_RANGE(index, CCMA_MINOR_START, (dms_max_nr_volumes - 1) ) ){
//
//		return true;
//
//	}else{
//
//		eprintk("%s: [%s], %s, volume = %llu which minor_index = %d isn't within volume_array_range!\n",
//				VOLUME_MOD, get_current()->comm, funcname, volumeID, index);
//	}
//
//	return false;
//}

/*
 * get volume ptr by volume ID
 */
struct DMS_Volume * Get_Volume_ptr_by_VolumeID(long64 volumeID)
{

	int i = 0;
	ulong flags = 0;
	struct DMS_Volume *volume = NULL;

	spin_lock_irqsave(&all_volumes_lock, flags);

	for (i = DMS_MINOR_START; i < dms_max_nr_volumes; i++)
	{

		if (CHECK_PTR(VOLUME_MOD, all_volumes[i]))
		{

			if (all_volumes[i]->volumeID == volumeID)
			{

				volume = all_volumes[i];

				break;
			}
		}
	}

	spin_unlock_irqrestore(&all_volumes_lock, flags);

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volume ptr = %p \n", volume);

	return volume;
}

struct DMS_Volume * Get_Volume_ptr_by_gendisk(struct gendisk *disk)
{

	int drive_index = 0;
	ulong flags = 0;
	struct DMS_Volume *volume = NULL;

	if (IS_LEGAL(VOLUME_MOD, disk))
	{

		//get index by minor
		drive_index = GET_DMS_VOLUMES_INDEX_BY_MINOR(disk->first_minor);

		spin_lock_irqsave(&all_volumes_lock, flags);
		volume = all_volumes[drive_index];
		spin_unlock_irqrestore(&all_volumes_lock, flags);

		if (unlikely(!volume))
		{
			eprintk(VOLUME_MOD, "volume name = %s not exist! \n", disk->disk_name);
		}

	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volume ptr = %p \n", volume);

	return volume;
}

/*
 * internal function. outside module should get volume ptr directly rather than index
 * due to we don't export all_volumes[].
 */
int __Get_DMS_Volume_Index_by_VolumeID(long64 volumeID)
{

	int i = 0, index = -1;
	unsigned long flags = 0;

	spin_lock_irqsave(&all_volumes_lock, flags);

	//go thru all_volumes
	for (i = DMS_MINOR_START; i < dms_max_nr_volumes; i++)
	{
		if (CHECK_PTR(VOLUME_MOD, all_volumes[i]))
		{
			if (all_volumes[i]->volumeID == volumeID)
			{
				index = i;
				break;
			}
		}
	}

	spin_unlock_irqrestore(&all_volumes_lock, flags);

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volumeID = %lld in index = %d \n", volumeID, index);

	return index;

}


/*
 * CCMA BLKDEV use the minor to index the volume pointer position in the all_volumes[].
 * ignore the index 0 due to the minor will be 0 when first open the /dev/dmsxxx.
 *
 * this function find an available index and occupy the position by volumeID.
 * RECOVER_FUNC: __Remove_Volume_from_All_Volumes()
 * return index in all_volumes[]
 */
int __Occupy_in_All_Volumes(long64 volumeID)
{

	int i = 0, index = -1;
	unsigned long flags = 0;

	spin_lock_irqsave(&all_volumes_lock, flags);

	//start from 1
	for (i = DMS_MINOR_START; i < dms_max_nr_volumes; i++)
	{
		//find an available position
		if (all_volumes[i] == NULL)
		{
			all_volumes[i] = (struct DMS_Volume *)volumeID;
			index = i;
			break;
		}
	}

	spin_unlock_irqrestore(&all_volumes_lock, flags);

	//check the index
	if(!IS_IN_ALL_VOLUME_RANGE(index))
	{
		DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "all_volumes has no available position (index = %d) for volume [%lld]! \n",
				index, volumeID);

		index = -EVMGR_OUT_OF_RANGE;

	}else{

		DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volume [%lld] occupied at all_volumes[%d] done~! \n",
				volumeID, index);
	}


	return index;
}


/*
 * CCMA BLKDEV use the minor to index the volume pointer position in the all_volumes[].
 * ignore the index 0 due to the minor will be 0 when first open the /dev/dmsxxx.
 *
 * this function insert DMS_Volume into the all_volumes[volume->minor_index] which get index by __Occupy_in_All_Volumes()
 * return index in all_volumes[]
 */
int __Insert_Volume_to_All_Volumes(struct DMS_Volume *volume)
{

	int index = -1;
	unsigned long flags = 0;

	if (IS_LEGAL(VOLUME_MOD, volume)){

		index = volume->minor_index;

		//check the index
		if( IS_IN_ALL_VOLUME_RANGE(index) )
		{

			spin_lock_irqsave(&all_volumes_lock, flags);

			//if occupied volumeID is myself
			if( ( (long64)all_volumes[index] == volume->volumeID ) )
			{
				all_volumes[index] = volume;
			}
			else
			{
				eprintk(VOLUME_MOD, "the occupied volumeID = %lld isn't myself = %lld !!\n",
						(long64)all_volumes[index], volume->volumeID);

				index = -DMS_FAIL;
			}

			spin_unlock_irqrestore(&all_volumes_lock, flags);

		}
		else
		{
			index = -EVMGR_OUT_OF_RANGE;

			PRINT_DMS_ERR_LOG(index, VOLUME_MOD, "[%s], volume = %lld which minor_index = %d.",
								get_current()->comm, volume->volumeID, index);
		}

	}


	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volume [%lld] insert to all_volumes[%d] done~!\n", volume->volumeID, index);

	return index;
}

/*
 * Remove volume by index
 * return retcode rather than volume ptr.
 */
int __Remove_Volume_from_All_Volumes(int index)
{

	int retcode = -1;

	unsigned long flags = 0;

	if( IS_IN_ALL_VOLUME_RANGE(index) ){

		DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volume remove from all_volumes[%d] start~!\n", index);

		spin_lock_irqsave(&all_volumes_lock, flags);

		all_volumes[index] = NULL;

		spin_unlock_irqrestore(&all_volumes_lock, flags);

		retcode = DMS_OK;

	}
	else
	{
		retcode = -EVMGR_OUT_OF_RANGE;
	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volume remove done~!\n");

	return retcode;
}

/*
 * check the volume already exist or not,
 * return 1 if exist, 0 otherwise
 */
int Check_Volume_Existence(long64 volumeID)
{

	int retcode = false;
	int index = __Get_DMS_Volume_Index_by_VolumeID(volumeID);

	//check whether this volume is attached or not.
	if (IS_IN_ALL_VOLUME_RANGE(index))
	{
		retcode = true;
	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "index = %d\n", index);

	return retcode;
}


/********************************************************************************/
/*																				*/
/*							INIT VOLUME MANAGER RELATED 						*/
/*																				*/
/********************************************************************************/

/*
 * create a dms volume and occupy one position in all_volumes[]
 */
struct DMS_Volume * Create_and_Insert_DMS_Volume(long64 volumeID, sector_t dsectors)
{

	int index = -1;

	struct DMS_Volume *volume = NULL;

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "create volume [%lld] start~!\n", volumeID);

	//occupy one index first to prevent reentrant race condition
	index = __Occupy_in_All_Volumes(volumeID);


	if ( IS_IN_ALL_VOLUME_RANGE(index) )
	{

		//create dms volume
		volume = __Create_Volume(volumeID, dsectors, index);

		if( volume != NULL ){

			index = __Insert_Volume_to_All_Volumes(volume);

			DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "all_volumes[%d] volume ptr = %p, request queue ptr = %p, disk ptr = %p \n",
							index, volume, volume->r_queue, volume->disk);

			//insert fail
			if (!IS_IN_ALL_VOLUME_RANGE(index))
			{
				__Release_Volume(volume);

				volume = NULL;
			}

		}
		else
		{

			//release occupied position.
			__Remove_Volume_from_All_Volumes(index);

		}

	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "create volume [%lld] done~! ptr = %p \n", volumeID, volume);

	return volume;
}

///*
// * the internal use release_function without lock
// */
//void __Release_DMS_Volume_From_All_Volumes(struct DMS_Volume *volume)
//{
//
//	if (IS_LEGAL(VOLUME_MOD, volume))
//	{
//
//		if (IS_IN_ALL_VOLUME_RANGE(volume->minor_index))
//		{
//
//			all_volumes[volume->minor_index] = NULL;
//
//		}
//		else
//		{
//
//			eprintk("%s: [%s] want to remove volume = %llu which minor_index = %d isn't within volume_array_range!\n",
//					VOLUME_MOD, get_current()->comm, volume->volumeID, volume->minor_index);
//		}
//
//		//__Release_Volume(volume);
//	}
//
//}

/*
 * free the memory and set the content of all_volumes[index] to NULL
 */
void Remove_and_Release_DMS_Volume(struct DMS_Volume *volume)
{

	if (IS_LEGAL(VOLUME_MOD, volume))
	{

		DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volume %lld remove from all_volumes[%d] start~!\n",
				volume->volumeID, volume->minor_index);

		__Remove_Volume_from_All_Volumes(volume->minor_index);

		__Release_Volume(volume);
	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "remove done~!\n");

}

/*
 * if you have to increase the max volume size, you can call this API.
 * BUT, it can't be decrease max volume size for now.
 */
int __Dynamic_Change_Max_Volume_Size(int size)
{

	int retcode = -DMS_FAIL;
	ulong flags = 0;

	struct DMS_Volume **volume_array = (struct DMS_Volume **) DMS_Malloc_NOWAIT(sizeof(struct DMS_Volume *) * size);

	if (IS_LEGAL(VOLUME_MOD, volume_array))
	{

		//init all_volumes[] to NULL
		memset(volume_array, 0, (sizeof(struct DMS_Volume *) * size));

		if (IS_LEGAL(VOLUME_MOD, all_volumes) && size > dms_max_nr_volumes)
		{

			spin_lock_irqsave(&all_volumes_lock, flags);

			//copy from all_volumes[] to new_all_volumes[]
			memcpy( volume_array, all_volumes, ((sizeof(struct DMS_Volume *) * dms_max_nr_volumes)) );
			dms_max_nr_volumes = size;

			spin_unlock_irqrestore(&all_volumes_lock, flags);

			retcode = DMS_OK;

		}
		else
		{
			retcode = -EVMGR_DEC_MAX;
		}

	}
	else
	{

		eprintk(VOLUME_MOD, "no memory~! allocate fail!\n");

	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "done~!\n");

	return retcode;

}

/*
 * init Volume_Manager
 */
int Init_DMS_Volume_Manager()
{

	int retcode = -DMS_FAIL;

	all_volumes = (struct DMS_Volume **) DMS_Malloc_NOWAIT(sizeof(struct DMS_Volume *) * dms_max_nr_volumes);

	if (IS_LEGAL(VOLUME_MOD, all_volumes))
	{

		//init lock
		spin_lock_init(&all_volumes_lock );

		//init all_volumes[] to NULL
		memset(all_volumes, 0, (sizeof(struct DMS_Volume *) * dms_max_nr_volumes));

		init_waitqueue_head(&volume_wq);

		atomic_set(&nr_detach_cmds, 0);

		retcode = DMS_OK;

	}
	else
	{

		eprintk(VOLUME_MOD, "no memory~! allocate fail!\n");

	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "done~!\n");

	return retcode;

}

/*
 * release Volume_Manager
 * TODO we have to think about how to prevent concurrency issue
 * sol 1: use a new local pointer point to all_volume and set all_volume to NULL,
 * 			but you need to check all_volume ptr in all functions
 * sol 2: prevent by system status checking.
 */
void Release_DMS_Volume_Manager()
{

	int i;

	struct DMS_Volume *volume = NULL;

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "start~!\n");

	for (i = DMS_MINOR_START; i < dms_max_nr_volumes; i++)
	{

		if ( all_volumes[i] != NULL && CHECK_PTR(VOLUME_MOD, all_volumes[i]) )
		{
			volume = all_volumes[i];

			//remove from volume manager
			__Remove_Volume_from_All_Volumes(i);

			//flush volume and release memory
			__Release_Volume(volume);

		}
	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "done!\n");

}


/********************************************************************************/
/*																				*/
/*							VOLUME IOCTL RELATED		 						*/
/*																				*/
/********************************************************************************/

////TODO !!add flush IOCTL
//
///*
// * Identify how many remain ios by keeping three counters to tracking remaining read and write requests.
// * if all the three counters are 0, wake up this process and return.
// */
//int Flush_IO_by_Volume_ptr(struct DMS_Volume *volume){
//
//	if(IS_LEGAL(VOLUME_MOD, volume)){
//
//		//if there are remaining IOs
//		//TODO while( NUM_LINGER_IO_IN_VOL(volume->statistic) + OPEN_COUNTER_IN_VOL(volume->statistic) )
//		while( !IS_ALL_IO_DONE(volume->linger_io) || OPEN_COUNTER_IN_VOL(volume) )
//		{
//			wprintk("%s%s, volumeID = %lld, NUM_LINGER_IO_IN_VOL() = %d, OPEN_COUNTER_IN_VOL() = %d, waiting to finish. \n",
//					volume->volumeID, NUM_LINGER_IO_IN_VOL(volume->linger_io), OPEN_COUNTER_IN_VOL(volume) );
//
//			//waiting flush finish
//			wait_event_interruptible( volume_wq, IS_ALL_IO_DONE(volume->linger_io) );
//		}
//	}
//
//	return 0;
//
//}

int Flush_IO_by_Volume_ptr(struct DMS_Volume *volume)
{

	//TODO return __Flush_Volume_IO(volume);

	return DMS_OK;
}

int Flush_IO_by_VolumeID(long64 volumeID)
{

	struct DMS_Volume *volume = Get_Volume_ptr_by_VolumeID(volumeID);

	//TODO return Flush_IO_by_Volume_ptr(volume);

	return DMS_OK;

}

/*
 * wake up all waiting volumes in the wait queue to check whether the IO has been done or not.
 */
int Wakeup_Volume_WQ(long64 volumeID)
{

	if (atomic_read(&nr_detach_cmds) > 0)
	{

		struct DMS_Volume *volume = Get_Volume_ptr_by_VolumeID(volumeID);

		if (IS_LEGAL(VOLUME_MOD, volume))
		{

			//if this volume state is detaching, go to wake up the sleeping processes.
			if (volume->volume_state & DETACHING)
			{

				if (waitqueue_active(&volume_wq) && IS_ALL_IO_DONE(volume->linger_io))
				{

					DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volumeID = %lld, NUM_LINGER_IO_IN_VOL() = %d, IS_IOREQ_EMPTY_IN_VOL() = %d \n",
							volume->volumeID, NUM_LINGER_IO_IN_VOL(volume->linger_io), IS_IOREQ_EMPTY_IN_VOL(volume->linger_io) );

					wake_up_interruptible_all(&volume_wq);
				}
			}
		}
	}

	return DMS_OK;
}

//TODO
#if 0
/*
 * decrease the linger rw counter
 */
int Decrease_RW_Counter(struct NN_req *nn_req, char *caller)
{

	int retcode = 0;

	if(check_ptr_validation(FLUSH_MOD, __func__, nn_req) &&
			check_ptr_validation(FLUSH_MOD, __func__, nn_req->req) &&
			check_ptr_validation(FLUSH_MOD, __func__, nn_req->req->drive) )
	{
		DMS_PRINTK(KERN_DEBUG "%s: the caller is: %s, req_id = %llu \n", __func__, caller, nn_req->nnreq_id);

		struct DMS_Volume *drive = nn_req->req->drive;

		switch( nn_req->rw )
		{

			case KERN_REQ_WRITE:
			atomic_dec(&drive->linger_io.wreq_counter);
			break;

			case KERN_REQ_OVERWRITE:
			atomic_dec(&drive->linger_io.ovwreq_counter);
			break;

			case KERN_REQ_READ:
			atomic_dec(&drive->linger_io.rreq_counter);
			break;

			default:
			DMS_PRINTK(KERN_DEBUG "%s, volumeID = %d, unknow rw type: %d \n", __func__, drive->volinfo.volid, nn_req->rw);
			retcode = -1;
			break;
		}
	}

	return retcode;

}

/*
 * count the linger_rw_request based on number of nn_request
 */
int Increase_RW_Counter(struct NN_req *nn_req, char *caller)
{

	int retcode = 0;

	if(check_ptr_validation(FLUSH_MOD, __func__, nn_req) &&
			check_ptr_validation(FLUSH_MOD, __func__, nn_req->req) &&
			check_ptr_validation(FLUSH_MOD, __func__, nn_req->req->drive) )
	{
		DMS_PRINTK(KERN_DEBUG "%s: the caller is: %s, req_id = %llu \n", __func__, caller, nn_req->nnreq_id);

		struct DMS_Volume *volume = nn_req->req->volume;

		switch( nn_req->rw )
		{

			case KERN_REQ_WRITE:
			atomic_inc(&volume->linger_io.wreq_counter);
			break;

			case KERN_REQ_OVERWRITE:
			atomic_inc(&volume->linger_io.ovwreq_counter);
			break;

			case KERN_REQ_READ:
			atomic_inc(&volume->linger_io.rreq_counter);
			break;

			default:
			DMS_PRINTK(KERN_DEBUG "%s, volumeID = %d, unknow rw type: %d \n", __func__, volume->volinfo.volid, nn_req->rw);
			retcode = -1;
			break;
		}
	}

	return retcode;

}
#endif



/*
 * attach volume, setup dms volume status
 */
int DMS_Attach_Volume(long64 volumeID, sector_t dsectors)
{

	int retcode = -ATTACH_FAIL;
	struct DMS_Volume *volume = NULL;
//	struct gendisk *disk = NULL;
//	struct request_queue *r_queue = NULL;

	iprintk(IOCTL_MOD, "volumeID=%lld, capacity=%llu (in DMSBLK sectors) \n", volumeID, dsectors);

	//check whether this volume is attached or not.
	if( likely(!Check_Volume_Existence(volumeID)) ){

		//create dms volume
		volume = Create_and_Insert_DMS_Volume(volumeID, dsectors);

		if( likely(volume != NULL) ) {

			//set the voluem state to "attached"
			retcode = __Attach_Volume(volume);

			iprintk(IOCTL_MOD, "attach vol DONE, volumeID = %lld\n", volumeID);

		} else {

			eprintk(IOCTL_MOD, "ERROR! there is no available drive\n");
			retcode = -ATTACH_FAIL;

		}

	}else{

		wprintk(IOCTL_MOD, "This volume has been attached!! Don't attach it again!!\n");
		retcode = -EEXIST;
	}

	return retcode;

}

/* FIXME concurrency issue
 * detach a volume, set dms volume status to detach
 */
int DMS_Detach_Volume(long64 volumeID)
{

	int retcode = -DETACH_FAIL;

	struct DMS_Volume *volume = NULL;


	volume = Get_Volume_ptr_by_VolumeID(volumeID);

	/* No such device */
	if( unlikely(!volume) ){

		wprintk(IOCTL_MOD, "no such volume!\n");
		return -ENODEV;
	}
	/* Device or resource busy */
	if( !IS_OPEN_COUNTER_EMPTY_IN_VOL(volume) ){

		wprintk(IOCTL_MOD, "volume is busy!\n");
		return -EBUSY;
	}


	retcode = __Detach_Volume(volume);

	if(retcode == DMS_OK){
		Remove_and_Release_DMS_Volume(volume);
	}


	return retcode;
}

/********************************************************************************/
/*																				*/
/*								GENDISK fops RELATED							*/
/*																				*/
/********************************************************************************/
///*
// * open a dms volume, increase open counter
// *
// * find the corresponding volume by minor number
// */
//int DMS_Volume_Open(unsigned unit) {
//
//	int retcode = 0;
//	int volume_index = GET_DMS_VOLUMES_INDEX_BY_MINOR(unit);
//	struct DMS_Volume *volume = NULL;
//
//	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "CCMA_MINOR_NUM = %d \n", volume_index);
//
//	if( IS_IN_ALL_VOLUME_RANGE(volume_index) ) {
//
//		volume = all_volumes[volume_index];
//
//		if(IS_LEGAL(VOLUME_MOD, volume)){
//
//			//check volume status
//			if( !test_bit(DETACHING_BIT, &volume->volume_state) )
//			{
//				//TODO change to
//				//get_disk(volume);
//				//increase the counter
//				atomic_inc(&volume->open_counter);
//
//				retcode = 0;
//
//			}else{
//
//				//open failed
//				wprintk("The dms volume: %lld is detaching!\n", volume->volumeID);
//				retcode = -EFAULT;
//			}
//
//		}else{
//
//			//invalidate pointer
//			retcode = -EFAULT;
//		}
//	}
//
//	return retcode;
//}
//
///*
// * close a dms volume, decrease open counter
// *
// * find the corresponding volume by minor number
// */
//int DMS_Volume_Release(unsigned unit) {
//
//	int retcode = 0;
//	int drive_index = GET_DMS_VOLUMES_INDEX_BY_MINOR(unit);
//	struct DMS_Volume *volume = NULL;
//
//	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "CCMA_MINOR_NUM(unit) = %d \n", drive_index);
//
//	if( IS_IN_ALL_VOLUME_RANGE(drive_index) ) {
//
//		volume = all_volumes[drive_index];
//
//		if(IS_LEGAL(VOLUME_MOD, volume)){
//
//			//TODO check volume ID also.
//			if(volume){
//
//				//TODO change to
//				//put_disk(volume);
//				//decrease the counter
//				atomic_dec(&volume->open_counter);
//
//				retcode = 0;
//
//			}else{
//
//				//can't find this drive, this shouldn't happen. still return OK for close file.
//				retcode = 0;
//			}
//		}else{
//
//			//can't find this drive, this shouldn't happen. still return OK for close file.
//			retcode = 0;
//		}
//	}
//
//	return retcode;
//
//}


/*
 * open a dms volume, increase open counter
 *
 * find the corresponding volume by minor number
 */
int Open_DMS_Volume(unsigned unit)
{

	int retcode = 0;
	int volume_index = GET_DMS_VOLUMES_INDEX_BY_MINOR(unit);
	struct DMS_Volume *volume = NULL;

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "CCMA_MINOR_NUM = %d \n", volume_index);

	if (IS_IN_ALL_VOLUME_RANGE(volume_index))
	{

		volume = all_volumes[volume_index];

		retcode = __Open_Volume(volume);
	}

	return retcode;
}

/*
 * close a dms volume, decrease open counter
 *
 * find the corresponding volume by minor number
 */
int Close_DMS_Volume(unsigned unit)
{

	int retcode = 0;
	int drive_index = GET_DMS_VOLUMES_INDEX_BY_MINOR(unit);
	struct DMS_Volume *volume = NULL;

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "CCMA_MINOR_NUM(unit) = %d \n", drive_index);

	if (IS_IN_ALL_VOLUME_RANGE(drive_index))
	{

		volume = all_volumes[drive_index];

		retcode = __Close_Volume(volume);
	}

	return retcode;

}


#ifdef DMS_UTEST
EXPORT_SYMBOL(Check_Volume_Existence);
EXPORT_SYMBOL(Create_and_Insert_DMS_Volume);
EXPORT_SYMBOL(Remove_and_Release_DMS_Volume);
EXPORT_SYMBOL(DMS_Attach_Volume);
EXPORT_SYMBOL(DMS_Detach_Volume);
EXPORT_SYMBOL(Open_DMS_Volume);
EXPORT_SYMBOL(Close_DMS_Volume);
EXPORT_SYMBOL(Get_Volume_ptr_by_VolumeID);
#endif
