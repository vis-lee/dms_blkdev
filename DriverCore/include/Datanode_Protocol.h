/*
 * Datanode_Protocol.h
 *
 *  Created on: May 17, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef DATANODE_PROTOCOL_H_
#define DATANODE_PROTOCOL_H_


#include "DMS_Protocol_Header.h"


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

enum DMS_DN_Service_Type {

	/* datanode service */
	DNDO_READ						= DMS_OP_READ,
	DNDO_WRITE						= DMS_OP_WRITE,
	DNDO_PWRITE						= 0x10,
	REREPLICATION					= 0x11,
	HB_INVALIDATION					= 0x12,

};


enum DMS_DN_Sub_Type {

	/* common error code */
	UNDEFINED						= -1000,
	TIMEOUT							= -1001,
	EXCEPTION						= -1002,
	IOEXCEPTION						= -1003,
	IOFAIL_BUT_LUN_OK_EXCEPTION		= -1004,
	FAILED_LUN_EXCEPTION			= -1005,

	PARTIAL_FAIL					= -2001,

	/* normal type */
	SERVICE							= 1,
	RESPONSE						= 2,

	/* DN_ACK type*/
	READ_ACK						= 101,
	WMEM_ACK						= 102,	/* write mem ack */
	WDISK_ACK						= 103,
	PWMEM_ACK						= 104,	/* partial write mem ack */
	PWDISK_ACK						= 105,
	REREPLICATION_ACK				= 106,
	HB_INVALIDATION_ACK				= 107,

};




struct DN_Service_Request {

	short nr_hbids;
	short nr_triplets;
	ulong64 hbids[KSECTORS_TO_DSECTORS(MAX_KSECTORS)];
	ulong64 triplets[KSECTORS_TO_DSECTORS(MAX_KSECTORS)];

};

/* success response */
struct DN_Service_SResponse {

	short nr_hbids;

};

/* fail response */
struct DN_Service_FResponse {

	short nr_triplets;
	ulong64 f_triplets[KSECTORS_TO_DSECTORS(MAX_KSECTORS)];

};


union DN_Protocol_Body {

	struct DN_Service_Request rw_req;
//	struct DN_Service_Request write_req;

	struct DN_Service_SResponse sres;
	struct DN_Service_FResponse fres;
};











/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/








#endif /* DATANODE_PROTOCOL_H_ */
