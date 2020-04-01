/*
 * Finite_State_Machine.c
 *
 *  Created on: Mar 20, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */




/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *FSM_MOD = "FSM: ";

static struct workqueue_struct *timer_q = NULL;

/*TODO
 *
 * set fsm timer when
 * 1. send nn req
 * 2. send dn req
 *
 * cancel fsm timer when
 * 1. recv nn req
 * 2. recv dn req
 *
 * get time by:
 * time = (now - io_req_timestamp < default_timeout) ? (now - io_req_timestamp):(default_timeout);
 */

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



int DIO_State_Handler(struct DMS_IO *dio){

	int retcode = 0;
	int state = 0;

	struct request *kreq = dio->io_req->kreq;

	//this request's start time.
	kreq->start_time;

	state = atomic_read(dio->state);

	switch( state ){

		case DIO_INIT:
			break;

		case DIO_WAIT_META_DATA:
			//resend meta-data request
			break;

		case DIO_WAIT_DN_ACK:
			//resend payload request
			break;

		case DIO_COMMIT_TO_USER:
			break;

		case DIO_END:
			break;

		case DIO_COMMIT_TO_NAMENODE:
			break;

		default:
			eprintk(FSM_MOD, "unknown DIO state: %d \n", state);
	}

	return retcode;
}



/********************************************************************************/
/*																				*/
/*							Timer Scheduling/Cancel 							*/
/*																				*/
/********************************************************************************/

unsigned long Get_FSM_Schedule_Time(struct io_request * io_req){

	unsigned long long timeo = 0;

	if(io_req->tstate == IO_REQ_RETRY){

		timeo = IO_REQ_TIMEOUT * HZ;

	}else if(io_req->tstate == IO_REQ_DEADLINE){

		timeo = ((IO_REQ_MAX_RETRY_COUNT - io_req->retry_count) * IO_REQ_TIMEOUT + IO_REQ_ALLDONE_WAIT_TIMEOUT) * HZ;

	}else{

		printk("retry_count = %d over the MAX_RETRY_COUNT!\n", io_req->retry_count);
	}

	DMS_PRINTK("io_req->tstate = %d, IO_Req_Timeo = %d \n", io_req->tstate, timeo);

//	switch(io_req->tstate){
//
//		case IO_REQ_RETRY:
//
//			timeo = jiffies + IO_REQ_TIMEOUT * HZ;
//			break;
//
//		case IO_REQ_ALL_DONE:
//
//			if(io_req->retry_count < IO_REQ_MAX_RETRY_COUNT){
//				timeo = jiffies + ((IO_REQ_MAX_RETRY_COUNT - io_req->retry_count) * IO_REQ_TIMEOUT + IO_REQ_ALLDONE_WAIT_TIMEOUT) * HZ;
//			}else{
//				printk("retry_count = %d over the MAX_RETRY_COUNT!\n", io_req->retry_count);
//			}
//
//			break;
//
//		default:
//			DMS_PRINTK("unknow tstate = %d \n", io_req->tstate);
//	}

	//10s for first and second req, 60s for thirdly
	//(io_req->retry_count < IO_REQ_MAX_RETRY_COUNT) ? (timeo = jiffies + IO_REQ_TIMEOUT * HZ) : (timeo = jiffies + IO_REQ_LAST_WAIT_TIMEOUT * HZ);

	return timeo;
}



int Cancel_FSM_Timer(struct io_request * io_req){

	int retcode = -DMS_FAIL;

	DMS_PRINTK("%s, start~! req_id = %d\n", __func__, io_req->rid);

	if(IS_DRIVER_WORKING() &&
		check_ptr_validation("IO_Req:", __func__, io_req) )
	{
		struct work_struct *work = &io_req->retry_work;

		if(work->data != 0){
			//this API will flush the work if cancel failed.
			cancel_rearming_delayed_work(work);

			//reset to 0 to prevent double cancel
			work->data = 0;
		}

	}

	DMS_PRINTK("%s, end~! retcode = %d\n", __func__, retcode);

	return retcode;
}

int Schedule_FSM_Timer(struct io_request * io_req){

	int retcode = -DMS_FAIL;

	DMS_PRINTK("%s, start~! io_req_id = %llu \n", __func__, io_req->rid);

	if(IS_DRIVER_WORKING() &&
		check_ptr_validation("IO_Req:", __func__, io_req) )
	{
		INIT_WORK( &io_req->retry_work, (void *)IO_Req_Timeout_Handler, (void *)io_req);

		retcode = queue_delayed_work(io_req->drive->retry_workq, &io_req->retry_work, Get_IO_Req_Schedule_Time(io_req));
	}

	DMS_PRINTK("%s, end~! retcode = %d\n", __func__, retcode);

	return retcode;
}



/********************************************************************************/
/*																				*/
/*							MODULE INIT FUNC									*/
/*																				*/
/********************************************************************************/

int Init_Finite_State_Machine(void){

	int retcode = DMS_OK;

	timer_q = create_workqueue("dms_fsm_workq");

	return retcode;
}


void Release_Finite_State_Machine(void){

	//flush work queue, prevent there are remain works.
	flush_workqueue(timer_q);

	destroy_workqueue(timer_q);

}









