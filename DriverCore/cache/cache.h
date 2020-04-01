#ifndef __CCMA_NBD_CACHE_H
#define __CCMA_NBD_CACHE_H

#include <linux/rcupdate.h>
#include <linux/mutex.h>

#include "radix-tree.h"

#define DEBUG	1
#define CACHE_STAT	1
#define CACHE_TEST	1

//#define PAYLOAD_ENABLE	1
#undef PAYLOAD_ENABLE

#if DEBUG
	#define ccma_printk(fmt...)	do { printk(KERN_ERR fmt); } while (0)
#else
	#define ccma_printk(fmt...)	do { } while(0)
#endif

#ifdef CACHE_STAT
struct cache_stat {
	unsigned long m_hit;
	unsigned long m_miss;
	unsigned long p_hit;
	unsigned long p_miss;
};
#endif

struct volume {
	struct hlist_node hlist;

	unsigned long long volid;	// 30 bits
	atomic_t cache_num;		
	unsigned int cache_size;

	struct mutex tree_lock;
	struct radix_tree_root tree;
	struct list_head lru_list;

	/* one spinlock for protecting tree_leaf's reference count */
	spinlock_t tree_leaf_lock;
	
	struct cache_stat stat;
};

struct meta_data;
struct payload;

struct tree_leaf {
	/* must put in the first */
	struct meta_data *data;

	/* least recent used list */
	struct list_head list;

	/* for memory reclaim */
	unsigned long long blkid;

	unsigned int flag_refcnt;
	#define tree_leaf_set_delete(leaf)	(leaf)->flag_refcnt |= (1u<<31)
	#define tree_leaf_is_delete(leaf)	((leaf)->flag_refcnt & (1u<<31))
	#define tree_leaf_ref_count(leaf)	((leaf)->flag_refcnt & (~(1u<<31)))
};

/* 
 *  1. we need to tranverse payload for some peticular volumee, so list. (kick out by lru)
 *  2. we may want to lookup a payload in other volume's payloads, so global hash.
 *  3. recent-used list needed to kick payload out.
 *  4. on kicking, we need to remove from valumn's tranversing list too (kick out by lru).
 */
struct payload_node {
	unsigned long long pbID;
	struct hlist_node hlist;
	struct list_head list;	// lru list
	void *data;
};

/* types used by Invalidate_Metadata_Cache_Item_By_Type,
 * Invalidate_Payload_Cache_Item_By_Type */
#define INVALIDATE_TYPE_RANGE               1
#define INVALIDATE_TYPE_VOLUMEID            2
#define INVALIDATE_TYPE_CODITION_CALLBACK   3

struct Cache_Operations {
	// create volume cache
	int (*Create_Volume_Cache)(unsigned long long volumeID, int cache_size);
	
	// clean meta-data and playload by volume id
	int (*Release_Volume_Cache)(unsigned long long volumeID);

	//invalidate cache item by a LBID range
	int (*Invalidate_Metadata_Cache_Item)(unsigned long long volumeID,
						unsigned long long start_LBID,
						int nr_LBIDs);
	int (*Invalidate_Payload_Cache_Item)(unsigned long long volumeID,
						unsigned long long start_LBID,
						int nr_LBIDs);

	//update cache item by a LBID range
	int (*Update_Metadata_Cache_Item)(unsigned long long volumeID,
					unsigned long long start_LBID,
					int nr_LBIDs,
					struct meta_data **md_array);
	int (*Update_Payload_Cache_Item)(unsigned long long volumeID,
					unsigned long long start_phyLocationID,
					int nr_pl,
					struct payload **pl_array);

	//get cache item by a LBID range
	int (*Get_Metadata_Cache_Item)(unsigned long long volumeID,
					unsigned long long start_LBID,
					int nr_LBIDs,
					struct meta_data **md_array[]);
    	int (*Get_Payload_Cache_Item)(unsigned long long volumeID,
					unsigned long long start_phyLocationID,
					int nr_pl,
					struct payload **pl_array[]);

	//put cache item by a LBID range
	int (*Put_Metadata_Cache_Item)(unsigned long long volumeID,
					unsigned long long start_LBID,
					int nr_LBIDs,
					struct meta_data **md_array[]);
    	int (*Put_Payload_Cache_Item)(unsigned long long volumeID,
					unsigned long long start_phyLocationID,
					int nr_pl,
					struct payload **pl_array[]);
	int (*Invalidate_Metadata_Cache_Item_By_Type)(char type,
					unsigned long long volumeID,
					unsigned long long start_LBID,
					int nr_LBIDs,
					int(*check_condition)(
						struct meta_data *,
						void *)
				);
	int (*Invalidate_Payload_Cache_Item_By_Type)(char type,
					unsigned long long volumeID,
					unsigned long long start_LBID,
					int nr_LBIDs,
					int(*check_condition)(
						struct payload *,
						void *)
				);

};

int Init_Cache_Module(struct Cache_Operations *c_ops);

void Release_Cache_Module(void);

#ifdef CACHE_TEST
void init_test(void);
void exit_test(void);
#endif

#endif
