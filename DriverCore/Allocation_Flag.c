/*
 * Allocation_Flag.c
 *
 *  Created on: Mar 16, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */
#include <linux/bitops.h>

#include "DMS_Common.h"
#include "Volume.h"


/********************************************************************************/
/*																				*/
/*							Global variables 									*/
/*																				*/
/********************************************************************************/

char *ALF_MOD = "ALF: ";



/********************************************************************************/
/*																				*/
/*								DEFINITIONS										*/
/*																				*/
/********************************************************************************/


#if 0 //useful bit-ops
extern long find_first_zero_bit(const unsigned long * addr, unsigned long size);
extern long find_next_zero_bit (const unsigned long * addr, long size, long offset);
extern long find_first_bit(const unsigned long * addr, unsigned long size);
extern long find_next_bit(const unsigned long * addr, long size, long offset);
#endif

/********************************************************************************/
/*																				*/
/*							FUNCS Implementations								*/
/*																				*/
/********************************************************************************/


int Init_Allocation_Flag_Map(struct DMS_Volume *volume)
{
	int retcode = DMS_OK;

	return DMS_OK;
}

int Create_Allocation_Flag_Map(struct DMS_Volume *volume){

	int retcode = DMS_OK;

	return retcode;
}

void Release_Allocation_Flag_Map(struct DMS_Volume *volume)
{

}


ulong64 Get_Allocation_Flag(struct DMS_Volume *volume, ulong64 start_LBID)
{


#ifndef DMS_UTEST

	ulong64 chunk = 0;

	return chunk;

#else

	ulong64 chunk = 0;

	switch(start_LBID%3){
		case 0:
			chunk = 0x0000000000000000;
			break;
		case 1:
			chunk = 0xffffffffffffffff;
			break;
		case 2:
			chunk = 0xaaffaaffaaffaaff;
			break;
		default:
			chunk = 0x0000000000000000;
	}


	return chunk;

#endif
}



int Set_Allocation_Flag(struct DMS_Volume *volume, ulong64 start_LBID, int length)
{


	int retcode = DMS_OK;

	return retcode;

}

int Clear_Allocation_Flag(unsigned long long volid)
{

	int retcode = DMS_OK;

	return retcode;
}

