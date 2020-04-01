#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include "radix-tree.h"

#define CACHE_TEST	1

#include "cache.h"

#ifdef CACHE_TEST
#include <linux/proc_fs.h>

struct proc_dir_entry *test_proc_entry;
#endif

enum {
	CACHE_RADIX_TREE  	= 0,
	CACHE_HASH		= 1,
};
static int cache_mode = CACHE_RADIX_TREE;
module_param(cache_mode, int, 0);

#define VOLID_TABLE_SIZE	23
#define PAYLOAD_TABLE_SIZE	10001

static struct hlist_head volid_table[VOLID_TABLE_SIZE];
static rwlock_t volid_table_lock;	/* protect volid_table */

static struct kmem_cache *tree_leaf_pool = NULL;


#ifdef PAYLOAD_ENABLE
/* TODO this lock is too big */
static rwlock_t payload_table_lock;
static struct hlist_head payload_table[PAYLOAD_TABLE_SIZE];
static spinlock_t payload_lru_lock;
static struct list_head payload_lru_list;

static struct kmem_cache *payload_node_pool = NULL;
#endif

static void (*release_meta_data)(struct meta_data *data) = NULL;
// TODO 
static void __release_meta_data(struct meta_data *data)
{

}

/* functions related to volume cache */

static struct volume *alloc_volume(void)
{
	return kzalloc(sizeof(struct volume), GFP_KERNEL);
}

static void free_volume(struct volume *v)
{
	kfree(v);
}

static struct volume *volume_find(unsigned long long volid)
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct volume *v = NULL;

	// TODO better hash function needed here.
	// TODO % doesn't work on 64-bit integer, change back!
	head = &volid_table[((unsigned long)volid)%VOLID_TABLE_SIZE];

	read_lock(&volid_table_lock);
	hlist_for_each_entry(v, node, head, hlist) {
		if (v->volid == volid)
			goto unlock;
	}
unlock:
	read_unlock(&volid_table_lock);
	if (likely(v != NULL && v->volid == volid))
		return v;
	return NULL;
}

static void volume_register(struct volume *v)
{
	INIT_HLIST_NODE(&v->hlist);
	
	write_lock(&volid_table_lock);
	// TODO better hash function
	// TODO % doesn't work on 64-bit integer, change back!
	hlist_add_head(&v->hlist,
		&volid_table[((unsigned long)v->volid)%VOLID_TABLE_SIZE]);
	write_unlock(&volid_table_lock);
}

static void volume_remove_by_id(unsigned long long volid)
{
	struct hlist_head *head;
	struct hlist_node *node, *tmp;
	struct volume *v = NULL;

	// TODO better hash function needed here.
	// TODO % doesn't work on 64-bit integer, change back!
	head = &volid_table[((unsigned long)volid)%VOLID_TABLE_SIZE];

	write_lock(&volid_table_lock);
	hlist_for_each_entry_safe(v, node, tmp, head, hlist) {
		if (v->volid == volid) {
			hlist_del(node);
			goto unlock;
		}
	}
unlock:
	write_unlock(&volid_table_lock);
	if (v != NULL && v->volid == volid)
		free_volume(v);
}

static void kick_meta_cache(unsigned long long vid,
				int count,
				int(*check_condition)(
					struct meta_data *,
					void*)
				);
static void __kick_meta_cache(struct volume *cache,
				int count,
				int(*check_condition)(
					struct meta_data *,
					void *)
				);

static void volume_remove_all(void)
{
	int i;
	struct hlist_head *head;
	struct hlist_node *node, *tmp;
	struct volume *v = NULL;

	ccma_printk("%s, getting table lock\n", __func__);
	write_lock(&volid_table_lock);
	ccma_printk("%s, got table lock\n", __func__);
	for (i = 0; i < VOLID_TABLE_SIZE; ++i) {
		head = &volid_table[i];
		hlist_for_each_entry_safe(v, node, tmp, head, hlist) {
			ccma_printk("%s, handle table %d\n",
						__func__,
						(int)v->volid);
			hlist_del(&v->hlist);
			__kick_meta_cache(v, -1, NULL);
			ccma_printk("%d: left cache_num %ld\n",
					(int)v->volid,
					(unsigned long)atomic_read(&v->cache_num));
			//radix_tree_shrink(&v->tree);
			ccma_printk("tree->height %d\n", v->tree.height);
			free_volume(v);
		}
	}
	write_unlock(&volid_table_lock);
	ccma_printk("%s, release lock & leaving\n", __func__);
}

// caller is responsible for not creating same volume twice.
static int volume_create(unsigned long long volumeID, int cache_size)
{
	struct volume *v = alloc_volume();
	if (unlikely(v == NULL))
		return -ENOMEM;

	v->volid = volumeID;
	v->cache_size = cache_size;

	INIT_RADIX_TREE(&v->tree, GFP_KERNEL);
	INIT_LIST_HEAD(&v->lru_list);
	mutex_init(&v->tree_lock);

	spin_lock_init(&v->tree_leaf_lock);

	volume_register(v);

	return 0;
}

/* functions related to payload cache */

#ifdef PAYLOAD_ENABLE
static inline void __payload_touch(struct payload_node *p)
{
	spin_lock(&payload_lru_lock);
	list_del(&p->list);
	list_add(&p->list, &payload_lru_list);
	spin_unlock(&payload_lru_lock);
}

static void *__payload_find(unsigned long long pbID)
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct payload_node *p = NULL;


	// TODO better hash function needed here.
	// TODO % doesn't work on 64-bit integer, change back!
	head = &payload_table[((unsigned long)pbID)%PAYLOAD_TABLE_SIZE];

	read_lock(&payload_table_lock);
	hlist_for_each_entry(p, node, head, hlist) {
		if (p->pbID == pbID)
			goto unlock;
	}
unlock:
	read_unlock(&payload_table_lock);
	if (likely(p != NULL && p->pbID == pbID)) {
		__payload_touch(p);
		return p;
	}
	return NULL;
}

static int payload_find(unsigned long long vid,
			unsigned long long start,
			int count,
			struct payload **pl_array[])
{
	int i, off = 0;

	for (i = 0; i < count; ++i) {
		void *data = __payload_find(i + start);
		if (!data)
			continue;
		pl_array[off++] = data;
	}
	return off;
}

static void __payload_invalidate(unsigned long long pbID)
{
	struct hlist_head *head;
	struct hlist_node *node, *tmp;
	struct payload_node *p = NULL;

	// TODO better hash function needed here.
	// TODO % doesn't work on 64-bit integer, change back!
	head = &payload_table[((unsigned long)pbID)%PAYLOAD_TABLE_SIZE];

	write_lock(&payload_table_lock);
	hlist_for_each_entry_safe(p, node, tmp, head, hlist) {
		if (p->pbID == pbID) {
			hlist_del(node);
			goto unlock;
		}
	}
unlock:
	write_unlock(&payload_table_lock);
	if (p != NULL && p->pbID == pbID) {
		// TODO how to free p->data?
		kmem_cache_free(payload_node_pool, p);
	}
}

static int payload_invalidate(unsigned long long vid,
				unsigned long long start,
				int count)
{
	int i;
	for (i = 0; i < count; ++i)
		__payload_invalidate(i + start);
	return 0;
}

static int payload_add(unsigned long long vid,
			unsigned long long pbID,
			int nr,
			struct payload **data)
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct payload_node **p, *pp;
	int i, ret = -ENOMEM;

	p = kzalloc(nr*sizeof(struct payload_node *), GFP_KERNEL);
	if (unlikely(p == NULL))
		return -ENOMEM;

	for (i = 0; i < nr; ++i) {
		p[i] = kmem_cache_alloc(payload_node_pool, GFP_KERNEL);
		if (unlikely(p[i] == NULL))
			goto free_array;
		INIT_HLIST_NODE(&p[i]->hlist);
		INIT_LIST_HEAD(&p[i]->list);
		p[i]->data = data[i];
	}

	write_lock(&payload_table_lock);
	for (i = 0; i < nr; ++i) {
		// TODO better hash function needed here.
		head = &payload_table[(pbID+i)%PAYLOAD_TABLE_SIZE];
		hlist_for_each_entry(pp, node, head, hlist) {
			if (pp->pbID == (pbID+i)) {
				pp->data = data[i];
				kmem_cache_free(payload_node_pool, p[i]);
				continue;
			}
		}
		hlist_add_head(&p[i]->hlist, head);
	}
	write_unlock(&payload_table_lock);
	ret = 0;
free_array:
	kfree(p);
	return ret;
}

static int payload_update(unsigned long long pbID, void *data)
{
	struct hlist_head *head;
	struct hlist_node *node;
	struct payload_node *p;
	int ret = 0;

	p = kmem_cache_alloc(payload_node_pool, GFP_KERNEL);
	if (unlikely(p == NULL))
		return -ENOMEM;

	// TODO better hash function needed here.
	head = &payload_table[pbID%PAYLOAD_TABLE_SIZE];

	write_lock(&payload_table_lock);
	hlist_for_each_entry(p, node, head, hlist) {
		if (p->pbID == pbID) {
			// TODO how to free p->data?
			p->data = data;
			goto unlock;
		}
	}
	ret = -ENOENT;
unlock:
	write_unlock(&payload_table_lock);
	return ret;
}

int (*Invalidate_Payload_Cache_Item_By_Type)(char type,
				unsigned long long volumeID,
				unsigned long long start_LBID,
				int nr_LBIDs,
				int(*check_condition)(
					struct payload *,
					void *)
			);

static int payload_put(unsigned long long volumeID,
			unsigned long long start,
			int count,
			struct payload **pl_array[])
{
	return 0;
}


#endif

/* functions related to meta-data cache */

static inline void
release_tree_leaf(struct tree_leaf *leaf)
{
	BUG_ON(tree_leaf_ref_count(leaf) > 0);
	release_meta_data(leaf->data);
	kmem_cache_free(tree_leaf_pool, leaf);
}

static int cache_find(unsigned long long volid,
			unsigned long long start,
			int count,
			struct meta_data **basket[])
{
	struct volume *cache;
	int ret = 0, i, rret = 0;
	unsigned long long j, ret_off;

	cache = volume_find(volid);
	if (unlikely(cache == NULL))
		return -1;

	for (i = 0; i < count; ++i)
		basket[i] = NULL;

	mutex_lock(&cache->tree_lock);

	// borrow basket to store tree_leaf pointers temporarily */
	ret = radix_tree_gang_lookup(&cache->tree, (void **)basket,
					start, count);
	if (ret > 0) {
		for (i = ret; i < count; ++i)
			basket[i] = NULL;
		ret_off = start + count;
		for (i = ret-1; i >= 0; --i) {
			struct tree_leaf *leaf = (void *)basket[i];
			BUG_ON(leaf == NULL);

			j = leaf->blkid;
			if (j >= start && j < ret_off) {
				basket[i] = NULL;
				basket[j-start] = (void *)&leaf->data;

				spin_lock(&cache->tree_leaf_lock);
				++leaf->flag_refcnt;
				spin_unlock(&cache->tree_leaf_lock);

				ret_off = j;
				++rret;

				list_del(&leaf->list);
				list_add(&leaf->list, &cache->lru_list);

			} else {
				/* radix-tree's bug */
				basket[i] = NULL;
			}
		}
	} else {
		for (i = 0; i < count; ++i)
			basket[i] = NULL;
		goto out;
	}

#ifdef CACHE_STAT
	cache->stat.m_hit += ret;
	cache->stat.m_miss += count-ret;
#endif
out:
	mutex_unlock(&cache->tree_lock);
	return rret;
}

static int cache_put(unsigned long long volid,
			unsigned long long start,
			int count,
			struct meta_data **basket[])
{
	struct volume *cache;
	int i;

	cache = volume_find(volid);
	if (unlikely(cache == NULL))
		return -1;

	spin_lock(&cache->tree_leaf_lock);
	for (i = 0; i < count; ++i) {
		struct tree_leaf *leaf = (void *)basket[i];
		if (leaf != NULL) {
			--leaf->flag_refcnt;
			if (likely(!tree_leaf_is_delete(leaf)))
				continue;
			if (tree_leaf_ref_count(leaf) > 0)
				continue;

			spin_unlock(&cache->tree_leaf_lock);
			release_tree_leaf(leaf);
			spin_lock(&cache->tree_leaf_lock);
		}
	}
	spin_unlock(&cache->tree_leaf_lock);

	return 0;
}



static int cache_invalidate(unsigned long long volid,
				unsigned long long start,
				int count)
{
	struct volume *cache;
	int i;

	cache = volume_find(volid);
	if (unlikely(cache == NULL))
		return 0;

	mutex_lock(&cache->tree_lock);
	for (i = 0; i < count; ++i) {
		struct tree_leaf *leaf = (struct tree_leaf *)
			radix_tree_delete(&cache->tree, start + i);
		if (leaf) {
			list_del(&leaf->list);
			atomic_dec(&cache->cache_num);
			spin_lock(&cache->tree_leaf_lock);
			if (tree_leaf_ref_count(leaf) > 0) {
				tree_leaf_set_delete(leaf);
			} else {
				spin_unlock(&cache->tree_leaf_lock);
				release_tree_leaf(leaf);
				spin_lock(&cache->tree_leaf_lock);
			}
			spin_unlock(&cache->tree_leaf_lock);
		}
	}
	mutex_unlock(&cache->tree_lock);

	return 0;
}

/* should hold lock */
static void __kick_meta_cache(struct volume *cache,
				int count,
				int(*check_condition)(
					struct meta_data *,
					void *)
				)
{
	struct tree_leaf *leaf, *node;
	unsigned int nr = 0, total = count;

	if (count == -1 || count > atomic_read(&cache->cache_num))
		total = atomic_read(&cache->cache_num);

	//ccma_printk("%s: kick %ld\n", __func__,
	//		       (unsigned long)total);

	list_for_each_entry_safe_reverse(leaf, node, &cache->lru_list, list) {

		if (check_condition && check_condition(leaf->data, NULL))
			continue;

		radix_tree_delete(&cache->tree, leaf->blkid);
		
		list_del(&leaf->list);
		atomic_dec(&cache->cache_num);

		spin_lock(&cache->tree_leaf_lock);
		if (tree_leaf_ref_count(leaf) > 0) {
			tree_leaf_set_delete(leaf);
			spin_unlock(&cache->tree_leaf_lock);
		} else {
			spin_unlock(&cache->tree_leaf_lock);
			release_tree_leaf(leaf);
		}

		if (count != -1 && ++nr >= total)
			break;
	}

	/*
	if (count == -1)
		radix_tree_dump(cache->tree.rnode, cache->tree.height);
	*/
}


/* kick out @count number of meta-cache in volume @vid 
 * if @count == -1, then kick all.
 */
static void kick_meta_cache(unsigned long long vid,
			int count,
			int (*check_condition)(
				struct meta_data *,
				void*)
			)
{
	struct volume *cache;

	cache = volume_find(vid);
	if (unlikely(cache == NULL))
		return;

	__kick_meta_cache(cache, count, check_condition);
}

static int cache_update(unsigned long long volid,
			unsigned long long start,
			int count,
			struct meta_data **basket)
{
	struct volume *cache;
	int ret = -ENOMEM, i;
	struct tree_leaf *leaf, *old_leaf;

	cache = volume_find(volid);
	if (unlikely(cache == NULL))
		return -1;

	mutex_lock(&cache->tree_lock);

	/* reclaim some memory */
	i = atomic_read(&cache->cache_num) + count - cache->cache_size;
	if (i > (int)cache->cache_size)
		i = cache->cache_size;
	//ccma_printk("%s: cache_num/cache_size = %ld/%ld\n",
	//		__func__,
	//		(unsigned long)atomic_read(&cache->cache_num),
	//		(unsigned long)cache->cache_size);
	if (i > 0)
		__kick_meta_cache(cache, i, NULL);

	for (i = 0; i < count; ++i) {
		
		leaf = kmem_cache_alloc(tree_leaf_pool, GFP_KERNEL);
		if (unlikely(leaf == NULL))
			goto out;

		leaf->data = basket[i];
		leaf->blkid = start + i;
		leaf->flag_refcnt = 0;
		INIT_LIST_HEAD(&leaf->list);

		old_leaf = NULL;

		ret = radix_tree_insert(&cache->tree,
					start + i, leaf, (void **)&old_leaf);
		if (unlikely(ret < 0)) {
			kmem_cache_free(tree_leaf_pool, leaf);
			goto out;
		}

		list_add(&leaf->list, &cache->lru_list);
		atomic_inc(&cache->cache_num);

		leaf = old_leaf;
		if (unlikely(leaf != NULL)) {
			list_del(&leaf->list);
			atomic_dec(&cache->cache_num);
			spin_lock(&cache->tree_leaf_lock);
			if (tree_leaf_ref_count(leaf) > 0) {
				tree_leaf_set_delete(leaf);
				spin_unlock(&cache->tree_leaf_lock);
			} else {
				spin_unlock(&cache->tree_leaf_lock);
				release_tree_leaf(leaf);
			}
		}
	}

	ret = 0;
out:
	mutex_unlock(&cache->tree_lock);
	return ret;
}


static int volume_clear_all(unsigned long long vid)
{
	/* TODO kick all payloads */
	kick_meta_cache(vid, -1, NULL);

	volume_remove_by_id(vid);

	return 0;
}

int cache_invalidate_by_type(char type,
			unsigned long long volumeID,
			unsigned long long start_LBID,
			int nr_LBIDs,
			int(*check_condition)(
				struct meta_data *,
				void * )
			)
{
	if (type == INVALIDATE_TYPE_VOLUMEID) {
		kick_meta_cache(volumeID, -1, NULL);
		return 0;
	}

	if (type == INVALIDATE_TYPE_CODITION_CALLBACK) {
		BUG_ON(!check_condition);
		kick_meta_cache(volumeID, -1, check_condition);
		return 0;
	}

	if (type == INVALIDATE_TYPE_RANGE)
		return cache_invalidate(volumeID, start_LBID, nr_LBIDs);

	return 0;
}

#ifdef CACHE_TEST
int test_read(char *buffer, char **buffer_location,
		off_t offset, int buffer_length,
		int *eof, void *data)
{
	init_test();
	return 0;
}
#endif

static int cache_init(void)
{
	int i;

	rwlock_init(&volid_table_lock);
	for (i = 0; i < VOLID_TABLE_SIZE; ++i)
		INIT_HLIST_HEAD(&volid_table[i]);

	tree_leaf_pool = kmem_cache_create("DMS-client/tree_leaf",
					sizeof(struct tree_leaf),
					0, 0, NULL, NULL);
	if (!tree_leaf_pool)
		return -ENOMEM;

#ifdef PAYLOAD_ENABLE
	rwlock_init(&payload_table_lock);
	spin_lock_init(&payload_lru_lock);
	INIT_LIST_HEAD(&payload_lru_list);
	for (i = 0; i < PAYLOAD_TABLE_SIZE; ++i)
		INIT_HLIST_HEAD(&payload_table[i]);

	payload_node_pool = kmem_cache_create("DMS-client/payload_node",
					sizeof(struct payload_node),
					0, 0, NULL, NULL);
	if (!payload_node_pool)
		goto free_tree_leaf_pool;
#endif

#ifdef CACHE_TEST
	test_proc_entry = create_proc_entry("ccma_dms_test", 0644, NULL);
	if (test_proc_entry == NULL) {
		remove_proc_entry("ccma_dms_test", &proc_root);
		// simply return ..
		return -ENOMEM;
	}
	test_proc_entry->read_proc = test_read;
#endif

	return 0;

#ifdef PAYLOAD_ENABLE
free_tree_leaf_pool:
#endif
	kmem_cache_destroy(tree_leaf_pool);

	return -ENOMEM;
}

int Init_Cache_Module(struct Cache_Operations *c_ops)
{
	c_ops->Create_Volume_Cache 		= volume_create;
	c_ops->Release_Volume_Cache 		= volume_clear_all;
#ifdef PAYLOAD_ENABLE
	c_ops->Invalidate_Payload_Cache_Item	= payload_invalidate;
	c_ops->Update_Payload_Cache_Item	= payload_add;
	c_ops->Get_Payload_Cache_Item		= payload_find;
	c_ops->Put_Payload_Cache_Item		= payload_put;
#endif
	c_ops->Update_Metadata_Cache_Item	= cache_update;
	c_ops->Get_Metadata_Cache_Item		= cache_find;
	c_ops->Put_Metadata_Cache_Item		= cache_put;
	c_ops->Invalidate_Metadata_Cache_Item_By_Type	= cache_invalidate_by_type;
	c_ops->Invalidate_Metadata_Cache_Item 	= cache_invalidate;

	// TODO
	release_meta_data = __release_meta_data;

	radix_tree_init();

#ifndef CACHE_TEST
	return cache_init();	
#else
	return 0;
#endif
}

void Release_Cache_Module(void)
{
	synchronize_rcu();
	volume_remove_all();
	synchronize_rcu();
	kmem_cache_destroy(tree_leaf_pool);
#ifdef PAYLOAD_ENABLE
	kmem_cache_destroy(payload_node_pool);
#endif
	radix_tree_exit();
#ifdef CACHE_TEST
	remove_proc_entry("ccma_dms_test", &proc_root);
	exit_test();
#endif

}
	
module_init(cache_init);
module_exit(Release_Cache_Module);

MODULE_LICENSE("Dual BSD/GPL");
