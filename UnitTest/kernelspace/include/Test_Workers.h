/*
 * Test_Workers.h
 *
 *  Created on: Feb 14, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */
//#include <linux/wait.h>

#include "UTest_Common.h"
#include "UT_Type.h"
#include "DMS_Type.h"


#ifndef TEST_WORKERS_H_
#define TEST_WORKERS_H_



/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

struct Test_Workers;

typedef struct Test_Worker
{

	int tid;
	struct task_struct *task_ptr;

	struct Test_Workers *tws_mgr;
	void *tf_param;

}TWorker_t;



// function type
typedef int (*Done_Fn_t)(void *tws_mgr, int result);
typedef int (*UTest_Fn_t)(TWorker_t *);

typedef int (*Setup_Env_t)(unsigned long param);
typedef void (*Teardown_Env_t)(unsigned long param);
typedef int (*Verify_Fn_t)(unsigned long param);


typedef struct Env_Operations{

	//user specified env constructor
	Setup_Env_t setup;
	unsigned long setup_param;

	//user specified env destructor
	Teardown_Env_t teardown;
	unsigned long teardown_param;

	//user specified verify function
	Verify_Fn_t verify;
	unsigned long verify_param;

}Env_op_t;


typedef struct Test_Workers
{

	int nr_workers;
	TWorker_t **workers;

	//user IOCTL arg
	UT_Param_t *ut_arg;

	long64 *results;

	int retry_count;

	wait_queue_head_t wait_all_done;

	Done_Fn_t done_func;

	atomic_t done_count;

	Env_op_t env_apis;

}Test_Workers_t;


/********************************************************************************/
/*																				*/
/*								DEFINITIONS 									*/
/*																				*/
/********************************************************************************/




/********************************************************************************/
/*																				*/
/*						Test Workers Manager FUNCTIONS 							*/
/*																				*/
/********************************************************************************/

void Release_TestWorkers(Test_Workers_t *tws);
/*
 * test_arg: pass from userspace
 * test_param: the parameters that tester function needed.
 */
Test_Workers_t * Init_Test_Env_and_Workers(UT_Param_t *ut_arg, Env_op_t *env_ops, UTest_Fn_t tfunc, void *tf_param);

int Start_TestWorkers(Test_Workers_t *tws);






/********************************************************************************/
/*																				*/
/*							Test Worker FUNCTIONS 								*/
/*																				*/
/********************************************************************************/

extern void Release_TestWorker(TWorker_t *worker);
extern TWorker_t * Create_TestWorker(int id, Test_Workers_t *tws, UTest_Fn_t tfunc, void *test_param);
extern int Start_TestWorker(TWorker_t *worker);




#endif /* TEST_WORKERS_H_ */
