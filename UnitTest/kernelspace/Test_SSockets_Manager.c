/*
 * Test_SSockets_Manager.c
 *
 *  Created on: Apr 24, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#include <linux/kthread.h>

#include "DMS_Common.h"
#include "UTest_Common.h"
#include "SSocket.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *TDNC_MOD = "TDNC: ";



/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/

extern void Reset_DNCS_Manager(void);
extern struct DMS_Node_Container * __Node_Index_to_DNC(int node_index);

/********************************************************************************/
/*																				*/
/*							FUNCS Implementations								*/
/*																				*/
/********************************************************************************/

int Test_Recv_Func(void *data){

	char buf[PAGE_SIZE];

	int node_index = (int) data;

	int i = 0;

	int result = 0;

	//struct DMS_Node_Container *dnc = (struct DMS_Node_Container *)data;

	//it should be always wait at the socket
	while(result >= 0){

		result = Recv_Msg(node_index, (char *)&buf, PAGE_SIZE, 0);
		DMS_PRINTK(TDNC_DBG, TDNC_MOD, "%d-th return... retcode = %d \n", ++i, result);

		msleep(100);

	}

	iprintk(TDNC_MOD, "someone stop me = %s, go to exit!\n", (char *)&current->comm);

	return result;

}

struct task_struct * Create_Receiver(int node_index, void *data){

	struct task_struct *recver = NULL;
	recver = kthread_create(Test_Recv_Func, (void *)node_index, "TestReceiver[]" );

	return recver;

}

void Stop_Receiver(void *data){

	struct task_struct *receiver = (struct task_struct *)data;

	if(pid_alive(receiver)){

		DMS_PRINTK(TDNC_DBG, TDNC_MOD, "stopping %s \n", (char *)&receiver->comm);

		kthread_stop(receiver);

		DMS_PRINTK(TDNC_DBG, TDNC_MOD, "stopped %s \n", (char *)&receiver->comm);

	}else{

		DMS_PRINTK(TDNC_DBG, TDNC_MOD, "thread has dead... dn_receiver = %p \n", receiver);
	}
}

int Test_DNC(TWorker_t *tw)
{

	int retcode = DMS_OK, node_index = -1;
	Test_Workers_t *tws = NULL;

	if( IS_LEGAL(TDNC_MOD, tw) &&
			IS_LEGAL(TDNC_MOD, tw->tws_mgr) )
	{
		int tid = 0;
		unsigned int timeo = 0;
		char *ip_c = NULL;
		int node_ip;
		unsigned short port = 0;
		struct DMS_Node_Container *dnc = NULL;

		tid = tw->tid;
		tws = tw->tws_mgr;
		timeo = tws->ut_arg->nr_elements;


		node_ip = (int)tw->tws_mgr->ut_arg->user_param;
		port = tw->tws_mgr->ut_arg->result;

		ip_c = (char *)&node_ip;

		DMS_PRINTK(TDNC_DBG, TDNC_MOD, "tid = %d, ip = %d.%d.%d.%d, port = %d \n",
				tid, ip_c[3], ip_c[2], ip_c[1], ip_c[0], port);

		node_index = Build_DMS_Node_Container(node_ip, port, Create_Receiver, Stop_Receiver, NULL, NULL, NULL);

		DMS_PRINTK(TDNC_DBG, TDNC_MOD, "tid = %d, node_index = %d \n", tid, node_index);


		//if not connected, wait until connected.
		while((dnc = __Node_Index_to_DNC(node_index)) &&
				atomic_read(&dnc->status) != DNODE_CONNECTED)
		{

			msleep(timeo*1000);

			DMS_PRINTK(TDNC_DBG, TDNC_MOD, "tid = %d, node_index = %d, status = %d \n",
					tid, node_index, atomic_read(&dnc->status));

		}


	} else {

		retcode = -DMS_FAIL;
	}

	if( CHECK_PTR(TDNC_MOD, tws->done_func) ){

		tws->done_func(tw, retcode);
	}


	return retcode;

}




/********************************************************************************/
/*																				*/
/*									Env APIs									*/
/*																				*/
/********************************************************************************/

int Setup_TDNC_Env(unsigned long param){

	int retcode = DMS_OK;

	//init DNC MGR
	//retcode = Init_DNCS_Manager();

	return retcode;

}

void Teardown_TDNC_Env(unsigned long param){

	char dbg_str[PAGE_SIZE];

	SPrint_DNCS_Manager((char *)&dbg_str, PAGE_SIZE-256);

	DMS_PRINTK(TDNC_DBG, TDNC_MOD, "%s \n", (char *)&dbg_str);

	Reset_DNCS_Manager();

}

int Verify_TDNC_Result(unsigned long param){

	int retcode = DMS_OK;


	return retcode;
}








