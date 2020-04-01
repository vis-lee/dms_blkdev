/*
 * Copyright (C) 2001 Momchil Velikov
 * Portions Copyright (C) 2001 Christoph Hellwig
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
#ifndef _LINUX_RADIX_TREE_H
#define _LINUX_RADIX_TREE_H

#include <linux/sched.h>
#include <linux/preempt.h>
#include <linux/types.h>

/* root tags are stored in gfp_mask, shifted by __GFP_BITS_SHIFT */
struct radix_tree_root {
	unsigned int		height;
	gfp_t			gfp_mask;
	struct radix_tree_node	*rnode;
};

#define RADIX_TREE_INIT(mask)	{					\
	.height = 0,							\
	.gfp_mask = (mask),						\
	.rnode = NULL,							\
}

#define RADIX_TREE(name, mask) \
	struct radix_tree_root name = RADIX_TREE_INIT(mask)

#define INIT_RADIX_TREE(root, mask)					\
do {									\
	(root)->height = 0;						\
	(root)->gfp_mask = (mask);					\
	(root)->rnode = NULL;						\
} while (0)

int radix_tree_insert(struct radix_tree_root *, unsigned long long,
			void *, void **);
void *radix_tree_lookup(struct radix_tree_root *, unsigned long long);
void **radix_tree_lookup_slot(struct radix_tree_root *, unsigned long long);
void *radix_tree_delete(struct radix_tree_root *, unsigned long long);
unsigned int
radix_tree_gang_lookup(struct radix_tree_root *root, void **results,
			unsigned long long first_index, unsigned int max_items);
int radix_tree_preload(gfp_t gfp_mask);
void radix_tree_init(void);
void radix_tree_exit(void);
void radix_tree_dump(struct radix_tree_node *node, int height);

static inline void radix_tree_preload_end(void)
{
	preempt_enable();
}

#endif /* _LINUX_RADIX_TREE_H */
