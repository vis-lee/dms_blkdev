/*
 * DMS_IOCTL.c
 *
 *  Created on: 2011/7/13
 *      Author: Vis Lee
 *      		Lego Lin
 *
 *  handle IOCTL requests
 */

#include <linux/kernel.h>
#include "DMS_IOCTL.h"

//
//
///*
// * attach volume ioctl
// */
//int IOCTL_AttachVolume(void __user *ioctl_param){
//
//	int retcode = -ATTACH_FAIL;
//	int index = -1;
//	struct dms_volume_info vinfo = {0};
//	struct ccma_drive *volume = NULL;
//
//	copy_from_user(&vinfo, ioctl_param, sizeof(struct dms_volume_info));
//
//	printk(KERN_INFO "%s%s, capacity=%llu volid=%d\n", IOCTL_MOD, __func__, vinfo.capacity, vinfo.volid);
//
//	index = Find_CCMA_Drive_Index_by_VolumeID(vinfo.volid);
//
//	//check whether this volume is attached or not.
//	if( !IS_IN_RANGE(index, CCMA_MINOR_START, max_num_volumes-1) ){
//
//		//create disk drive
//		volume = dynamic_create_drive(&vinfo);
//
//		if( volume != NULL ) {
//
//			atomic_set(&volume->q_running, 1);
//
//			//add disk to fs system and be functional.
//			add_disk(volume->disk);
//
//	#ifdef VOLUME_WAIT_Q
//
//			//set the voluem state to "attached"
//			SET_DS_ATTACHED(volume);
//	#endif
//
//			retcode = ATTACH_OK;
//
//			printk(KERN_INFO "%s%s, attach vol DONE, id=%d\n", IOCTL_MOD, __func__, vinfo.volid);
//
//		} else {
//
//			printk("%s%s, there is no available drive\n", IOCTL_MOD, __func__);
//			retcode = -ATTACH_FAIL;
//
//		}
//
//	}else{
//
//		printk("%s%s, This volume has been attached!! Don't attach it again!!\n", IOCTL_MOD, __func__);
//		retcode = -EEXIST;
//	}
//
//	printk(KERN_INFO "%s%s, attach vol END, id=%d\n", IOCTL_MOD, __func__, vinfo.volid);
//
//	return retcode;
//}
//
///*
// * call kernel API to flush all remained request in the disk_queue
// */
//void Sync_Disk_Queue(struct request_queue * disk_queue){
//
//	unsigned long flags = 0;
//
//	spin_lock_irqsave(disk_queue->queue_lock, flags);
//	//stop disk_queue, it has to get lock before call stop queue.
//	blk_stop_queue (disk_queue);
//
//	spin_unlock_irqrestore(disk_queue->queue_lock, flags);
//
//	//sync any remained requests
//	blk_sync_queue(disk_queue);
//
//}
//
///*
// * detach volume ioctl
// */
//int IOCTL_DetachVolume(long ioctl_param){
//
//	long volid = ioctl_param;
//	int index = -1;
//
//	printk(KERN_INFO "%s%s, driver detach vol called, ready to detach volume id %ld\n",
//			IOCTL_MOD, __func__, volid);
//
//	index = Find_CCMA_Drive_Index_by_VolumeID(volid);
//
//	/* No such device */
//	if( !IS_IN_RANGE(index, CCMA_MINOR_START, (max_num_volumes - 1)) ){
//
//		printk(KERN_WARNING "%s%s, no such volume!", IOCTL_MOD, __func__);
//		return -ENODEV;
//	}
//	/* Device or resource busy */
//	if( !IS_OPEN_COUNTER_EMPTY_IN_VOL(all_drives[index]->statistic) ){
//
//		printk(KERN_WARNING "%s%s, volume is busy!", IOCTL_MOD, __func__);
//		return -EBUSY;
//	}
//
//	if(	check_ptr_validation( IOCTL_MOD, __func__, all_drives[index] ) ){
//
//		struct ccma_drive *volume = all_drives[index];
//		struct request_queue * disk_queue = volume->queue;
//
//		//counting flush cmds
//		atomic_inc( &Num_Detach_cmds );
//
//		//set the volume state to "detaching"
//		SET_DS_DETACHING(volume);
//
//		//stop and sync disk_queue
//		Sync_Disk_Queue(disk_queue);
//
//		//wait for all linger ios to finish
//		Flush_IO_in_Volume(volid);
//
//		//remove ccma_drive ptr first to protect another one "open" again
//		Remove_Drive_from_CCMA_Drives(index);
//
//		printk(KERN_DEBUG "%s%s, detach active!! volID = %d, NUM_LINGER_IO_IN_VOL() = %d, IS_IOREQ_EMPTY_IN_VOL() = %d \n",
//				FLUSH_MOD, __func__, volume->volinfo.volid, NUM_LINGER_IO_IN_VOL(volume->statistic), IS_IOREQ_EMPTY_IN_VOL(volume->statistic) );
//
//		//set the volume state to "not attach"
//		SET_DS_NO_ATTACH(volume);
//
//		release_overwritten(volume);
//
//		//delete the disk, this operation not always guarantee success. Invalidate partitioning information and perform cleanup
//		del_gendisk(volume->disk);
//
//		//put back to kernel
//		put_disk(volume->disk);
//
//		/* Dissociate the driver from the request_queue. Internally calls
//		elevator_exit() */
//		blk_cleanup_queue(disk_queue);
//
//		kfree(volume);
//
//		atomic_dec( &Num_Detach_cmds );
//
//		return DETACH_OK;
//
//	}else{
//
//		printk(KERN_INFO "%s%s, volume ptr is NULL, volume id = %ld, index = %d, ",
//				IOCTL_MOD, __func__, volid, index );
//
//		return -DETACH_FAIL;
//	}
//
//}

