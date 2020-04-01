/*
 * DMS_IOCTL.h
 *
 *  Created on: 2011/7/11
 *      Author: Vis Lee
 *
 *  Common header file. Shared between user space daemon & kernel module
 */

#ifndef DMS_IOCTL_H_
#define DMS_IOCTL_H_


#include <linux/types.h>

//#define CMD_TYPE_READ_REQ 2
//#define CMD_TYPE_WRITE_REQ 1
//#define NETLINK_TEST 27
//#define MOD_NUM 590

//#define MAX_CONCURRENT_REQ 30
//#define MAX_NUM_DMSDEV 20
//#define DEV_NAME_LEN 5

//#define MAX_MMAP_UNIT 4096
//#define MMAP_SIZE  MAX_MMAP_UNIT* MAX_CONCURRENT_REQ* MAX_NUM_DMSDEV




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

/* the device name presented under /dev */
#define DMS_DEV_NAME				"dms_vdd"
#define DMS_CONTROLLER_NAME			"dms_vdd_ctrl"

#define DMS_CONTROLLER_MAJOR		151

#define DC_IOCTL_NUM				0xDC

/*
 * DMS Block Device operations
 */
#define IOCTL_RESET_DRIVER			_IOR(DC_IOCTL_NUM, 10, unsigned long)
//#define IOCTL_FORCE_REMOVE 		_IOR(DC_IOCTL_NUM, 11, unsigned long)
#define IOCTL_RELEASE_DRIVER 		_IOR(DC_IOCTL_NUM, 12, unsigned long)

/*
 * DMS Volume operations
 */
#define IOCTL_ATTACH_VOL 			_IOW(DC_IOCTL_NUM, 20, unsigned long)
#define IOCTL_DETATCH_VOL 			_IOW(DC_IOCTL_NUM, 21, unsigned long)
#define IOCTL_FLUSH_VOL 			_IOW(DC_IOCTL_NUM, 22, unsigned long)
#define IOCTL_RESETOVERWRITTEN		_IOW(DC_IOCTL_NUM, 23, unsigned long)
#define IOCTL_INVALIDCACHE			_IOWR(DC_IOCTL_NUM, 24, unsigned long)

/*
 * cmdt
 */
#define IOCTL_TEST_OVERWRITTEN		_IOR(DC_IOCTL_NUM, 90, unsigned long)
//#define IOCTL_CMDT					_IOWR(DC_IOCTL_NUM, 91, unsigned long)


#define RESET_OVERWRITTEN_ACK_OK 		0
#define RESET_OVERWRITTEN_ACK_NOVOLUME 1
#define RESET_OVERWRITTEN_ACK_ERR 		-1

#define INVALIDATE_CACHE_ACK_OK 		0
#define INVALIDATE_CACHE_ACK_NOVOLUME 	1
#define INVALIDATE_CACHE_ACK_ERR 		-1

#define ATTACH_OK 0
#define ATTACH_FAIL EFAULT
#define DETACH_OK 0
#define DETACH_FAIL EFAULT

#define SUBOP_INVALIDATECACHE_BYHBIDS	(0)
#define SUBOP_INVALIDATECACHE_BYVOLID	(1)
#define SUBOP_INVALIDATECACHE_BYDNNAME	(2)





/********************************************************************************/
/*																				*/
/*								DATA STRUCTURES									*/
/*																				*/
/********************************************************************************/

typedef struct {
    unsigned int opcode;
    unsigned int sector;
    unsigned int length;
    pid_t pid;
    int offset;
    int size;
    int reqid;
    int drvid;
//	char drvname[30];
} sector_payload;

struct  ReadReq {
    short CmdType;
    int	PktLen;			// 16 bits,		unit in byte
    int	OpCode;			// 16 bits
    int			ID;				// 32 bits
    int			StartOfst;		// 32 bits, 	unit in byte
    int			PayloadLen;		// 32 bits, 	unit in byte
    int			DevPathLen;		// 32 bits, 	unit in byte
//	char*		DevPath;		// Variable bits
    char*	Payload;
    int	PayloadSize;
    int DevID;
};


struct dms_volume_info {
    long long volid;
    unsigned long long capacity_in_bytes;
    int replica_factor;
    int deviceNum;

};

struct ResetCacheInfo {
    int SubOpCode;
    union {
        struct {
            unsigned long long VolumeID;
            unsigned int NumOfLBID;
            unsigned long long * LBIDs;
        } HBIDs;
        struct {
            unsigned long long VolumeID;
        } VolID;
        struct {
            unsigned int Port;
            unsigned int NameLen;
            char * Name;
        } DNName;
    } u;
};


//typedef struct UnitTest_Param{
//	unsigned int testType;
//	unsigned int threads;
//	unsigned int result;
//}UT_Param_t;


#endif /* DMS_IOCTL_H_ */
