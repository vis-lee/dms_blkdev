/*
 * DMS_Utest.c
 *
 *  Created on: Feb 14, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 *	the entry point that received IOCTL from user space and handle test requests.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/kallsyms.h>

#include "DMS_Common.h"
#include "Test_IOCTL.h"
#include "DMS_Error.h"

#include "Test_Cases.h"
#include "UT_Type.h"
#include "Test_Workers.h"
#include "DMS_Error.h"
//#include "DMS_IOCTL.h"
#include "Utils.h"

/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *TEST_MOD = "TEST: ";

char TFUNC_NAME[KSYM_NAME_LEN];

extern int _inet_str2n(char *cp);

/********************************************************************************/
/*																				*/
/*									FUNCS 										*/
/*																				*/
/********************************************************************************/

int Test_Executer(UT_Param_t *ut_arg, Env_op_t *env_ops, UTest_Fn_t tfunc, void *test_param){

	int retcode = -DMS_FAIL;

	//prepare workers

	Test_Workers_t *tws = Init_Test_Env_and_Workers(ut_arg, env_ops, tfunc, test_param);

	if(IS_LEGAL(TEST_MOD, tws)){

		Start_TestWorkers(tws);

		retcode = DMS_OK;

	}

	return retcode;
}



void Print_User_Args(UT_Param_t *ut_arg){

	if(IS_LEGAL(TEST_MOD, ut_arg)){

		printk("ut_arg: user_param = %lld, test_type = %u, test_case = %u, threads = %u, nr_elements = %u \n",
				ut_arg->user_param, ut_arg->test_type, ut_arg->test_case, ut_arg->threads, ut_arg->nr_elements);

	}
}

/**
 * parse the test cmds from user
 * @param: test_arg - the test arguments from user
 * @param: test_param - if the test case need special parameters or data structure, you can pass it.
 */
int Parse_Test_Cmds(UT_Param_t *ut_arg){

	int retcode = -DMS_FAIL;

	UTest_Fn_t tfunc = NULL;
	void *test_param = NULL;	//specified and used by testers.

	Env_op_t env_ops = {NULL, 0, NULL, 0, NULL, 0};

	Print_User_Args(ut_arg);

	if(IS_LEGAL(TEST_MOD, ut_arg)){

		switch(ut_arg->test_case){

			case TCASE_VOLMGR:
				tfunc = Test_Volume_Manager;
				test_param = NULL;
				break;

			case TCASE_DMP:
				tfunc = Test_DMS_Mem_Pool;
				test_param = NULL;
				break;

			case TCASE_DLIST:
				if( ut_arg->threads <= 1 )
					tfunc = Test_DMS_DList;
				else
					tfunc = Test_DMS_DList_Multi_Threads;


				test_param = NULL;

				env_ops.setup = Setup_TDList_Env;
				env_ops.teardown = Teardown_TDList_Env;
				env_ops.verify = Verify_Test_TDList_Result;
				break;

			case TCASE_VIOHDER:

				tfunc = Test_vIO_Handler;

				test_param = NULL;

				env_ops.setup = Setup_Test_vIO_Env;
				env_ops.setup_param = ut_arg->user_param;
				env_ops.teardown = Teardown_Test_vIO_Env;
				env_ops.teardown_param = ut_arg->user_param;
				env_ops.verify = Verify_Test_vIO_Result;
				break;

			case TCASE_DNC_BUILDING:
			{

				tfunc = Test_DNC;

				env_ops.setup = Setup_TDNC_Env;
				env_ops.teardown = Teardown_TDNC_Env;
				env_ops.verify = Verify_TDNC_Result;
				break;
			}

			default:

				eprintk(TEST_MOD, "unknown test case: %d\n", ut_arg->test_case);
				ut_arg->result = -ENOTTY;
				return -ENOTTY;
				break;
		}

		printk("/********************************************************************************/\n");
		print_symbol("\t\t UTEST FUNC: %s \n", (unsigned long)tfunc);
		printk("/********************************************************************************/\n");

		//start test
		Test_Executer(ut_arg, &env_ops, tfunc, test_param);

	}


	return retcode;
}




//static ssize_t dms_misc_read(struct file *file, char __user *user,
//		    size_t size, loff_t *o)
//{
//	struct task_struct *tsk = NULL;
//
//	//search the target task
//
//
//	return 0;
//}
//
//static ssize_t dms_misc_write(struct file *file, const char __user *in,
//		     size_t size, loff_t *off)
//{
//	//TODO we should check the length of the char array?
//	copy_from_user(proc_name, in, TASK_COMM_LEN);
//
//	dms_misc_read(file, NULL, 0, NULL);
//
//	return 0;
//}

int Test_Case_IOCTL(struct inode *inode, /* see include/linux/fs.h */
						 struct file *file, /* ditto */
						 unsigned int ioctl_cmd, /* number and param for ioctl */
						 unsigned long param) {

	void __user *ioctl_param = (void __user *)param;

	UT_Param_t ut_arg;

	switch (ioctl_cmd) {

		case IOCTL_CMDT:

			copy_from_user(&ut_arg, ioctl_param, sizeof(UT_Param_t));

			Parse_Test_Cmds(&ut_arg);

			copy_to_user( ioctl_param, &ut_arg, sizeof(UT_Param_t) );
			break;

		case IOCTL_CTVOL:
		{
			struct dms_volume_info vinfo;
			copy_from_user(&vinfo, ioctl_param, sizeof(struct dms_volume_info));

			Attach_UTest_Volume(&vinfo);

			if(vinfo.deviceNum == 1){

				Setup_Test_vIO_Env(vinfo.volid);

			}
		}

		break;

		case IOCTL_RTVOL:
		{
			struct dms_volume_info vinfo;
			copy_from_user(&vinfo, ioctl_param, sizeof(struct dms_volume_info));

			if(vinfo.deviceNum == 1){

				Teardown_Test_vIO_Env(vinfo.volid);

			}

			Detach_UTest_Volume(&vinfo);

			printk("Vis dbg: IOCTL_RTVOL done! \n");

		}
		break;

		default:

			eprintk(TEST_MOD, "unknown ioctl:%d\n", ioctl_cmd);
			return -ENOTTY;
			break;
	}

	return 0;


}

static struct file_operations mymisc_fops = {
	.owner	= THIS_MODULE,
	.ioctl	= Test_Case_IOCTL,
};

static struct miscdevice mymisc_dev = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= TEST_CONTROLLER_NAME,
	.fops	= &mymisc_fops,
};

int __init ut_misc_device_init(void)
{

	memset(&TFUNC_NAME, 0, KSYM_NAME_LEN);

	return misc_register(&mymisc_dev);

}

void __exit ut_misc_device_remove(void)
{

	misc_deregister(&mymisc_dev);
}

module_init(ut_misc_device_init);
module_exit(ut_misc_device_remove);

MODULE_LICENSE("Dual BSD/GPL");
