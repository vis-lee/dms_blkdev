/*
 * DMS_Common.h
 *
 *  Created on: 2011/7/11
 *      Author: Vis Lee
 *      		Lego
 *
 *  put the headers here if the header is used everywhere.
 *  put the definition or macro here if the they are used everywhere.
 *
 */

#ifndef DMS_COMMON_H_
#define DMS_COMMON_H_

#include <asm/atomic.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>
#include <linux/spinlock.h>
#include <linux/list.h>


#include "DMS_Type.h"
#include "DMS_Debug.h"
#include "DMS_Error.h"
#include "DMS_Mem_Pool.h"

/********************************************************************************/
/*																				*/
/*								DMS VDD CONSTANTS								*/
/*																				*/
/********************************************************************************/

#define DMS_IP_STR_LEN		24 /* 16bytes for ip, 6 for port number, and 1 for \0, 1 for reserve*/

#define MAGIC_NUM 			0xa5a5a5a5		/*0xa5a5a5a5a5a5a5a5ll*/

//----op code ----
#define DMS_OP_READ  		READ
#define DMS_OP_WRITE 		WRITE
#define DMS_OP_OVERWRITE	2

extern const char *r_str;
extern const char *w_str;
extern const char *ovw_str;


typedef enum {
	INIT,		/* driver init*/
	RUNNING,	/* driver running*/
	PENDING,	/* driver pending*/
	SERVICING,	/* DMS service opening*/
	SHUTDOWN,	/* driver unloading*/
} VDD_STATE;

extern int g_dms_state;



#define MAX_RETRY_COUNT				3


/********************************************************************************/
/*																				*/
/*							DMS VDD CONSTANTS for KERNEL						*/
/*																				*/
/********************************************************************************/


//#define DEVICE_NAME_PREFIX         "dms"

#define KERNEL_SECTOR_SIZE			512
#define BYTES_PER_SECTOR     		KERNEL_SECTOR_SIZE
//TODO someone tried 100000, it's increase performance rapidly.
#define MAX_KSECTORS 				128

#define NR_REQUESTS 				BLKDEV_MAX_RQ


#define DMS_DRIVER_STATE_WORKING_BIT	0

#define DMS_DRIVER_STATE_WORKING		( 1<<DMS_DRIVER_STATE_WORKING_BIT )


/********************************************************************************/
/*																				*/
/*								DMS LB CONSTANTS								*/
/*																				*/
/********************************************************************************/

/**
 * never set DMSBLK less than 4096, or you may encounter disaster.
 */
#define BYTES_PER_DMSBLK			4096LL
#define DMSBLK_SHIFT_BITS			12
#define DMSBLK_MASK					( ~(BYTES_PER_DMSBLK-1) )
#define DMSBLK_ALIGN(nr_bytes)		( (nr_bytes + (BYTES_PER_DMSBLK-1)) & DMSBLK_MASK )

#define CAL_NR_DMSBLKS(nr_bytes)	( DMSBLK_ALIGN(nr_bytes) >> DMSBLK_SHIFT_BITS)


//ex: ((kerenl_sectors * 512) / 4096)
#define KSECTORS_TO_DSECTORS(ksect)		((ksect << 9) >> DMSBLK_SHIFT_BITS)

//ex: ((dms_sectors * 4096) / 512)
#define DSECTORS_TO_KSECTORS(dsect)		((dsect << DMSBLK_SHIFT_BITS) >> 9)

#define KSECTORS_TO_LBID(ksect)			KSECTORS_TO_DSECTORS(ksect)


/********************************************************************************/
/*																				*/
/*								MODULE NAMES									*/
/*																				*/
/********************************************************************************/

//TODO move to makefile
#define DMS_UTEST

#define ENABLE_TCQ		1

#if (ENABLE_TCQ)
	#define TCQ_DEPTH	64
#endif

//
///* Namenode Queue Module */
//static char *NN_QUEUE_MOD = 		"DN_Q: ";
///* Namenode Request Handler*/
//static char *NN_QUEUE_WORKER =		"DN_WORKER: ";
//
///* Datanode Queue Module */
//static char *DN_QUEUE_MOD = 		"DN_Q: ";
///* Datanode Request Handler*/
//static char *DN_QUEUE_WORKER =		"DN_WORKER: ";
//
///* Connection Manager Module */
//static char *CP_MOD =				"CONNECTION_POOL: ";
///* Connection Retry Handler*/
//static char *CR_MOD =				"CONNECTION_RETRY: ";
//
///* Performance Measuring */
//static char *PM_MOD =				"Performance_Measuring: ";



/********************************************************************************/
/*																				*/
/*								PRINTK FUNCS									*/
/*																				*/
/********************************************************************************/

#define PFX "DMS_VDD: "

#if (DMS_DEBUG_FLAG == 0)
	#define cprintk(mod_n, fmt...) printk(KERN_CRIT PFX fmt)
	#define iprintk(mod_n, fmt...) printk(KERN_INFO PFX fmt)
	#define wprintk(mod_n, fmt...) printk(KERN_WARNING PFX fmt)
	#define eprintk(mod_n, fmt...) printk(KERN_ERR PFX fmt)
	#define dprintk(mod_n, fmt...) printk(KERN_DEBUG PFX fmt)
#elif(DMS_DEBUG_FLAG == 1)
	#define cprintk(mod_n, fmt, args...) printk(KERN_CRIT PFX "%s%s, " fmt, mod_n, __func__, ##args)
	#define iprintk(mod_n, fmt, args...) printk(KERN_INFO PFX "%s%s, " fmt, mod_n, __func__, ##args)
	#define wprintk(mod_n, fmt, args...) printk(KERN_WARNING PFX "%s%s, " fmt, mod_n, __func__, ##args)
	#define eprintk(mod_n, fmt, args...) printk(KERN_ERR PFX "%s%s, " fmt, mod_n, __func__, ##args)
	#define dprintk(mod_n, fmt, args...) printk(KERN_DEBUG PFX "%s%s, " fmt, mod_n, __func__, ##args)
#endif


/********************************************************************************/
/*																				*/
/*							USEFUL COMMON MACRO 								*/
/*																				*/
/********************************************************************************/


/* IS_IN_RANGE identify whether the val locates within start to end or not. */
#define IS_IN_RANGE(val, start, end)			( val >= start && val <= end )

/* IS_IN_LENGTH_RANGE identify whether the val locates within start plus given interval or not. */
#define IS_IN_LENGTH_RANGE(val, start, len)		( IS_IN_RANGE(val, start, (start + len - 1)) )

#define DMS_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) \
							 + sizeof(typeof(int[1 - 2*!!__builtin_types_compatible_p(typeof(arr), \
							 typeof(&arr[0]))]))*0)


#define DSTR_LIMIT				(PAGE_SIZE-256)
#define DSTR_MALLOC(name)		char *name = (char *)kmalloc(PAGE_SIZE, GFP_KERNEL)
#define DSTR_FREE(name)			kfree(name);

#define DSTR_NAME		str
#define DSTR_PRINT(flag, mod_name, sp_func, fmt, args...) 	\
		do{							\
			if(flag){				\
				DSTR_MALLOC(DSTR_NAME);	\
				sp_func;				\
				DMS_PRINTK(flag, mod_name, fmt"%s", ##args, DSTR_NAME);	\
				DSTR_FREE(DSTR_NAME);	\
			}						\
		}while(0)



#define IS_OVER_TIME(wait_time)		time_after(jiffies, wait_time)
#define GET_WAIT_TIME(secs)			(jiffies+secs*HZ)




#define htonll(x) \
((((x) & 0xff00000000000000LL) >> 56) | \
(((x) & 0x00ff000000000000LL) >> 40) | \
(((x) & 0x0000ff0000000000LL) >> 24) | \
(((x) & 0x000000ff00000000LL) >> 8) | \
(((x) & 0x00000000ff000000LL) << 8) | \
(((x) & 0x0000000000ff0000LL) << 24) | \
(((x) & 0x000000000000ff00LL) << 40) | \
(((x) & 0x00000000000000ffLL) << 56))

#define ntohll(x)	htonll(x)



/*
 * simple socket
 */
struct SSocket{

	int ip;
	unsigned short port;

};

#define Compare_SSocket(a, b)		( (a->ip == b->ip) && (a->port == b->port) )
#define Copy_SSocket(dst, src)		( memcpy(dst, src, sizeof(struct SSocket)) )

/********************************************************************************/
/*																				*/
/*								COMMON FUNCS									*/
/*																				*/
/********************************************************************************/

#define CHECK_PTR(modname, ptr)	likely( ptr != NULL && !IS_ERR(ptr) )


#define IS_LEGAL(modname, ptr)	likely(check_ptr_validation_FL(modname, __func__, __LINE__, ptr))
/*
 * poninter validation function, check NULL and IS_ERR.
 * @param: char *modname				your module name, easy to find your message
 * @param: const char *func_name		function name, you can get by __func__
 * @param: const void *ptr				the ptr you want to check
 * @return: true, if the ptr is validation, false otherwise.
 */
static inline int check_ptr_validation(char *modname, const char *func_name, const void *ptr)
{
	if(ptr == NULL || IS_ERR(ptr)){

		printk("%s: %s, FATAL ERROR!! ptr is invalidated, pointer = %p \n", modname, func_name, ptr);

		return false;

	}else{

		return true;

	}
}

static inline int check_ptr_validation_FL(char *modname, const char *func_name, int line_num,const void *ptr)
{
	if(ptr == NULL || IS_ERR(ptr)){

		printk("%s: %s, func: %s, line: %d, FATAL ERROR!! pointer = %p \n", PFX, modname, func_name, line_num, ptr);

		return false;

	}else{

		return true;

	}
}



static inline const char * op2str(int op){

	switch(op){

		case DMS_OP_READ:
			return r_str;
		case DMS_OP_WRITE:
			return w_str;
		case DMS_OP_OVERWRITE:
			return ovw_str;
	}

	return "unknown";
}





/********************************************************************************/
/*																				*/
/*							Linux kernel version control						*/
/*																				*/
/********************************************************************************/

#if 0
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 36)
  #define LATEST_LINUX_KERNEL
#endif

#ifdef LATEST_LINUX_KERNEL

#define dms_peek_request(q) blk_peek_request(q)
#define dms_fetch_request(q) blk_fetch_request(q)
#define dms_fs_request(req) (req->cmd_type == REQ_TYPE_FS)
#define dms_end_request(req, status, nbytes)\
__blk_end_request(req, status, nbytes)
#define dms_end_request_all(req, status)\
__blk_end_request_all(req, status)
#define dms_rq_pos(req) blk_rq_pos(req)

int dms_block_device_open(struct block_device *bd, fmode_t mode) {
  struct dms_device *device =
    (struct dms_device*)bd->bd_disk->private_data;
  if (device->reference_read > 0) // when there is reader failed the new writer request
    if (mode & FMODE_WRITE) return -1;
  if (device->reference_write > 0) return -1; // when there is writer fail the new open request
  device->reference++;
  if (mode & FMODE_WRITE)
    device->reference_write++;
  if (mode & FMODE_READ)
    device->reference_read++;
  dms_request_rc_changed
    (device->pid, ++device->sequence, device->reference,
     device->reference_read, device->reference_write);
  return 0;
}

int dms_block_device_release(struct gendisk *gd, fmode_t mode) {
  struct dms_device *device = (struct dms_device*)gd->private_data;
  device->reference--;
  if (mode & FMODE_WRITE)
    device->reference_write--;
  if (mode & FMODE_READ)
    device->reference_read--;
  dms_request_rc_changed
    (device->pid, ++device->sequence, device->reference,
     device->reference_read, device->reference_write);
  return 0;
}

void add_disk_worker(struct work_struct *work) {
  struct dms_device* device = (struct dms_device*)((worker_t*)work)->data;
  add_disk(device->gd);
  device->attached = true;
}

void request_worker(struct work_struct *work) {
  struct dms_device* device = (struct dms_device*)((worker_t*)work)->data;
  dms_request_io
    (device->pid, ++device->sequence, device->cmd,
     device->offset, device->nbytes);
}

#else

#define OLD_STATUS(status) ((status>=0)?1:status)
#define dms_peek_request(q) elv_next_request(q)
#define dms_fs_request(req) blk_fs_request(req)
#define dms_end_request(req, status, nbytes)\
  (!end_that_request_first(req, OLD_STATUS(status), nbytes/KERNEL_SECTOR_SIZE)?\
    end_that_request_last(req, OLD_STATUS(status)), 0:-1)
#define dms_end_request_all(req, status)\
  end_request(req, OLD_STATUS(status))
#define dms_rq_pos(req) (req->sector)

static struct request* dms_fetch_request(request_queue_t *q) {
  struct request *req = elv_next_request(q);
  if (req) blkdev_dequeue_request(req);
  return req;
}

struct bdev_inode {
  struct block_device bdev;
  struct inode vfs_inode;
};

static inline struct bdev_inode *BDEV_I(struct inode *inode)
{ return container_of(inode, struct bdev_inode, vfs_inode);}


int dms_block_device_open(struct inode *inode, struct file *file) {
  struct block_device *bd = &BDEV_I(inode)->bdev;
  struct dms_device *device =
    (struct dms_device*)bd->bd_disk->private_data;
  if (device->reference_read > 0) // when there is reader failed the new writer request
    if (file->f_mode & FMODE_WRITE) return -1;
  if (device->reference_write > 0) return -1; // when there is writer fail the new open request
  device->reference++;
  if (file->f_mode & FMODE_WRITE)
    device->reference_write++;
  if (file->f_mode & FMODE_READ)
    device->reference_read++;
  dms_request_rc_changed
    (device->pid, ++device->sequence, device->reference,
     device->reference_read, device->reference_write);
  return 0;
}

int dms_block_device_release(struct inode *inode, struct file *file) {
  struct block_device *bd = &BDEV_I(inode)->bdev;
  struct dms_device *device =
    (struct dms_device*)bd->bd_disk->private_data;
  device->reference--;
  if (file->f_mode & FMODE_WRITE)
    device->reference_write--;
  if (file->f_mode & FMODE_READ)
    device->reference_read--;
  dms_request_rc_changed
    (device->pid, ++device->sequence, device->reference,
     device->reference_read, device->reference_write);
  return 0;
}

void add_disk_worker(void *data) {
  struct dms_device* device = (struct dms_device*)data;
  PDEBUG("enter\n");
  add_disk(device->gd);
  device->attached = true;
  PDEBUG("exit\n");
}

void request_worker(void *data) {
  struct dms_device* device = (struct dms_device*)data;
  PDEBUG("enter\n");
  dms_request_io
    (device->pid, ++device->sequence, device->cmd,
     device->offset, device->nbytes);
  PDEBUG("exit\n");
}

#endif
#endif //if 0

#endif /* DMS_COMMON_H_ */
