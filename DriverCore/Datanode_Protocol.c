/*
 * Datanode_Protocol.c
 *
 *  Created on: May 17, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */




/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *DNP_MOD = "DNP: ";

static atomic64_t dnph_id_gen = ATOMIC64_INIT(1);



/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/





/********************************************************************************/
/*																				*/
/*							FUNCS Implementations								*/
/*																				*/
/********************************************************************************/

char * __DN_Type_ntoc(int ttype, int type) {

	switch(ttype){

		case TSENDER:
		{

			return __TSender_ntoc(type);

		}

		case TSERVICE:
		{
			switch(type){

				/*
				 * DMS_Service_Type
				 */
				case DNDO_READ:
					return "DNDO_READ";
				case DNDO_WRITE:
					return "DNDO_WRITE";
				case DNDO_PWRITE:
					return "DNDO_PWRITE";
				case REREPLICATION:
					return "REREPLICATION";
				case HB_INVALIDATION:
					return "HB_INVALIDATION";

				default:
					return "unknow service type";
			}

			break;
		}

		case TSUB:
		{
			switch(type){

				/*
				 * DMS_Sub_Type
				 */
				case UNDEFINED:
					return "UNDEFINED";
				case TIMEOUT:
					return "TIMEOUT";
				case EXCEPTION:
					return "EXCEPTION";
				case IOEXCEPTION:
					return "IOEXCEPTION";
				case IOFAIL_BUT_LUN_OK_EXCEPTION:
					return "IOFAIL_BUT_LUN_OK_EXCEPTION";
				case FAILED_LUN_EXCEPTION:
					return "FAILED_LUN_EXCEPTION";

				case SERVICE:
					return "SERVICE";
				case RESPONSE:
					return "RESPONSE";

				case READ_ACK:
					return "READ_ACK";
				case WMEM_ACK:
					return "WMEM_ACK";
				case WDISK_ACK:
					return "WDISK_ACK";
				case PWMEM_ACK:
					return "PWMEM_ACK";
				case PWDISK_ACK:
					return "PWDISK_ACK";
				case REREPLICATION_ACK:
					return "REREPLICATION_ACK";
				case HB_INVALIDATION_ACK:
					return "HB_INVALIDATION_ACK";

				default:
					return "unknow sub type";

			}

			break;
		}

		default:
			return "unknow dms type";
	}


};

















int Parse_DN_Service_SResponse(char *buf, struct DN_Service_SResponse *sresp){

	int len = 0;

	if(IS_LEGAL(PAM_MOD, sresp)){

		//translate to host byte order
		sresp->nr_hbids = READ_NET_SHORT(buf, len);
	}

	return len;
}

#if 0
inline int Parse_DN_Response_Body(struct DMS_Protocol_Header *header, union DN_Protocol_Body *body){

	int retcode = -DMS_FAIL;

	if(IS_LEGAL(DNP_MOD, header) &&
			IS_LEGAL(DNP_MOD, body) )
	{

		switch(header->sub_type){

			case READ_ACK:
				//read body
				//TODO call "recv vect" from payload mgr
				break;

			case WMEM_ACK:
			case PWMEM_ACK:
				break;

			case WDISK_ACK:
			case PWDISK_ACK:
				break;

			case IOEXCEPTION:
			case IOFAIL_BUT_LUN_OK_EXCEPTION:
			case FAILED_LUN_EXCEPTION:
				//TODO set DNLocation fail

			case PARTIAL_FAIL:
				//TODO
				break;

			default:
				eprintk(PAM_MOD, "unknown ack type = %d,  \n", ack);
		}

		retcode = DMS_OK;
	}

	return retcode;
}
#endif


int __Pack_HBIDs(ulong64 *buf, short nr_hbids, ulong64 *hbids){

	int i = 0;
	int len = 0;

	for( i = 0; i < nr_hbids; i++){

		buf[i] = htonll(hbids[i]);

	}

	len += sizeof(ulong64) * nr_hbids;

	return len;
}


int __Pack_Triplets(ulong64 *buf, short nr_triplets, ulong64 *triplets){

	int i = 0;
	int len = 0;

	for( i = 0; i < nr_triplets; i++){

		buf[i] = htonll(triplets[i]);

	}

	len += sizeof(ulong64) * nr_triplets;

	return len;
}


inline int Generate_DN_Request_Body(struct DN_Service_Request *sreq, union DN_Protocol_Body *body){

	int len = 0; /* record buf size in bytes */

	if( IS_LEGAL(DNP_MOD, body)){

		ulong64 *buf = NULL;

		memset(body, 0, sizeof(union DN_Protocol_Body));

		body->rw_req.nr_hbids = htons(sreq->nr_hbids);
		body->rw_req.nr_triplets = htons(sreq->nr_triplets);

		len += sizeof(body->rw_req.nr_hbids) + sizeof(body->rw_req.nr_triplets);

		/*
		 *  Pack hbids and triplets
		 */
		buf = &body->rw_req.hbids;

		len += __Pack_HBIDs(buf, sreq->nr_hbids, &sreq->hbids);

		//go forward by ulong64 base
		buf += sreq->nr_hbids;

		len += __Pack_Triplets(buf, sreq->nr_triplets, &sreq->triplets);

	}

	return len;
}






/**
 * Parse_DN_Protocol_Header
 *
 * @param header
 * @return	>0:		OK, the size of header
 * 			-1:		magic number mismatch
 */
int Parse_DN_Protocol_Header(struct DMS_Protocol_Header *header){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(DNP_MOD, header)){

		retcode = Parse_DMS_Protocol_Header(header);

		if( header->magicNumber != MAGIC_NUM ){

			retcode = -DMS_FAIL;
			DMS_PRINTK(PALO_DBG, DNP_MOD, "FATAL ERROR~! magic_number miss-match = %x \n", header->magicNumber);

		}
	}

	DMS_PRINTK(PALO_DBG, DNP_MOD, "end~!, retcode = %d \n", retcode);

	return retcode;

}




/**
 * Generate_DN_Protocol_Header
 *
 * @param header
 * @param retry
 * @param service_type
 * @param body_length
 * @param pdata
 * @return	>0 :	OK, the size of header
 * 			-1:		NULL ptr
 */
int Generate_DN_Protocol_Header(struct DMS_Protocol_Header *header, char retry,
									short service_type, int body_length, ulong64 pdata){

	int retcode = -DMS_FAIL;

	if( IS_LEGAL(DNP_MOD, header)){

		retcode = Generate_DMS_Protocol_Header(header, retry, service_type, SERVICE, body_length, pdata, atomic64_inc_return(&dnph_id_gen));

	}

	DMS_PRINTK(PALO_DBG, DNP_MOD, "end~!, retcode = %d \n", retcode);

	return retcode;
}



int SPrint_DN_Service_Request(char *dbg_str, int len_limit, struct DN_Service_Request *sreq){

	int len = 0;

	if( IS_LEGAL(META_MOD, sreq) &&
			IS_LEGAL(META_MOD, dbg_str) )
	{

		int i = 0;

		if(len < len_limit){

			len += sprintf(dbg_str+len, "\t sreq: { nr_hbids = %llu, nr_triplets = %d, \n\thbids = [", sreq->nr_hbids, sreq->nr_triplets);

			//print hbid array
			for(i = 0; i < sreq->nr_hbids; i++){

				len += sprintf(dbg_str+len, " %llu,", sreq->hbids[i]);
			}

			len += sprintf(dbg_str+len, "] \n\ttriplets = [ ");

			//print triplets array
			for(i = 0; i < sreq->nr_triplets; i++){

				len += sprintf(dbg_str+len, " %llu,", sreq->triplets[i]);
			}

			len += sprintf(dbg_str+len, "] \n\t} ");
		}

	}

	return len;

}


















