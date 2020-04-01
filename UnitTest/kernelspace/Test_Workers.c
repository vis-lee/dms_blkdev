/*
 * TestWorkers.c
 *
 *  Created on: Feb 14, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */


#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/timer.h>

#include "DMS_Common.h"
#include "Test_Workers.h"
#include "Test_IOCTL.h"
#include "UT_Type.h"
#include "DMS_Type.h"


/********************************************************************************/
/*																				*/
/*								PARAMETERS										*/
/*																				*/
/********************************************************************************/

char *TW_MOD = "TWorkers: ";



#define TWS_TIMEOUT		2*60 //sec

#define EXE_ENV_SETUP(tws)					if(CHECK_PTR(TW_MOD, tws) && CHECK_PTR(TW_MOD, tws->env_apis.setup)){ tws->env_apis.setup(tws->env_apis.setup_param); }
#define EXE_ENV_TEARDOWN(tws)				if(CHECK_PTR(TW_MOD, tws) && CHECK_PTR(TW_MOD, tws->env_apis.teardown)){ tws->env_apis.teardown(tws->env_apis.teardown_param); }
#define EXE_ENV_VERIFY(tws)					if(CHECK_PTR(TW_MOD, tws) && CHECK_PTR(TW_MOD, tws->env_apis.verify)){ tws->env_apis.verify(tws->env_apis.verify_param); }


/********************************************************************************/
/*																				*/
/*									FUNCS										*/
/*																				*/
/********************************************************************************/


unsigned long getTimeoutValue(void){

	//return jiffies + TWS_TIMEOUT * HZ;
	return TWS_TIMEOUT * HZ;

}


void Print_All_Workers(Test_Workers_t *tws)
{

	if (IS_LEGAL(TW_MOD, tws))
	{

		int i = 0;
		int nr_workers = tws->nr_workers;
		TWorker_t *worker = NULL;

		printk("the workers are: [ \n");

		for (i = 0; i < nr_workers; i++)
		{

			if ( tws->workers[i] )
			{
				worker = tws->workers[i];

				printk("tw->workers[%d]: %s, \n", i, worker->task_ptr->comm);

				worker = NULL;
			}

		}

		printk("]\n");

	}

}

void Release_TestWorkers(Test_Workers_t *tws)
{

	if (IS_LEGAL(TW_MOD, tws))
	{

		int i = 0;
		int nr_workers = tws->nr_workers;

		for (i = 0; i < nr_workers; i++)
		{

			Release_TestWorker(tws->workers[i]);
		}

		kfree(tws->ut_arg);
		kfree(tws->results);
		kfree(tws->workers);
		kfree(tws);

	}

}


int __Test_Workers_Done_Func(void *worker_ptr, int result){

	int retcode = -DMS_FAIL;
	TWorker_t *worker = (TWorker_t *)worker_ptr;
	Test_Workers_t *tws = (Test_Workers_t *)worker->tws_mgr;

	//increase reference count;
	retcode = atomic_inc_return(&tws->done_count);

	//set result
	tws->results[worker->tid] = result;

	if(retcode == tws->nr_workers){
		//wakeup tws thread.
		wake_up_interruptible_all(&tws->wait_all_done);
	}

	return retcode;

}

int __Create_All_Workers(Test_Workers_t *tws, UTest_Fn_t tfunc, void *tf_param){

	int i = 0, retcode = DMS_OK;
	unsigned int nr_workers = tws->nr_workers;

	for (i = 0; i < nr_workers; i++)
	{
		tws->workers[i] = Create_TestWorker(i+1, tws, tfunc, tf_param);

		if(tws->workers[i] == NULL)
		{
			tws->results[i] = -DMS_FAIL;

			tws->nr_workers--;

			eprintk(TW_MOD, "set fail result!\n");
		}
	}

	if(TWS_DBG){

		Print_All_Workers(tws);

		DMS_PRINTK(TWS_DBG, TW_MOD, "end~!\n");
	}

	return retcode;

}

/*
 * test_arg: pass from userspace
 * test_param: the parameters that tester function needed.
 */
Test_Workers_t * Create_TestWorkers_Mgr(UT_Param_t *ut_arg, Env_op_t *env_ops)
{

	Test_Workers_t *tws = NULL;

	if (IS_LEGAL(TW_MOD, ut_arg))
	{
		unsigned int nr_workers = ut_arg->threads;

		tws = (Test_Workers_t *) DMS_Malloc(sizeof(Test_Workers_t));

		tws->nr_workers = nr_workers;

		tws->workers = (TWorker_t **) DMS_Malloc(sizeof(TWorker_t *) * nr_workers);

		tws->results = (long64 *) DMS_Malloc(sizeof(long64) * nr_workers);
		memset(tws->results, 0, sizeof(long64) * nr_workers);

		tws->ut_arg = (UT_Param_t *) DMS_Malloc( sizeof(UT_Param_t) );
		memcpy(tws->ut_arg, ut_arg, sizeof(UT_Param_t));

		tws->retry_count = 0;

		init_waitqueue_head(&tws->wait_all_done);

		atomic_set(&tws->done_count, 0);

		/* you can specify your won done_func */
		tws->done_func = __Test_Workers_Done_Func;

		/* copy env APIs */
		memcpy(&tws->env_apis, env_ops, sizeof(Env_op_t));

	}


	return tws;

}



/**
 * Init_Test_Env_and_Workers
 *
 * @param ut_arg		-user IOCTL args
 * @param env_ops 		-setup your test environment.
 * @param tfunc			-test function
 * @param tf_param		-test param
 * @return
 *
 * DESCRIPTION:
 * 		create threads and a Test_Workers_Manager to start unit test.
 * 		besides, you can specify three additional functions for construct/destroy
 *		your test environment, and a verify function for check testing results.
 *		You can find the definition in the Env_op_t.
 */
Test_Workers_t * Init_Test_Env_and_Workers(UT_Param_t *ut_arg, Env_op_t *env_ops, UTest_Fn_t tfunc, void *tf_param)
{

	Test_Workers_t *tws = Create_TestWorkers_Mgr(ut_arg, env_ops);

	if( __Create_All_Workers(tws, tfunc, tf_param) < 0 ){
		goto create_tw_fail;
	}


	printk("%s%s, tws->env_apis.setup = %p, tws->env_apis.param = %lu\n", TW_MOD, __func__, tws->env_apis.setup, tws->env_apis.setup_param);

	EXE_ENV_SETUP(tws);

	return tws;

create_tw_fail:

	if (IS_LEGAL(TW_MOD, tws))
	{
		Release_TestWorkers(tws);
	}

	eprintk(TW_MOD, "create all workers fail!");

	return NULL;
}

int Start_TestWorkers(Test_Workers_t *tws)
{

	int i = 0;
	int retcode = -DMS_FAIL;
	int nr_workers = 0;
	int blocking_flag = 0;

	if(IS_LEGAL(TW_MOD, tws)){

		nr_workers = tws->nr_workers;
		blocking_flag = tws->ut_arg->test_type;

		//TODO we should check the nr_workers validation
		for (i = 0; i < nr_workers; i++)
		{
			Start_TestWorker( tws->workers[i] );
		}

		if(blocking_flag == TTYPE_BLOCKING){

			//wait for all thread done. and collect results
			while( (tws->retry_count++ < MAX_RETRY_COUNT) &&
					(atomic_read(&tws->done_count) < tws->nr_workers) )
			{
				//FIXME when user ^C, we have to kill all sleeping/waiting workers.

				retcode = wait_event_interruptible_timeout( tws->wait_all_done,
								atomic_read(&tws->done_count) == tws->nr_workers, TWS_TIMEOUT * HZ );

				iprintk(TW_MOD, "WAKED UP, retry_count = %d, tws = %p, done_count = %d, nr_workers = %d \n",
						tws->retry_count, tws, atomic_read(&tws->done_count), tws->nr_workers);
			}

			iprintk(TW_MOD, "VERIFY result, tws = %p\n", tws);
			EXE_ENV_VERIFY(tws);

			//Print_Result();

			//cleanup environment data structures. we call clean here due to consider the time out case and thread dead.
			EXE_ENV_TEARDOWN(tws);
			iprintk(TW_MOD, "TEARDOWN end~! tws = %p\n", tws);

		}else{

			retcode = DMS_OK;
			iprintk(TW_MOD, "Non-Blocking test mode, tws = %p\n", tws);
			//FIXME we haven't teardown the test env
		}


	}


	DMS_PRINTK(TWS_DBG, TW_MOD, "end~! tws = %p\n", tws);

	return retcode;
}
