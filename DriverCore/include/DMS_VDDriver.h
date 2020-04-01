/*
 * DMS_Dev.h
 *
 *  Created on: 2011/7/11
 *      Author: 980263
 */

#ifndef DMS_DEV_H_
#define DMS_DEV_H_


#include "DMS_Common.h"



/********************************************************************************/
/*																				*/
/*						CCMA BLOCK DEVICE DRIVER INFO 							*/
/*																				*/
/********************************************************************************/

#define VERSION_STR          "1.0.1"
#define COPYRIGHT            "Copyright 2010 CCMA/ITRI"
#define DRIVER_AUTHOR        "CCMA Storage Team"
#define DRIVER_DESC          "Block device driver for CCMA Storage System"


/********************************************************************************/
/*																				*/
/*						CCMA BLOCK DEVICE CONSTANT for KERNEL					*/
/*																				*/
/********************************************************************************/


//#define DEVICE_NAME_PREFIX        "dms"

//#define KERNEL_SECTOR_SIZE		512
//#define BYTES_PER_SECTOR     		KERNEL_SECTOR_SIZE





/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/
#define PARAM_PERMISSION 	(S_IRUGO|S_IWUSR) 	//( S_ISUID | S_ISGID | S_IRUSR | S_IRGRP )



/********************************************************************************/
/*																				*/
/*									FUNCS										*/
/*																				*/
/********************************************************************************/
/*
 * DMS BLKDEV IOCTLs. Implementation of regular kernel block device IOCTLs.
 */
int DMS_IOCTL_Handler(struct inode *inode,	/* see include/linux/fs.h */
                 struct file *file,		/* ditto */
                 unsigned int ioctl_num, /* number and param for ioctl */
                 unsigned long ioctl_param);


void DMS_Request_Handler(struct request_queue *r_queue);
int DMS_End_Request_Handler(struct request *kreq, unsigned long commit_size, int result);


/**
 * function: Client_Unlocked_IOCTL()
 * description: DMS Client register an additional character device for providing ioctl for users.
 */
long Client_Unlocked_IOCTL(struct file *filp, unsigned int cmd, unsigned long arg);


int DMS_VDisk_Open(struct inode * inode, struct file* flip);
int DMS_VDisk_Release(struct inode * inode, struct file* flip);


//#ifdef DMS_UTEST
//void Test_Commit_Func(struct request *req, struct request_queue *r_queue);
//void Test_Request_Handler(struct request_queue *r_queue);
//#endif


#endif /* DMS_DEV_H_ */
