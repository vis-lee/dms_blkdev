/*
 * Payload_Manager.h
 *
 *  Created on: May 17, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef PAYLOAD_MANAGER_H_
#define PAYLOAD_MANAGER_H_

#include "DIO.h"
#include "Datanode_Protocol.h"
#include "Payload.h"


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


struct DN_Statistic {


	//average latency per dn request
	u64 avg_mem_latency;
	u64 avg_disk_latency;

};




/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/


int Payload_Handler(struct DMS_IO *dio);
int Commit_Datanode_Tags(struct DMS_Metadata *dmeta, struct DMS_Datanode_Tag * ddt, int ack);

int Init_Payload_Handler(void);
void Release_Payload_Handler(void);


#endif /* PAYLOAD_MANAGER_H_ */
