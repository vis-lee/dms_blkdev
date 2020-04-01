/*
 * Namenode_Protocol.h
 *
 *  Created on: Mar 29, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef NAMENODE_PROTOCOL_H_
#define NAMENODE_PROTOCOL_H_

#include "DMS_Common.h"
#include "DMS_Protocol_Header.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/


//TODO low priority, prevent ref after free or double free.
#define NN_MAGIC_NUM		0x770a165e


/********************************************************************************/
/*																				*/
/*								Definitions	 									*/
/*																				*/
/********************************************************************************/

#define DEFAULT_NN_REQ_WAIT_TIME	10 //sec


enum DMS_NN_Service_Type {

	/* namenode service */
	QUERY_FOR_READ 					= DMS_OP_READ,
	QUERY_FOR_WRITE 				= DMS_OP_WRITE,
	QUERY_FOR_OVERWRITE 			= DMS_OP_OVERWRITE,
	COMMIT_ALLOCATION				= 3,
	REPORT_READ_FAILURE				= 4,
	COMMIT_METADATA					= 5,
	QUERY_VOLUME_INFO 				= 6,
	CREATE_VOLUME 					= 7,


};


enum DMS_NN_Sub_Type {

	/* common error code */
	UNDEFINED						= -1000,
	TIMEOUT							= -1001,
	EXCEPTION						= -1002,
	IOEXCEPTION						= -1003,


	/* normal type */
	SERVICE							= 1,
	RESPONSE						= 2,

};






/********************************************************************************/
/*																				*/
/*								Query Series 									*/
/*																				*/
/********************************************************************************/
#if 0//def CREATE_VOLUME_IOCTL
struct Create_Volume_Request{

	ulong64 capacity;
	int volume_attribute;
	int replica_factor;

}__attribute__ ((__packed__));

struct Create_Volume_Response{

	long64 volumeID;
	ulong64 capacity;
	int volume_attribute;
	int replica_factor;

}__attribute__ ((__packed__));
#endif



/*
 * Query Metadata
 */
struct Query_Metadata_Request{

	long64 volumeID;
	ulong64 start_LBID;
	short nr_LBIDs;

}__attribute__ ((__packed__));


struct Query_Metadata_Response{

	long64 volumeID;
	ulong64 start_LBID;
	short nr_LBIDs;
	ulong64 commit_ID;

	int nr_lrs;
	struct Located_Request **lrs;

}__attribute__ ((__packed__));



/*
 * Qeury VOLUME INFO
 */
struct Query_Volume_Info_Request{

	long64 volumeID;

}__attribute__ ((__packed__));


struct Query_Volume_Info_Response{

	long64 volumeID;
	ulong64 capacity;
	int volume_attributes;
	int replica_factor;

}__attribute__ ((__packed__));



/********************************************************************************/
/*																				*/
/*								Commmit Series 									*/
/*																				*/
/********************************************************************************/





/********************************************************************************/
/*																				*/
/*								Header and Body 								*/
/*																				*/
/********************************************************************************/


/*
 * Error msg
 */
struct DMS_Error_Msg {

	int error_code;
	char *msg;

}__attribute__ ((__packed__));



union NN_Protocol_Body{

#if 0//def CREATE_VOLUME_IOCTL
	struct Create_Volume_Request cvreq;
	struct Create_Volume_Response cr_res;
#endif

	struct Query_Metadata_Request qmeta_req;
	struct Query_Metadata_Response qmeta_res;

	struct Query_Volume_Info_Request qvinfo_req;
	struct Query_Volume_Info_Response qvinfo_res;

	struct DMS_Error_Msg emsg;

};



/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/


int Generate_NN_Protocol_Header(struct DMS_Protocol_Header *header, char retry, short service_type, int length, ulong64 pdata);
int Generate_NN_Protocol_Body(union NN_Protocol_Body *body, long64 volumeID, ulong64 startLBID, ulong64 nr_LBIDs);


int Parse_NN_Protocol_Header(struct DMS_Protocol_Header *header);
int Parse_NN_Protocol_Body(struct DMS_Protocol_Header *header, char *buf, union NN_Protocol_Body *body);
int Parse_Located_Request(struct Located_Request *lr, char *buf, ulong64 slbid);



//int SPrint_NN_Protocol_Header(char *buf, int len_limit, struct DMS_Protocol_Header *header);



#endif /* NAMENODE_PROTOCOL_H_ */
