/*
 * DMS_Socket_Protocol_Header.h
 *
 *  Created on: May 21, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef DMS_SOCKET_PROTOCOL_HEADER_H_
#define DMS_SOCKET_PROTOCOL_HEADER_H_

#include "linux/jiffies.h"


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

#define DMS_PROTO_VER								1



#define READ_NET_BUF(nh, rtype, buf, offset)		nh( *((rtype *)(buf+offset)) ); offset+=sizeof(rtype);

#define READ_NET_SHORT(buf, offset)					READ_NET_BUF(ntohs, short, buf, offset)
#define READ_NET_INT(buf, offset)					READ_NET_BUF(ntohl, int, buf, offset)
#define READ_NET_LONG(buf, offset)					READ_NET_BUF(ntohll, long long, buf, offset)


#define WRITE_NET_BUF(val, rtype, buf, offset)		memcpy(buf+offset, (&(val)), sizeof(rtype)); offset+=sizeof(rtype);

#define WRITE_NET_SHORT(val, buf, offset)			(val=htons(val)); WRITE_NET_BUF(val, short, buf, offset)
#define WRITE_NET_INT(val, buf, offset)				(val=htonl(val)); WRITE_NET_BUF(val, int, buf, offset)
#define WRITE_NET_LONG(val, buf, offset)			(val=htonll(val));WRITE_NET_BUF(val, unsigned long long, buf, offset)




/********************************************************************************/
/*																				*/
/*								Definitions	 									*/
/*																				*/
/********************************************************************************/

enum DMS_Types {

	TSENDER				= 0,
	TSERVICE,
	TSUB,

};


enum DMS_Sender_Type {

	NAMENODE 			= 0x09,
	CLIENTNODE			= 0x0c,
	DATANODE 			= 0x0d,

};



struct DMS_Protocol_Header {

	int 	magicNumber;	//4 bytes
	char 	version;		//1 bytes
	char 	retry;			//1 bytes
	char 	sender_type;	//1 bytes
	char 	reserve;		//1 bytes
	short 	service_type;	//2 bytes
	short 	sub_type;		//2 bytes
	int 	body_length; 	//4 bytes
	ulong64	serviceID;		//8 bytes
	ulong64 time_stamp;		//8 bytes
	ulong64 extention;		//8 bytes, total 40 bytes

	//union NN_Protocol_Body body;

}__attribute__ ((__packed__));



/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/



///** Convert to milliseconds */
//static inline __u64 __tv_to_ms(const struct timeval *tv)
//{
//	__u64 ms = tv->tv_usec / 1000;
//	ms += (__u64) tv->tv_sec * (__u64) 1000;
//	return ms;
//}

/** Convert to micro-seconds */
static inline __u64 __tv_to_us(const struct timeval *tv)
{
	__u64 us = tv->tv_usec;
	us += (__u64) tv->tv_sec * (__u64) 1000000;
	return us;
}

//static inline __u64 Get_Current_Ms(void)
//{
//	struct timeval tv;
//	do_gettimeofday(&tv);
//	return __tv_to_ms(&tv);
//}

static inline __u64 Get_Current_Us(void)
{
	struct timeval tv;
	do_gettimeofday(&tv);
	return __tv_to_us(&tv);
}




static inline int Parse_DMS_Protocol_Header(struct DMS_Protocol_Header *header){


	header->magicNumber 	= ntohl(header->magicNumber);
	header->version			= (header->version);
	header->retry			= (header->retry);
	header->sender_type 	= (header->sender_type);
	header->reserve			= (header->reserve);
	header->service_type	= ntohs(header->service_type);
	header->sub_type		= ntohs(header->sub_type);
	header->body_length		= ntohl(header->body_length);
	header->serviceID		= ntohll(header->serviceID);
	header->time_stamp		= htonll(header->time_stamp);
	header->extention		= htonll(header->extention);

	return sizeof(*header);

}




/**
 * Generate_NN_Protocol_Header
 *
 * @param header
 * @param version
 * @param service_type
 * @param length
 * @return	>0 :	OK, the size of header
 * 			-1:		NULL ptr
 */
static inline int Generate_DMS_Protocol_Header(struct DMS_Protocol_Header *header, char retry, short service_type,
													short sub_type, int body_length, ulong64 pdata, ulong64 sid){

	memset(header, 0, sizeof(struct DMS_Protocol_Header));

	header->magicNumber		= htonl(MAGIC_NUM);
	header->version			= (DMS_PROTO_VER);
	header->retry			= (retry);
	header->sender_type 	= (CLIENTNODE);
	header->reserve 		= 0;
	header->service_type	= htons(service_type);
	header->sub_type		= htons(sub_type);
	header->body_length		= htonl(body_length);
	header->serviceID		= htonll( sid );
	header->time_stamp		= htonll( Get_Current_Us() ); //TODO fsm.timestamp?
	header->extention		= htonll(pdata);

	return sizeof(struct DMS_Protocol_Header);

}


static inline char * __TSender_ntoc(int type){

	switch(type){

		/*
		 * DMS_Sender_Type
		 */
		case CLIENTNODE:
			return "CLIENTNODE";
		case DATANODE:
			return "DATANODE";
		case NAMENODE:
			return "NAMENODE";

		default:
			return "unknow sender type";
	}
}



#endif /* DMS_SOCKET_PROTOCOL_HEADER_H_ */
