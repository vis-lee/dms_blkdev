/*
 * Test_Cases.h
 *
 *  Created on: Feb 14, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef TEST_CASES_H_
#define TEST_CASES_H_

#include "Test_IOCTL.h"
#include "Test_Workers.h"
#include "UT_Type.h"

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










/********************************************************************************/
/*																				*/
/*									 FUNCS 										*/
/*																				*/
/********************************************************************************/

int Parse_Test_Cmds(UT_Param_t *utp);


//extern int IOCTL_Test_Volume_Manager(UT_Param_t *test_param);
extern int Test_Volume_Manager(TWorker_t *tw);
extern int Test_DMS_Mem_Pool(TWorker_t *tw);

/****** Test DList ******/
extern int Test_DMS_DList(TWorker_t *tw);
extern int Test_DMS_DList_Multi_Threads(TWorker_t *tw);
extern int Setup_TDList_Env(unsigned long param);
extern	void Teardown_TDList_Env(unsigned long param);
extern int Verify_Test_TDList_Result(unsigned long param);

/****** Test vIO handler ******/
int Test_vIO_Handler(TWorker_t *tw);
int Setup_Test_vIO_Env(unsigned long param);
void Teardown_Test_vIO_Env(unsigned long param);
int Verify_Test_vIO_Result(unsigned long param);


/****** Test DNC MGR ******/
int Test_DNC(TWorker_t *tw);
int Setup_TDNC_Env(unsigned long param);
void Teardown_TDNC_Env(unsigned long param);
int Verify_TDNC_Result(unsigned long param);


#endif /* TEST_CASES_H_ */
