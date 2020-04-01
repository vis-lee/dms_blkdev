/*
 * Volume.c
 *
 *  Created on: Feb 8, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 *	Volume.c implements the basic volume operation in DMS.
 */

#include <linux/hdreg.h>

#include "DMS_Common.h"
#include "volume.h"
#include "DMS_VDDriver.h"
#include "Volume_Manager.h"

#include "Driver_Core.h"
//#include "DIO.h"


/********************************************************************************/
/*																				*/
/*								Global variables 								*/
/*																				*/
/********************************************************************************/

/* Volume Manager */
char *VOLUME_MOD = 			"VOLUME: ";

static char *DEVICE_NAME_PREFIX =	"dms";

/*
 * major number that in DMS_VDDriver.c
 */
extern int DMS_DEV_MAJOR;
extern atomic_t nr_detach_cmds;

/*
 * dms volume fops for gendisk use.
 */
struct block_device_operations dms_volume_fops = {
	.owner = THIS_MODULE,
    .unlocked_ioctl = Client_Unlocked_IOCTL,

    .open = DMS_VDisk_Open,
    .release = DMS_VDisk_Release,
    .getgeo = __DMS_getgeo,

};



/********************************************************************************/
/*																				*/
/*								Implementations									*/
/*																				*/
/********************************************************************************/
/*
 * create request queue which is needed by general block device layer.
 */
struct request_queue * __DMS_Create_Request_Queue(struct DMS_Volume *volume){

	int retcode = -EFAULT;

	struct request_queue *r_queue = NULL;

	if(IS_LEGAL(VOLUME_MOD, volume) )	{

		spin_lock_init( &volume->r_queue_lock );

		//init a disk queue and hook DMS_Request_Handler function to IO scheduler
		r_queue = blk_init_queue(DMS_Request_Handler, &volume->r_queue_lock);

		if( likely(r_queue != NULL) ){

			//IO scheduler submit nr_request to sub-system at once.
			r_queue->nr_requests = NR_REQUESTS;

			//set the hardsector_size
			blk_queue_hardsect_size(r_queue, BYTES_PER_DMSBLK);

			//set the max_setors (by KERNEL_SECTOR_SIZE) to control the request block size,
			//it set max_hw_sectors also. there is a special case if the max_sector over 1024.
			blk_queue_max_sectors(r_queue, MAX_KSECTORS);

			//private data.
			r_queue->queuedata = (void *)volume;

			//ref@ LLD CH16: Queue control functions
			//blk_queue_max_phys_segments to say how many segments your driver is prepared to cope with;
			//this may be the size of a staticly allocated scatter list, for example.
			//TODO blk_queue_max_phys_segments(rq, max);

			//is the maximum number of segments that the device itself can handle.
			//Both of these parameters default to 128.
			//TODO blk_queue_max_hw_segments(rq, max);

			//blk_queue_max_segment_size tells the kernel how large any individual
			//segment of a request can be in bytes; the default is 65,536 bytes.
			//TODO blk_queue_max_segment_size(rq, max);

			//support barrier requests
			//TODO blk_queue_ordered(rq, flag);

			//check request barrier flag
			//TODO blk_barrier_rq(struct request *req);

			retcode = DMS_OK;

		#if (ENABLE_TCQ)

			//depth means how many requests you can handle at simultaneously.
			//if alloc fail, it will return err code.
			retcode = blk_queue_init_tags(r_queue, TCQ_DEPTH, NULL);
		#endif

			/* turn on/off congestion indicator */
		//	all_drives[n]->queue->nr_congestion_on = 3;
		//	all_drives[n]->queue->nr_congestion_off = 2;

			DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "init queue done, rq = %p\n", r_queue);

		}else{

			eprintk(VOLUME_MOD, "blk_init_queue fail!\n");
		}

	}

    return r_queue;

create_disk_Q_recovery:

	if(r_queue){
		blk_cleanup_queue(r_queue);
	}

	return NULL;
}

/*
 * create a disk drive and link to sysfs. ex: /dev/hda1
 */
struct gendisk * __DMS_Create_gendisk(struct DMS_Volume *volume, struct request_queue *r_queue, int index){

	struct gendisk *disk = NULL;

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "start~!\n");

	if(IS_LEGAL(VOLUME_MOD, volume) )
	{
		//minors means how many followed number from first_minor this disk can use for partition.
		if ( (disk = alloc_disk(DMS_DISK_MINORS)) != NULL ) {


			DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "alloc disk, done!\n");

			disk->major = DMS_DEV_MAJOR;

			//the actual minor number of this disk.
			disk->first_minor = GET_DMS_MINOR(index);
			disk->fops = &dms_volume_fops;
			disk->queue = r_queue;

			//this parameter need divide by kernel sector size.
			set_capacity(disk, DSECTORS_TO_KSECTORS(volume->dsectors) );
			sprintf( (char *)(&disk->disk_name), "%s%lld", DEVICE_NAME_PREFIX, volume->volumeID );

			//add the DMS volume ptr to private data
			disk->private_data = volume;

			DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "diskname = %s, disk->capacity = %llu, disk->first_minor = %d\n",
					(char *)(&disk->disk_name), get_capacity(disk), disk->first_minor );

		}else{

			eprintk(VOLUME_MOD, "alloc_disk failed! -ENOMEM\n");
		}

	}

	return disk;

}

/*
 * create related device structures in kernel
 */
int __Create_Kernel_Device_Structure(struct DMS_Volume *volume){

	int retcode = -EVMGR_CKS;
//	struct DMS_Volume *volume = NULL;
	struct gendisk *disk = NULL;
	struct request_queue *r_queue = NULL;

	if( IS_LEGAL(VOLUME_MOD, volume) ) {

		//create request queue
		r_queue = __DMS_Create_Request_Queue(volume);

		if( unlikely(!r_queue) ){
			goto create_disk_queue_fail;
		}

		//create gendisk data structure
		disk = __DMS_Create_gendisk(volume, r_queue, volume->minor_index);

		if( unlikely(!disk) ){
			goto create_gendisk_fail;
		}

		//set rq and disk ptr into the dms volume
		volume->r_queue = r_queue;
		volume->disk = disk;

		retcode = DMS_OK;

	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volume = %lld, request queue ptr = %p, disk ptr = %p done\n",
			volume->volumeID, r_queue, volume->disk);

	return retcode;


create_gendisk_fail:

	if(r_queue){
		blk_cleanup_queue(r_queue);
	}

create_disk_queue_fail:

	return -EVMGR_CKS;

}


/*
 * call kernel API to flush all remained request in the disk_queue
 */
void Sync_Disk_Queue(struct request_queue * disk_queue){

	unsigned long flags = 0;

	spin_lock_irqsave(disk_queue->queue_lock, flags);

	//stop disk_queue, it has to get lock before call stop queue.
	blk_stop_queue (disk_queue);

	spin_unlock_irqrestore(disk_queue->queue_lock, flags);

	//sync any remained requests
	blk_sync_queue(disk_queue);

}

/*
 * release related device structures in kernel
 */
int __Release_Kernel_Device_Structure(struct DMS_Volume *volume){

	int retcode = -EVMGR_CKS;


	if(	IS_LEGAL( VOLUME_MOD, volume ) ){

		//stop and sync disk_queue
		Sync_Disk_Queue(volume->r_queue);

		//if this disk has been add to kerenl
		if(volume->disk->flags & GENHD_FL_UP){

			//delete the disk, this operation not always guarantee success. Invalidate partitioning information and perform cleanup
			del_gendisk(volume->disk);

			//release disk structure
			kfree(volume->disk);

			/* Dissociate the driver from the request_queue.
			 * Internally calls	elevator_exit()            */
			blk_cleanup_queue(volume->r_queue);

		} else {

			eprintk(VOLUME_MOD, "this disk has not added to kerenl we can't release it in normal way!\n");

		}

	}


	return retcode;

}


/*
 * init io request statistic data structure
 */
int Init_Volume_Statistic(struct Request_Statistic *rs){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(VOLUME_MOD, rs)){

		//init the linger_io_counter
		atomic_set( &(rs->wreq_counter), 0 );
		atomic_set( &(rs->rreq_counter), 0 );
		atomic_set( &(rs->ovwreq_counter), 0 );

		retcode = DMS_OK;

	}

	return retcode;
}

/*
 * init dms volume data structure
 */
int Init_DMS_Volume(struct DMS_Volume *volume, long64 volumeID, sector_t dsectors){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(VOLUME_MOD, volume)){

		volume->v_owner = NULL;

		volume->volumeID = volumeID;

		volume->replica_factor = 0;

		//init volume state
		volume->volume_state = 0;

		//set index to -1
		volume->minor_index = -1;

		//init open counter to 0
		atomic_set( &(volume->open_counter), 0 );

		//init disk queue lock
		spin_lock_init( &(volume->r_queue_lock) );

		//set request_queue to NULL
		volume->r_queue = NULL;

		//set gendisk ptr to NULL
		volume->disk = NULL;

		//capacity in sectors by hwsec_size
		volume->dsectors = dsectors;

		//init linger io statistic
		Init_Volume_Statistic(&volume->linger_io);

		//init io statistic
		Init_Volume_Statistic(&volume->io_statistic);

		//TODO init volume io hash table (using TCQ)

		//setup vIO handler function
		volume->vIO_Handler = __VolumeIO_Handler;

		volume->vIO_Commiter = __VolumeIO_Commiter;

		volume->vIO_Read_Policy = Default_Read_Policy;

		//volume->vIO_Ack_Policy = NULL;

//		volume->Metadata_Handler = NULL;
//		volume->Metadata_Commit_Handler = NULL;
//		volume->Payload_Handler = NULL;
//		volume->Payload_Ack_Handler = NULL;

		volume->cmt2nn_q = Create_DList();

		retcode = DMS_OK;
	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "volume [%lld] init done~!\n", volumeID);

	return retcode;

}

/*
 * create one dms volume data structure, the caller should check the return ptr
 */
struct DMS_Volume * __Create_Volume(long64 volumeID, sector_t dsectors, int index){

	int retcode = -DMS_FAIL;

	struct DMS_Volume *volume = (struct DMS_Volume *)DMS_Volume_Malloc(sizeof(struct DMS_Volume));

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "allocate volume ptr = %p, for volume = %lld\n", volume, volumeID);

	if( likely(volume) ){

		Init_DMS_Volume(volume, volumeID, dsectors);

		volume->minor_index = index;

		//prepare_kernel_device_structure
		retcode = __Create_Kernel_Device_Structure(volume);

		if( unlikely(retcode < 0) ){

			goto create_fail;

		}

	}

	return volume;


create_fail:

	//recover
	DMS_Volume_Free(volume);

	return NULL;

}

/*
 * release a dms volume
 */
void __Release_Volume(struct DMS_Volume *volume){

	//TODO Flush_Volume_IO, reference count depend

	//TODO __Release_Kernel_Device_Structure();
	__Release_Kernel_Device_Structure(volume);

	//release volume
	//TODO Release_DList_and_User_Data(volume->cmt2nn_q, release_fn);

	return DMS_Volume_Free(volume);
}

//TODO !!add flush IOCTL
#if 0
/*
 * Identify how many remain ios by keeping three counters to tracking remaining read and write requests.
 * if all the three counters are 0, wake up this process and return.
 */
int __Flush_Volume_IO(struct DMS_Volume *volume){

	if(IS_LEGAL(VOLUME_MOD, volume)){

		//if there are remaining IOs
		//TODO while( NUM_LINGER_IO_IN_VOL(volume->statistic) + OPEN_COUNTER_IN_VOL(volume->statistic) )
		while( !IS_ALL_IO_DONE(volume->linger_io) || OPEN_COUNTER_IN_VOL(volume) )
		{
			wprintk("%s%s, volumeID = %lld, NUM_LINGER_IO_IN_VOL() = %d, OPEN_COUNTER_IN_VOL() = %d, waiting to finish. \n",
					VOLUME_MOD, __func__, volume->volumeID, NUM_LINGER_IO_IN_VOL(volume->linger_io), OPEN_COUNTER_IN_VOL(volume) );

			//waiting flush finish
			wait_event_interruptible( volume_wq, IS_ALL_IO_DONE(volume->linger_io) );
		}
	}

	return 0;

}
#endif



/********************************************************************************/
/*																				*/
/*							VOLUME IOCTL RELATED								*/
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
//					VOLUME_MOD, __func__, volume->volumeID, NUM_LINGER_IO_IN_VOL(volume->linger_io), OPEN_COUNTER_IN_VOL(volume) );
//
//			//waiting flush finish
//			wait_event_interruptible( volume_wq, IS_ALL_IO_DONE(volume->linger_io) );
//		}
//	}
//
//	return 0;
//
//}

//int Flush_IO_by_VolumeID(long64 volumeID){
//
//	struct DMS_Volume *volume = Get_Volume_ptr_by_VolumeID(volumeID);
//
//	return Flush_Volume_IO(volume);
//
//}

/*
 * wake up the wait queue
 */
//int Wakeup_Volume_WQ(long64 volumeID){
//
//	if(atomic_read(&nr_detach_cmds) > 0){
//
//		struct DMS_Volume *volume = Get_Volume_ptr_by_VolumeID(volumeID);
//
//		if(check_ptr_validation(VOLUME_MOD, __func__, volume)){
//
//			//if this volume state is detaching, go to wake up the sleeping processes.
//			if(volume->volume_state & DETACHING){
//
//				if (waitqueue_active(&volume_wq) && IS_ALL_IO_DONE(volume->linger_io)) {
//
//					DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "voGER_IO_IN_VOL() = %d, IS_IOREQ_EMPTY_IN_VOL() = %d \n",
//							volume->volumeID, NUM_LINGER_IO_IN_VOL(volume->linger_io), IS_IOREQ_EMPTY_IN_VOL(volume->linger_io) );
//
//					wake_up_interruptible_all(&volume_wq);
//				}
//			}
//		}
//	}
//
//	return DMS_OK;
//}

//TODO
#if 0
/*
 * decrease the linger rw counter
 */
int Decrease_RW_Counter(struct NN_req *nn_req, char *caller){

	int retcode = 0;

	if(check_ptr_validation(FLUSH_MOD, __func__, nn_req) &&
			check_ptr_validation(FLUSH_MOD, __func__, nn_req->req) &&
			check_ptr_validation(FLUSH_MOD, __func__, nn_req->req->drive) )
	{
		DMS_PRINTK(KERN_DEBUG "%s: the caller is: %s, req_id = %llu \n", __func__, caller, nn_req->nnreq_id);

		struct DMS_Volume *drive = nn_req->req->drive;

		switch( nn_req->rw ){

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
int Increase_RW_Counter(struct NN_req *nn_req, char *caller){

	int retcode = 0;

	if(check_ptr_validation(FLUSH_MOD, __func__, nn_req) &&
		check_ptr_validation(FLUSH_MOD, __func__, nn_req->req) &&
		check_ptr_validation(FLUSH_MOD, __func__, nn_req->req->drive) )
	{
		DMS_PRINTK(KERN_DEBUG "%s: the caller is: %s, req_id = %llu \n", __func__, caller, nn_req->nnreq_id);

		struct DMS_Volume *volume = nn_req->req->volume;

		switch( nn_req->rw ){

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
int __Attach_Volume(struct DMS_Volume *volume){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(VOLUME_MOD, volume)){

		//set the voluem state to "attached"
		SET_DS_ATTACHED(volume);

		//add disk to fs system and be functional.
		add_disk(volume->disk);

		retcode = DMS_OK;
	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "done! retcode = %d\n", retcode);

	return retcode;
}


/*
 * preparing to detach a volume, set dms volume status to detaching
 */
int __Detach_Volume_Prep(struct DMS_Volume *volume){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(VOLUME_MOD, volume)){

		//counting flush cmds
		atomic_inc( &nr_detach_cmds );

		//set the volume state to "detaching"
		SET_DS_DETACHING(volume);

		//wait for all linger ios to finish
		//TODO __Flush_Volume_IO(volume);

		//remove dms_volume ptr first to protect another one "open" again
		//Remove_Volume_from_All_Volumes(volume->minor_index);	//Vis marked

		retcode = DMS_OK;
	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "done! retcode = %d\n", retcode);

	return retcode;
}

/*
 * detach a volume, set dms volume status to detach
 */
int __Detach_Volume(struct DMS_Volume *volume){

	int retcode = -DMS_FAIL;

	if(	IS_LEGAL( VOLUME_MOD, volume ) ){

		struct request_queue * r_queue = volume->r_queue;

		//stop and sync disk_queue
		Sync_Disk_Queue(r_queue);

		__Detach_Volume_Prep(volume);

		DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "detach active!! volumeID = %lld, NUM_LINGER_IO = %d, IS_IOREQ_EMPTY = %d \n",
				volume->volumeID, NUM_LINGER_IO_IN_VOL(volume->linger_io), IS_IOREQ_EMPTY_IN_VOL(volume->linger_io) );

		//set the volume state to "not attach"
		SET_DS_NOT_ATTACH(volume);

		atomic_dec( &nr_detach_cmds );

		return DMS_OK;

	}

	DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "done! retcode = %d\n", retcode);

	return retcode;
}



/********************************************************************************/
/*																				*/
/*								GENDISK fops RELATED							*/
/*																				*/
/********************************************************************************/

/**
 * __DMS_getgeo: copy from vbd.c and blkfront.c of xen src
 *
 * @param bd
 * @param hg
 * @return 0: success
 */
int __DMS_getgeo(struct block_device *bd, struct hd_geometry *hg)
{
	/* We don't have real geometry info, but let's at least return
	   values consistent with the size of the device */
	sector_t nsect = get_capacity(bd->bd_disk);
	sector_t cylinders = nsect;

	//255
	hg->heads = 0xff;

	//63
	hg->sectors = 0x3f;

	sector_div(cylinders, hg->heads * hg->sectors);

	hg->cylinders = cylinders;

	printk("the nsect = %llu, cylinder = %llu \n", nsect, cylinders);

	if ((sector_t)(hg->cylinders + 1) * hg->heads * hg->sectors < nsect)
		hg->cylinders = 0xffff;

	return 0;
}

/*
 * open a dms volume, increase open counter
 *
 * find the corresponding volume by minor number
 */
int __Open_Volume(struct DMS_Volume *volume) {

	int retcode = 0;

	if(IS_LEGAL(VOLUME_MOD, volume)){

		//check volume status
		if( !test_bit(DETACHING_BIT, &volume->volume_state) )
		{
			//TODO change to
			//get_disk(volume);
			//increase the counter
			int count = atomic_inc_return( &volume->open_counter );

			retcode = 0;

			DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "[%s] opened volume: %s, count = %d\n", get_current()->comm, volume->disk->disk_name, count);

		}else{

			//open failed
			wprintk(VOLUME_MOD, "The dms volume: %lld is detaching!\n", volume->volumeID);
			retcode = -EFAULT;
		}

	}else{

		//invalidate pointer
		retcode = -EFAULT;
	}

	return retcode;
}

/*
 * close a dms volume, decrease open counter
 *
 * find the corresponding volume by minor number
 */
int __Close_Volume(struct DMS_Volume *volume) {

	int retcode = 0;

	if(IS_LEGAL(VOLUME_MOD, volume)){

		//TODO check volume ID also.
		if(volume){

			//TODO change to
			//put_disk(volume);
			//decrease the counter
			int count = atomic_dec_return( &volume->open_counter );

			retcode = 0;

			DMS_PRINTK(VOLUME_DBG, VOLUME_MOD, "[%s] closed volume: %s, count = %d\n", get_current()->comm, volume->disk->disk_name, count);

		}else{

			//can't find this drive, this shouldn't happen. still return OK for close file.
			retcode = 0;
		}

	}else{

		//can't find this drive, this shouldn't happen. still return OK for close file.
		retcode = 0;
	}

	return retcode;

}



#ifdef DMS_UTEST
EXPORT_SYMBOL(__Attach_Volume);
#endif



/********************************************************************************/
/*																				*/
/*								COMMIT NN Queue									*/
/*																				*/
/********************************************************************************/

int Add_to_Commit_NN_Queue(struct DMS_Volume *volume, struct IO_Request *ior){

	int retcode = 0;

	if(IS_LEGAL(VOLUME_MOD, volume) &&
			IS_LEGAL(VOLUME_MOD, ior) )
	{
		return Insert_User_Data_to_DList_Tail(volume->cmt2nn_q, ior);

	}

}

void Remove_from_Commit_NN_Queue(struct DMS_Volume *volume, struct IO_Request *ior){

	int retcode = 0;

	if(IS_LEGAL(VOLUME_MOD, volume) &&
			IS_LEGAL(VOLUME_MOD, ior) )
	{
		Remove_from_DList_Head(volume->cmt2nn_q, ior);

	}

}
















