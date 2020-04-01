/*
 * TestWorkers.c
 *
 *  Created on: Feb 14, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */


#include <linux/kthread.h>

#include "DMS_Common.h"
#include "Test_Workers.h"
#include "Test_IOCTL.h"
#include "UT_Type.h"

/********************************************************************************/
/*																				*/
/*								PARAMETERS										*/
/*																				*/
/********************************************************************************/

extern char *TW_MOD;


/********************************************************************************/
/*																				*/
/*									FUNCS										*/
/*																				*/
/********************************************************************************/
void Release_TestWorker(TWorker_t *worker)
{

	if (IS_LEGAL(TW_MOD, worker))
	{

		if (pid_alive(worker->task_ptr))
		{

			kill_proc(worker->task_ptr->pid, SIGKILL, 0);
		}

		kfree(worker);

	}

}

/*
 * create one test worker
 */
TWorker_t * Create_TestWorker(int id, Test_Workers_t *tws, UTest_Fn_t tfunc, void *tf_param)
{

	TWorker_t *worker = NULL;

	if (IS_LEGAL(TW_MOD, tws))
	{

		worker = (TWorker_t *) DMS_Malloc(sizeof(TWorker_t));

		worker->tid = id;
		worker->tws_mgr = tws;
		worker->tf_param = tf_param;

		worker->task_ptr = kthread_create(tfunc, (void *)worker, "worker[%d]", id);

		DMS_PRINTK(TWS_DBG, TW_MOD, "end~! worker = %p, id = %d, task_ptr = %p \n",
					worker, id, worker->task_ptr);

		if (!worker->task_ptr)
		{
			goto create_tw_fail;
		}

	}

	return worker;

create_tw_fail:


	if (IS_LEGAL(TW_MOD, worker))
	{
		Release_TestWorker(worker);
	}

	eprintk(TW_MOD, "create worker fail! id = %d", id);

	return NULL;
}

int Start_TestWorker(TWorker_t *worker)
{

	int retcode = DMS_OK;

	if (IS_LEGAL(TW_MOD, worker))
	{
		wake_up_process( worker->task_ptr );
	}

	return retcode;
}
