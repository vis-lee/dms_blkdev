/*
 * Metadata_Manager.h
 *
 *  Created on: Mar 20, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef METADATA_MANAGER_H_
#define METADATA_MANAGER_H_

#include "DIO.h"
#include <linux/jiffies.h>


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

#define IS_OVER_TIME(wait_time)		time_after(jiffies, wait_time)




/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/


int Metadata_Handler(struct DMS_IO *dio);

int Commit_to_Namenode(struct DMS_Metadata *dmeta);



//TODO init a socket receiver here, and using kthread_stop(); at Release func.
int Init_Metadata_Manager(int nn_ip, short port);
void Release_Metadata_Manager(void);



#endif /* METADATA_MANAGER_H_ */
