/*
 * DMS_Error.h
 *
 *  Created on: 2011/4/1
 *      Author: Vis Lee
 *
 *  READ ME:
 *  remember to add the error log into err_log_Map[] in DMS_Error.c
 */

#ifndef DMS_ERROR_H_
#define DMS_ERROR_H_

#include "asm/errno.h"

/********************************************************************************/
/*																				*/
/*							ERROR CODE BASE Definitions	 						*/
/*																				*/
/********************************************************************************/

//-------------------------defined CONSTANTS code start-------------------------//
#define DMS_OK		0
#define DMS_FAIL	1		//error code convention: return -DMS_FAIL;

#define DMS_READ_LR_ALL_INVALID			2
#define DMS_WRITE_LR_ALL_INVALID		3
#define DMS_WRITE_LR_PARTIAL_INVALID	4

//-------------------------defined CONSTANTS code end---------------------------//




//-------------------------MODULE ERROR CODE BASE start-------------------------//

#define EDMS_BASE		1000
#define EDMS_RANGE		100

/*Block Device Driver Manager errors*/
#define EDEV_ID					1
#define EDEV_BASE				(EDMS_BASE + EDEV_ID * EDMS_RANGE)
#define EDEV_SYS_PENDING		(EDMS_BASE + 1)
#define EDEV_NOP				(EDMS_BASE + 2)
#define EDEV_NOMEM				(EDMS_BASE + 3)

/* Volume Manager errors*/
#define EVMGR_ID				2
#define EVMGR_BASE				(EDMS_BASE + EDEV_ID * EDMS_RANGE)
#define EVMGR_OUT_OF_RANGE		(EVMGR_BASE + 1)
#define EVMGR_DEC_MAX			(EVMGR_BASE + 2)
#define EVMGR_CKS				(EVMGR_BASE + 3)


/*IO request module errors*/
#define EIO_ID			3
#define EIO_BASE		(EDMS_BASE + EIO_ID * EDMS_RANGE)

/*Namenode module errors*/
#define ENN_ID			4
#define ENN_BASE		(EDMS_BASE + ENN_ID * EDMS_RANGE)

/*Datanode module errors*/
#define EDN_ID			5
#define EDN_BASE		(EDMS_BASE + EDN_ID * EDMS_RANGE)
#define EDN_EDNS		(EDN_BASE + 1)

/*DMS Node Container module errors*/
#define EDNC_ID			6
#define EDNC_BASE		(EDMS_BASE + EDNC_ID * EDMS_RANGE)
#define EDNC_CONNECT	(EDNC_BASE + 1)
#define EDNC_FORMAT		(EDNC_BASE + 2)
#define EDNC_RC_OVTIME	(EDNC_BASE + 3)		/*rebuild connection over time (one day)*/
#define EDNC_SNDTIMEO	(EDNC_BASE + 4)
#define EDNC_RECVTIMEO	(EDNC_BASE + 5)





//-------------------------MODULE ERROR CODE BASE end--------------------------//


/********************************************************************************/
/*																				*/
/*								GLOBAL VARIABLE									*/
/*																				*/
/********************************************************************************/
typedef struct DMS_Error_Log_String_Entry{
	int err_id;
	char *logString;
}DMS_Error_Log_String_Entry_t;

extern struct DMS_Error_Log_String_Entry err_log_Map[];

/********************************************************************************/
/*																				*/
/*								API Definitions	 								*/
/*																				*/
/********************************************************************************/

const char *__errorntostr(int errno);



#define PRINT_DMS_ERR_LOG(err_no, mod_n, fmt, args...)						\
do{                                                             \
                                                                \
	int i = 0;                                                  \
                                                                \
	while(err_log_Map[i].err_id != 0){                          \
                                                                \
		if(err_log_Map[i].err_id == err_no){                    \
			eprintk(mod_n, "Error Msg: %s, " fmt, (err_log_Map[i].logString), ##args);        \
			break;                                              \
		}                                                       \
		i++;                                                    \
	}                                                           \
                                                                \
}while(0)                                              			\



#endif /* DMS_ERROR_H_ */
