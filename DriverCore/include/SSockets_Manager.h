/*
 * SSockets_Manager.h
 *
 *  Created on: Apr 12, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef SSOCKETS_MANAGER_H_
#define SSOCKETS_MANAGER_H_

#include <linux/net.h>

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


typedef struct task_struct * (*CreateRecv_Fn_t)(int node_index, void *data);
typedef void (*ReleaseRecv_Fn_t)(void *data);

typedef void (*NErRep_Fn_t)(struct sock *sk);
typedef void (*NStChg_Fn_t)(struct sock *sk);




/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/

int Build_DMS_Node_Container(int node_ip, short port,
		CreateRecv_Fn_t crecv_fn, ReleaseRecv_Fn_t rrecv_fn, void *data,
		NErRep_Fn_t nerrep_fn, NStChg_Fn_t nstchg_fn);

void Destroy_DMS_Node_Container(int node_index);

int Init_DNCS_Manager(void);
void Release_DNCS_Manager(void);


int Send_Msg(int node_index, char *buf, int len, int msg_flags);
int Recv_Msg(int node_index, char *buf, int len, int msg_flags);


int Get_Node_Index_by_IP_Port(int ip, short port);
int Get_Size_of_DNCS(void);
int SPrint_DNCS_Manager(char *buf, int len_limit);

char * Get_IP_Str(int node_index);
int Get_IP_Int(int node_index);
short Get_Port(int node_index);
int Get_Node_Index_by_SSocket(struct SSocket *ssk);


extern int _inet_str2n(char *cp);


#endif /* SSOCKETS_MANAGER_H_ */
