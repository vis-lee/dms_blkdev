/*
 * DMS_Debug.h
 *
 *  Created on: 2011/7/13
 *      Author: 980263
 */

#ifndef DEBUG_UTILS_H_
#define DEBUG_UTILS_H_

#include "DMS_Common.h"


/********************************************************************************/
/*																				*/
/*								GLOBAL VARIABLE									*/
/*																				*/
/********************************************************************************/

//FIXME undefine DMS_DEBUG
#define DMS_DEBUG 1

#if (DMS_DEBUG == 1)
	static int dms_dbg_flag = 0xfffff7f5;
	//static int dms_dbg_flag = 0xffffffff;
	//static int dms_dbg_flag = 0;
#else
	static int dms_dbg_flag = 0;
#endif


/********************************************************************************/
/*																				*/
/*							DEBUG FLAG DEFINITIONS								*/
/*																				*/
/********************************************************************************/
#define DMSDEV_DBG_FLAG_BIT		0	/*dev_init or ioctl related*/
#define VOLUME_DBG_FLAG_BIT		1	/*volume manager dbg*/
#define IOR_DBG_FLAG_BIT		2	/*io_req dbg*/
#define LB_DBG_FLAG_BIT			3	/*Logical Block dbg*/
#define CACHE_DBG_FLAG_BIT		4	/*cache dbg*/
#define OVW_DBG_FLAG_BIT		5	/*over written flag dbg*/
#define META_DBG_FLAG_BIT		6	/*meta data handler dbg*/
#define PALO_DBG_FLAG_BIT		7	/*payload handler dbg*/
#define DNC_DBG_FLAG_BIT		8	/*DMS Node Container dbg*/
#define FSM_DBG_FLAG_BIT		9	/*finite state machine dbg*/
#define KP_DBG_FLAG_BIT			10	/*Kthread pool dbg*/
#define CMT_DBG_FLAG_BIT		11	/*nn committer dbg*/
#define DMP_DBG_FLAG_BIT		12	/*DMS Mem Pool dbg*/
#define DLIST_DBG_FLAG_BIT		13	/*DMS DList dbg*/
#define DIO_DBG_FLAG_BIT		14	/*DMS IO dbg*/
#define DC_DBG_FLAG_BIT			15	/*DRIVE CORE dbg*/

//dbg flags use to compare to dms_dbg_flag
#define DMSDEV_DBG_FLAG		(1<<DMSDEV_DBG_FLAG_BIT)
#define VOLUME_DBG_FLAG		(1<<VOLUME_DBG_FLAG_BIT)
#define IOR_DBG_FLAG		(1<<IOR_DBG_FLAG_BIT)
#define LB_DBG_FLAG			(1<<LB_DBG_FLAG_BIT)
#define CACHE_DBG_FLAG		(1<<CACHE_DBG_FLAG_BIT)
#define OVW_DBG_FLAG		(1<<OVW_DBG_FLAG_BIT)
#define META_DBG_FLAG		(1<<META_DBG_FLAG_BIT)
#define PALO_DBG_FLAG		(1<<PALO_DBG_FLAG_BIT)
#define DNC_DBG_FLAG		(1<<DNC_DBG_FLAG_BIT)
#define FSM_DBG_FLAG		(1<<FSM_DBG_FLAG_BIT)
#define KP_DBG_FLAG			(1<<KP_DBG_FLAG_BIT)
#define CMT_DBG_FLAG		(1<<CMT_DBG_FLAG_BIT)
#define DMP_DBG_FLAG		(1<<DMP_DBG_FLAG_BIT)
#define DLIST_DBG_FLAG		(1<<DLIST_DBG_FLAG_BIT)
#define DIO_DBG_FLAG		(1<<DIO_DBG_FLAG_BIT)
#define DC_DBG_FLAG			(1<<DC_DBG_FLAG_BIT)

/*
 * set and clear dbg flag macro
 */
#define SET_DMSDEV_DBG_FLAG(dbg_flag)	(set_bit(DMSDEV_DBG_FLAG_BIT, &dbg_flag))
#define SET_VOLUME_DBG_FLAG(dbg_flag)	(set_bit(VOLUME_DBG_FLAG_BIT, &dbg_flag))
#define SET_IOR_DBG_FLAG(dbg_flag)		(set_bit(IOR_DBG_FLAG_BIT	, &dbg_flag))
#define SET_LB_DBG_FLAG(dbg_flag)		(set_bit(LB_DBG_FLAG_BIT	, &dbg_flag))
#define SET_CACHE_DBG_FLAG(dbg_flag)	(set_bit(CACHE_DBG_FLAG_BIT	, &dbg_flag))
#define SET_OVW_DBG_FLAG(dbg_flag)		(set_bit(OVW_DBG_FLAG_BIT	, &dbg_flag))
#define SET_META_DBG_FLAG(dbg_flag)		(set_bit(META_DBG_FLAG_BIT	, &dbg_flag))
#define SET_PALO_DBG_FLAG(dbg_flag)		(set_bit(PALO_DBG_FLAG_BIT	, &dbg_flag))
#define SET_DNC_DBG_FLAG(dbg_flag)		(set_bit(DNC_DBG_FLAG_BIT	, &dbg_flag))
#define SET_FSM_DBG_FLAG(dbg_flag)		(set_bit(FSM_DBG_FLAG_BIT	, &dbg_flag))
#define SET_KP_DBG_FLAG(dbg_flag)		(set_bit(KP_DBG_FLAG_BIT	, &dbg_flag))
#define SET_CMT_DBG_FLAG(dbg_flag)		(set_bit(CMT_DBG_FLAG_BIT	, &dbg_flag))
#define SET_DMP_DBG_FLAG(dbg_flag)		(set_bit(DMP_DBG_FLAG_BIT	, &dbg_flag))
#define SET_DLIST_DBG_FLAG(dbg_flag)	(set_bit(DLIST_DBG_FLAG_BIT	, &dbg_flag))
#define SET_DIO_DBG_FLAG(dbg_flag)		(set_bit(DIO_DBG_FLAG_BIT	, &dbg_flag))
#define SET_DC_DBG_FLAG(dbg_flag)		(set_bit(DC_DBG_FLAG_BIT	, &dbg_flag))

#define CLEAR_DMSDEV_DBG_FLAG(dbg_flag)		(clear_bit(DMSDEV_DBG_FLAG_BIT	, &dbg_flag))
#define CLEAR_VOLUME_DBG_FLAG(dbg_flag)		(clear_bit(VOLUME_DBG_FLAG_BIT	, &dbg_flag))
#define CLEAR_IOR_DBG_FLAG(dbg_flag)		(clear_bit(IOR_DBG_FLAG_BIT		, &dbg_flag))
#define CLEAR_LB_DBG_FLAG(dbg_flag)			(clear_bit(LB_DBG_FLAG_BIT		, &dbg_flag))
#define CLEAR_CACHE_DBG_FLAG(dbg_flag)		(clear_bit(CACHE_DBG_FLAG_BIT	, &dbg_flag))
#define CLEAR_OVW_DBG_FLAG(dbg_flag)		(clear_bit(OVW_DBG_FLAG_BIT		, &dbg_flag))
#define CLEAR_META_DBG_FLAG(dbg_flag)		(clear_bit(META_DBG_FLAG_BIT	, &dbg_flag))
#define CLEAR_PALO_DBG_FLAG(dbg_flag)		(clear_bit(PALO_DBG_FLAG_BIT	, &dbg_flag))
#define CLEAR_DNC_DBG_FLAG(dbg_flag)		(clear_bit(DNC_DBG_FLAG_BIT		, &dbg_flag))
#define CLEAR_FSM_DBG_FLAG(dbg_flag)		(clear_bit(FSM_DBG_FLAG_BIT		, &dbg_flag))
#define CLEAR_KP_DBG_FLAG(dbg_flag)			(clear_bit(KP_DBG_FLAG_BIT		, &dbg_flag))
#define CLEAR_CMT_DBG_FLAG(dbg_flag)		(clear_bit(CMT_DBG_FLAG_BIT		, &dbg_flag))
#define CLEAR_DMP_DBG_FLAG(dbg_flag)		(clear_bit(DMP_DBG_FLAG_BIT		, &dbg_flag))
#define CLEAR_DLIST_DBG_FLAG(dbg_flag)		(clear_bit(DLIST_DBG_FLAG_BIT	, &dbg_flag))
#define CLEAR_DIO_DBG_FLAG(dbg_flag)		(clear_bit(DIO_DBG_FLAG_BIT		, &dbg_flag))
#define CLEAR_DC_DBG_FLAG(dbg_flag)			(clear_bit(DC_DBG_FLAG_BIT		, &dbg_flag))

//#define DATAFLOW_DBG_FLAG_BIT		31
//#define DATAFLOW_DBG_FLAG	(1<<DATAFLOW_DBG_FLAG_BIT)
//#define SET_DATAFLOW_DBG_FLAG(dbg_flag)	(set_bit(DATAFLOW_DBG_FLAG_BIT, &dbg_flag))
//#define CLEAR_DATAFLOW_DBG_FLAG(dbg_flag)	(clear_bit(DATAFLOW_DBG_FLAG_BIT, &dbg_flag))

/*
 * why don't use test_bit(), because we don't need it has to be read from memory.
 */
#define DMSDEV_DBG	(dms_dbg_flag & DMSDEV_DBG_FLAG)
#define VOLUME_DBG	(dms_dbg_flag & VOLUME_DBG_FLAG)
#define IOR_DBG		(dms_dbg_flag & IOR_DBG_FLAG)
#define LB_DBG    	(dms_dbg_flag & LB_DBG_FLAG)
#define CACHE_DB  	(dms_dbg_flag & CACHE_DBG_FLAG)
#define OVW_DBG	  	(dms_dbg_flag & OVW_DBG_FLAG)
#define META_DBG   (dms_dbg_flag & META_DBG_FLAG)
#define PALO_DBG   	(dms_dbg_flag & PALO_DBG_FLAG)
#define DNC_DBG    	(dms_dbg_flag & DNC_DBG_FLAG)
#define FSM_DBG	  	(dms_dbg_flag & FSM_DBG_FLAG)
#define KP_DBG    	(dms_dbg_flag & KP_DBG_FLAG)
#define CMT_DBG    	(dms_dbg_flag & CMT_DBG_FLAG)
#define DMP_DBG		(dms_dbg_flag & DMP_DBG_FLAG)
#define DLIST_DBG	(dms_dbg_flag & DLIST_DBG_FLAG)
#define DIO_DBG		(dms_dbg_flag & DIO_DBG_FLAG)
#define DC_DBG		(dms_dbg_flag & DC_DBG_FLAG)



/********************************************************************************/
/*																				*/
/*						DMS GLOBAL DEBUG FLAG DEFINITIONS						*/
/*																				*/
/********************************************************************************/
/*
 * DMS_DEBUG_FLAG controls the whole driver's print output.
 * values:
 * 0: print nothing, but error
 * 1: print by conditions (including the debug flag condition)
 * 2: print all debug message of the module
 */
#if (DMS_DEBUG == 1)
	#define DMS_DEBUG_FLAG	1
#else
	#define DMS_DEBUG_FLAG	0
#endif

//no condition constant
#define NOCON				0
#define ALWAYS				1

//prink debug message
#if (DMS_DEBUG_FLAG == 0)
	#define DMS_PRINTK(condition, fmt, args...)

#elif(DMS_DEBUG_FLAG == 1)
	#define DMS_PRINTK(condition, mod_n, fmt, args...)			if(condition) printk(PFX "%s%s, " fmt, mod_n, __func__, ##args)
//	#define DMS_PRINT_SYM(condition, mod_n, fmt, args...)			if(condition) print_symbol(PFX "%s%s, " fmt, mod_n, __func__, ##args);

#elif(DMS_DEBUG_FLAG == 2)
	#define DMS_PRINTK(condition, mod_n, fmt, args...)			if(DMS_DEBUG_FLAG || condition) printk(PFX "%s%s ," fmt, mod_n, __func__, ##args)
#endif




/********************************************************************************/
/*																				*/
/*									MACROS										*/
/*																				*/
/********************************************************************************/
#define assert(p) do {						\
	if (!(p)) {								\
		cprintk("BUG at %s:%d assert(%s)\n",\
		       __FILE__, __LINE__, #p);		\
		dump_stack();						\
		BUG();								\
	}										\
} while (0)


#define eprintk_fn(fmt, args...) do {		\
	eprintk("%s(%d) " fmt, __FUNCTION__,	\
			__LINE__, ##args);				\
} while (0)


/********************************************************************************/
/*																				*/
/*									FUNCS										*/
/*																				*/
/********************************************************************************/

static inline void __Print_BIO(int index, struct bio *bio){

	printk("\nBIO[%d] info: bi_sector = %llu, bi_vcnt = %u, bi_idx = %u, bi_max_vecs = %u, bi_phys_segments = %u, bi_hw_segments = %u,"
			" bi_size = %u, bio_sectors = %u, bio_segments = %d, cur_sectors = %u \n",
			index, bio->bi_sector, bio->bi_vcnt, bio->bi_idx, bio->bi_max_vecs, bio->bi_phys_segments, bio->bi_hw_segments,
			bio->bi_size, bio_sectors(bio), bio_segments(bio), bio_cur_sectors(bio));
}







#endif /* DEBUG_UTILS_H_ */
