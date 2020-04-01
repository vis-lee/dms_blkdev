/*
 * SSocket.h
 *
 *  Created on: Apr 12, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef SSOCKET_H_
#define SSOCKET_H_

#include <linux/net.h>
#include "DMS_Common.h"
#include "SSockets_Manager.h"


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

#define CONNECTION_RETYR_LIST

#define IS_DNC_CONNECTED(dnc)		(atomic_read(&dnc->status) == DNODE_CONNECTED)

enum Node_State{
	DNODE_INIT 				= 1,
	DNODE_BUILDING 			= 2,
	DNODE_CONNECTED 		= 3,
	DNODE_DISCONNECTED		= 4,
};


/********************************************************************************/
/*																				*/
/*								Definitions	 									*/
/*																				*/
/********************************************************************************/

/**
 * Integrate DMS node information
 */
struct DMS_Node_Container {

	//ip and port
	struct SSocket ssk;

//	cip_t node_ip;
//	short port;

	char ip_str[DMS_IP_STR_LEN];

	struct socket *sock;

	// build connection work
	struct work_struct bc_work;

	//ref count
	struct kref	 dnc_kref;

	//average arrival rate
	int avg_arv_rate;

	int send_retry;

	atomic_t status;

	rwlock_t lock;

	int retry_count;

	struct task_struct *recver;

	CreateRecv_Fn_t crecv_fn;
	ReleaseRecv_Fn_t rrecv_fn;

	NErRep_Fn_t node_error_report;
	NStChg_Fn_t node_state_change;

	int index;

	void *private;

};


/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/


int sock_xmit(struct socket *sock, int send, void *buf, int size,
		int msg_flags);


int Build_Connection(struct DMS_Node_Container *dnc);


int Init_DMS_Node_Container(struct DMS_Node_Container *dnc, int node_ip, short port,
		CreateRecv_Fn_t crecv_fn, ReleaseRecv_Fn_t rrecv_fn, void *data,
		NErRep_Fn_t nerrep, NStChg_Fn_t nstchg);

struct DMS_Node_Container * Create_DMS_Node_Container(int node_ip, short port,
		CreateRecv_Fn_t crecv_fn, ReleaseRecv_Fn_t rrecv_fn, void *data,
		NErRep_Fn_t nerrep_fn, NStChg_Fn_t nstchg_fn);

void Release_DMS_Node_Container(struct DMS_Node_Container *dnc);


int SPrint_DMS_Node_Containers(char *buf, int len_limit, struct DMS_Node_Container *dnc);

int Init_Node_Builder(void);
void Release_Node_Builder(void);

#endif /* SSOCKET_H_ */
