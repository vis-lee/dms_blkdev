/*
 * Allocation_Flag.h
 *
 *  Created on: Mar 16, 2012
 *      Author: Vis Lee
 *              Lego Lin
 *
 */

#ifndef ALLOCATION_FLAG_H_
#define ALLOCATION_FLAG_H_

#include "DMS_Common.h"

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
//one chunk presents 64 DMSBLKs
#define CHUNK_SIZE					64LL
#define CHUNK_SIZE_SHIFT_BITS		6
#define CHUNK_MASK					( ~(CHUNK_SIZE-1) )
#define CHUNK_ALIGN(nr_dsects)		( (nr_dsects + (CHUNK_SIZE-1)) & CHUNK_MASK )

#define CAL_NR_CHUNKS(nr_dsects)	( CHUNK_ALIGN(nr_dsects) >> CHUNK_SIZE_SHIFT_BITS)

//#define NR_CHUNKS					CAL_NR_CHUNKS(KSECTORS_TO_DSECTORS(MAX_KSECTORS))


struct ALF_Unit{

		//allocation flags vector * n
		ulong64 *alflags_vect;

		//rwlock for ALF_Unit
		rwlock_t lock;

		int vect_size;
};

struct ALF_Map{
		//TODO we will use hlist in the future

		struct ALF_Unit alf_unit;

		ulong64 nr_chunks;

};



/********************************************************************************/
/*																				*/
/*								FUNCS DECLARATION								*/
/*																				*/
/********************************************************************************/

int Init_Allocation_Flag_Map(struct DMS_Volume *volume);
int Create_Allocation_Flag_Map(struct DMS_Volume *volume);
void Release_Allocation_Flag_Map(struct DMS_Volume *volume);
int Clear_Allocation_Flag_Map(struct DMS_Volume *volume);

ulong64 Get_Allocation_Flag(struct DMS_Volume *volume, ulong64 start_LBID);
int Set_Allocation_Flag(struct DMS_Volume *volume, ulong64 start_LBID, int length);


#endif /* ALLOCATION_FLAG_H_ */
