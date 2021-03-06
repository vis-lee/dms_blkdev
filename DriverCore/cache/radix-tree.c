/*
 * Copyright (C) 2001 Momchil Velikov
 * Portions Copyright (C) 2001 Christoph Hellwig
 * Copyright (C) 2005 SGI, Christoph Lameter <clameter@sgi.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/percpu.h>
#include <linux/slab.h>
#include <linux/notifier.h>
#include <linux/cpu.h>
#include <linux/gfp.h>
#include <linux/string.h>
#include <linux/bitops.h>

#include "radix-tree.h"

#ifdef __KERNEL__
#define RADIX_TREE_MAP_SHIFT	6
#else
#define RADIX_TREE_MAP_SHIFT	3	/* For more stressful testing */
#endif

#define RADIX_TREE_MAP_SIZE	(1ULL << RADIX_TREE_MAP_SHIFT)
#define RADIX_TREE_MAP_MASK	(RADIX_TREE_MAP_SIZE-1)

struct radix_tree_node {
	unsigned int	count;
	void		*slots[RADIX_TREE_MAP_SIZE];
};

struct radix_tree_path {
	struct radix_tree_node *node;
	int offset;
};

#define RADIX_TREE_INDEX_BITS  (8 /* CHAR_BIT */ * sizeof(unsigned long long))
#define RADIX_TREE_MAX_PATH (RADIX_TREE_INDEX_BITS/RADIX_TREE_MAP_SHIFT + 2)

static unsigned long long height_to_maxindex[RADIX_TREE_MAX_PATH] __read_mostly;

/*
 * Radix tree node cache.
 */
static kmem_cache_t *radix_tree_node_cachep;

/*
 * Per-cpu pool of preloaded nodes
 */
struct radix_tree_preload {
	int nr;
	struct radix_tree_node *nodes[RADIX_TREE_MAX_PATH];
};
DEFINE_PER_CPU(struct radix_tree_preload, radix_tree_preloads) = { 0, };

static inline gfp_t root_gfp_mask(struct radix_tree_root *root)
{
	return root->gfp_mask & __GFP_BITS_MASK;
}

// yifeng
static atomic_t node_count;

/*
 * This assumes that the caller has performed appropriate preallocation, and
 * that the caller has pinned this thread of control to the current CPU.
 */
static struct radix_tree_node *
radix_tree_node_alloc(struct radix_tree_root *root)
{
	struct radix_tree_node *ret;
	gfp_t gfp_mask = root_gfp_mask(root);

	ret = kmem_cache_alloc(radix_tree_node_cachep, gfp_mask);
	if (ret == NULL && !(gfp_mask & __GFP_WAIT)) {
		struct radix_tree_preload *rtp;

		rtp = &__get_cpu_var(radix_tree_preloads);
		if (rtp->nr) {
			ret = rtp->nodes[rtp->nr - 1];
			rtp->nodes[rtp->nr - 1] = NULL;
			rtp->nr--;
		}
	}
	atomic_inc(&node_count);
	return ret;
}

static inline void
radix_tree_node_free(struct radix_tree_node *node)
{
	atomic_dec(&node_count);
	kmem_cache_free(radix_tree_node_cachep, node);
}

/*
 * Load up this CPU's radix_tree_node buffer with sufficient objects to
 * ensure that the addition of a single element in the tree cannot fail.  On
 * success, return zero, with preemption disabled.  On error, return -ENOMEM
 * with preemption not disabled.
 */
int radix_tree_preload(gfp_t gfp_mask)
{
	struct radix_tree_preload *rtp;
	struct radix_tree_node *node;
	int ret = -ENOMEM;

	preempt_disable();
	rtp = &__get_cpu_var(radix_tree_preloads);
	while (rtp->nr < ARRAY_SIZE(rtp->nodes)) {
		preempt_enable();
		node = kmem_cache_alloc(radix_tree_node_cachep, gfp_mask);
		if (node == NULL)
			goto out;
		preempt_disable();
		rtp = &__get_cpu_var(radix_tree_preloads);
		if (rtp->nr < ARRAY_SIZE(rtp->nodes))
			rtp->nodes[rtp->nr++] = node;
		else
			kmem_cache_free(radix_tree_node_cachep, node);
	}
	ret = 0;
out:
	return ret;
}

/*
 *	Return the maximum key which can be store into a
 *	radix tree with height HEIGHT.
 */
static inline unsigned long long radix_tree_maxindex(unsigned int height)
{
	return height_to_maxindex[height];
}

/*
 *	Extend a radix tree so it can store key @index.
 */
static int radix_tree_extend(struct radix_tree_root *root,
			unsigned long long index)
{
	struct radix_tree_node *node;
	unsigned int height;

	/* Figure out what the height should be.  */
	height = root->height + 1;
	while (index > radix_tree_maxindex(height))
		height++;

	if (root->rnode == NULL) {
		root->height = height;
		goto out;
	}

	do {
		if (!(node = radix_tree_node_alloc(root)))
			return -ENOMEM;

		/* Increase the height.  */
		node->slots[0] = root->rnode;

		node->count = 1;
		root->rnode = node;
		root->height++;
	} while (height > root->height);
out:
	return 0;
}

/**
 *	radix_tree_insert    -    insert into a radix tree
 *	@root:		radix tree root
 *	@index:		index key
 *	@item:		item to insert
 *
 *	Insert an item into the radix tree at position @index.
 */
int radix_tree_insert(struct radix_tree_root *root,
			unsigned long long index, void *item,
			void **item_replaced)
{
	struct radix_tree_node *node = NULL, *slot;
	unsigned int height, shift;
	int offset;
	int error;

	/* Make sure the tree is high enough.  */
	if (index > radix_tree_maxindex(root->height)) {
		error = radix_tree_extend(root, index);
		if (error)
			return error;
	}

	slot = root->rnode;
	height = root->height;
	shift = (height-1) * RADIX_TREE_MAP_SHIFT;

	offset = 0;			/* uninitialised var warning */
	while (height > 0) {
		if (slot == NULL) {
			/* Have to add a child node.  */
			if (!(slot = radix_tree_node_alloc(root)))
				return -ENOMEM;
			if (node) {
				node->slots[offset] = slot;
				node->count++;
			} else
				root->rnode = slot;
		}

		/* Go a level down */
		offset = (index >> shift) & RADIX_TREE_MAP_MASK;
		node = slot;
		slot = node->slots[offset];
		shift -= RADIX_TREE_MAP_SHIFT;
		height--;
	}

	if (slot != NULL)
		*item_replaced = slot;

	if (node) {
		if (slot == NULL)
			node->count++;
		node->slots[offset] = item;
	} else {
		root->rnode = item;
	}

	return 0;
}

static inline void **__lookup_slot(struct radix_tree_root *root,
				   unsigned long long index)
{
	unsigned int height, shift;
	struct radix_tree_node **slot;

	height = root->height;

	if (index > radix_tree_maxindex(height))
		return NULL;

	if (height == 0 && root->rnode)
		return (void **)&root->rnode;

	shift = (height-1) * RADIX_TREE_MAP_SHIFT;
	slot = &root->rnode;

	while (height > 0) {
		if (*slot == NULL)
			return NULL;

		slot = (struct radix_tree_node **)
			((*slot)->slots +
				((index >> shift) & RADIX_TREE_MAP_MASK));
		shift -= RADIX_TREE_MAP_SHIFT;
		height--;
	}

	return (void **)slot;
}

/**
 *	radix_tree_lookup_slot    -    lookup a slot in a radix tree
 *	@root:		radix tree root
 *	@index:		index key
 *
 *	Lookup the slot corresponding to the position @index in the radix tree
 *	@root. This is useful for update-if-exists operations.
 */
void **radix_tree_lookup_slot(struct radix_tree_root *root,
				unsigned long long index)
{
	return __lookup_slot(root, index);
}

/**
 *	radix_tree_lookup    -    perform lookup operation on a radix tree
 *	@root:		radix tree root
 *	@index:		index key
 *
 *	Lookup the item at the position @index in the radix tree @root.
 */
void *radix_tree_lookup(struct radix_tree_root *root, unsigned long long index)
{
	void **slot;

	slot = __lookup_slot(root, index);
	return slot != NULL ? *slot : NULL;
}

void radix_tree_dump(struct radix_tree_node *node, int height)
{
	unsigned long i;

	if (!node)
		return;

	printk(KERN_ERR"count %4d\n", node->count);
	for (i = 0; i < RADIX_TREE_MAP_SIZE; ++i) {
		if (node->slots[i]) {
			if (height == 1)
				printk(KERN_ERR"leaf %2ld %p\n", i,
						node->slots[i]);
			else {
				printk(KERN_ERR"node %2d %p\n", height,
						node->slots[i]);
				radix_tree_dump(node->slots[i], height-1);
			}
		}
	}
}

static unsigned int
__lookup(struct radix_tree_root *root, void **results,
		unsigned long long index,
		unsigned int max_items,
		unsigned long long *next_index)
{
	unsigned int nr_found = 0;
	unsigned int shift, height;
	struct radix_tree_node *slot;
	unsigned long long i;

	height = root->height;
	if (height == 0) {
		if (root->rnode && index == 0)
			results[nr_found++] = root->rnode;
		goto out;
	}

	shift = (height-1) * RADIX_TREE_MAP_SHIFT;
	slot = root->rnode;

	for ( ; height > 1; height--) {

		for (i = (index >> shift) & RADIX_TREE_MAP_MASK ;
				i < RADIX_TREE_MAP_SIZE; i++) {
			if (slot->slots[i] != NULL)
				break;
			index &= ~((1ULL << shift) - 1);
			index += 1ULL << shift;
			if (index == 0)
				goto out;	/* 32-bit wraparound */
		}
		if (i == RADIX_TREE_MAP_SIZE)
			goto out;

		shift -= RADIX_TREE_MAP_SHIFT;
		slot = slot->slots[i];
	}

	/* Bottom level: grab some items */
	for (i = index & RADIX_TREE_MAP_MASK; i < RADIX_TREE_MAP_SIZE; i++) {
		index++;
		if (slot->slots[i]) {
			results[nr_found++] = slot->slots[i];
			if (nr_found == max_items)
				goto out;
		}
	}
out:
	*next_index = index;
	return nr_found;
}

/**
 *	radix_tree_gang_lookup - perform multiple lookup on a radix tree
 *	@root:		radix tree root
 *	@results:	where the results of the lookup are placed
 *	@first_index:	start the lookup from this key
 *	@max_items:	place up to this many items at *results
 *
 *	Performs an index-ascending scan of the tree for present items.  Places
 *	them at *@results and returns the number of items which were placed at
 *	*@results.
 *
 *	The implementation is naive.
 */
unsigned int
radix_tree_gang_lookup(struct radix_tree_root *root, void **results,
			unsigned long long first_index, unsigned int max_items)
{
	const unsigned long long max_index = radix_tree_maxindex(root->height);
	unsigned long long cur_index = first_index;
	unsigned int ret = 0;

	while (ret < max_items) {
		unsigned int nr_found;
		unsigned long long next_index;	/* Index of next search */

		if (cur_index > max_index)
			break;
		nr_found = __lookup(root, results + ret, cur_index,
					max_items - ret, &next_index);
		ret += nr_found;
		if (next_index == 0)
			break;
		cur_index = next_index;
	}
	return ret;
}

/**
 *	radix_tree_shrink    -    shrink height of a radix tree to minimal
 *	@root		radix tree root
 */
static inline void radix_tree_shrink(struct radix_tree_root *root)
{
	/* try to shrink tree height */
	while (root->height > 0 &&
			root->rnode->count == 1 &&
			root->rnode->slots[0]) {
		struct radix_tree_node *to_free = root->rnode;

		root->rnode = to_free->slots[0];
		root->height--;
		/* must only free zeroed nodes into the slab */
		to_free->slots[0] = NULL;
		to_free->count = 0;
		radix_tree_node_free(to_free);
	}
}

/**
 *	radix_tree_delete    -    delete an item from a radix tree
 *	@root:		radix tree root
 *	@index:		index key
 *
 *	Remove the item at @index from the radix tree rooted at @root.
 *
 *	Returns the address of the deleted item, or NULL if it was not present.
 */
void *radix_tree_delete(struct radix_tree_root *root,
			unsigned long long index)
{
	struct radix_tree_path path[RADIX_TREE_MAX_PATH], *pathp = path;
	struct radix_tree_node *slot = NULL;
	unsigned int height, shift;
	int offset;

	height = root->height;
	if (index > radix_tree_maxindex(height))
		goto out;

	slot = root->rnode;
	if (height == 0 && root->rnode) {
		root->rnode = NULL;
		goto out;
	}

	shift = (height - 1) * RADIX_TREE_MAP_SHIFT;
	pathp->node = NULL;

	do {
		if (slot == NULL)
			goto out;

		pathp++;
		offset = (index >> shift) & RADIX_TREE_MAP_MASK;
		pathp->offset = offset;
		pathp->node = slot;
		slot = slot->slots[offset];
		shift -= RADIX_TREE_MAP_SHIFT;
		height--;
	} while (height > 0);

	if (slot == NULL)
		goto out;

	/* Now free the nodes we do not need anymore */
	while (pathp->node) {
		pathp->node->slots[pathp->offset] = NULL;
		pathp->node->count--;

		if (pathp->node->count) {
			if (pathp->node == root->rnode)
				radix_tree_shrink(root);
			goto out;
		}

		radix_tree_node_free(pathp->node);
		pathp--;
	}

	root->height = 0;
	root->rnode = NULL;
out:
	return slot;
}

static void
radix_tree_node_ctor(void *node, kmem_cache_t *cachep, unsigned long flags)
{
	memset(node, 0, sizeof(struct radix_tree_node));
}

static unsigned long long __maxindex(unsigned int height)
{
	unsigned int tmp = height * RADIX_TREE_MAP_SHIFT;
	unsigned long long index =
		(~0ULL >> (RADIX_TREE_INDEX_BITS - tmp - 1)) >> 1;

	if (tmp >= RADIX_TREE_INDEX_BITS)
		index = ~0ULL;
	return index;
}

static void radix_tree_init_maxindex(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(height_to_maxindex); i++) {
		height_to_maxindex[i] = __maxindex(i);
	}
}

#if 0
#ifdef CONFIG_HOTPLUG_CPU
static int radix_tree_callback(struct notifier_block *nfb,
                            unsigned long action,
                            void *hcpu)
{
       int cpu = (long)hcpu;
       struct radix_tree_preload *rtp;

       /* Free per-cpu pool of perloaded nodes */
       if (action == CPU_DEAD) {
               rtp = &per_cpu(radix_tree_preloads, cpu);
               while (rtp->nr) {
                       kmem_cache_free(radix_tree_node_cachep,
                                       rtp->nodes[rtp->nr-1]);
                       rtp->nodes[rtp->nr-1] = NULL;
                       rtp->nr--;
               }
       }
       return NOTIFY_OK;
}
#endif /* CONFIG_HOTPLUG_CPU */
#endif

void radix_tree_init(void)
{
	radix_tree_node_cachep = kmem_cache_create("DMS-client/radix_tree_node",
			sizeof(struct radix_tree_node), 0,
			SLAB_PANIC, radix_tree_node_ctor, NULL);
	radix_tree_init_maxindex();
	//hotcpu_notifier(radix_tree_callback, 0);
}

void radix_tree_exit(void)
{
	struct radix_tree_preload *rtp;
	int cpu, i;

	printk(KERN_ERR"node_count %d\n", (int)atomic_read(&node_count));

	if (!radix_tree_node_cachep)
		return;
	for_each_present_cpu(cpu) {
 		rtp = &per_cpu(radix_tree_preloads, cpu);
		printk(KERN_ERR"%s: %d per_cpu %lx\n", __func__,
				cpu,
				(unsigned long)rtp);
		printk(KERN_ERR"nr %d\n", rtp->nr);
		for (i = 0; i < rtp->nr; ++i) {
			kmem_cache_free(radix_tree_node_cachep, rtp->nodes[i]);
		}
	}
	kmem_cache_destroy(radix_tree_node_cachep);
	//remove_hotcpu_notifier(radix_tree_callback, 0);
}
