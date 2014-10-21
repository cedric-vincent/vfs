/* -*- c-set-style: "K&R"; c-basic-offset: 8 -*-
 *
 * This file is part of PRoot.
 *
 * Copyright (C) 2014 STMicroelectronics
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301 USA.
 */

#ifndef PROOT_VFS_NODE
#define PROOT_VFS_NODE

#include <stdbool.h>	/* bool, */

#include <talloc.h>
#include <uthash.h>

typedef enum {
	ERROR = 0,	/* missing initialization.  */
	BINDING,	/* remap a host path over a guest path.  */
	FILLER,		/* fill gap between a guest leaf and a binding leaf.  */
	CACHE,		/* remember translation result.  */
} NodePurpose;

typedef enum {
	DEFAULT = 0,	/* Trust the actual file-system.  */
	ADDED,		/* Add to parent's dentries.  */
	REMOVED,	/* Hide from parent's dentries.  */
} NodeVisibility;

struct node;

/* Resolve a virtual node, as in /proc.  */
typedef int (* NodeEvaluator)(struct node *node);

typedef struct node
{
	/* Short name of this node.  */
	const char *name;

	/* Avoid several calls to stat(2).  */
	int type;

	/* Either avoid several calls to readlink(2), or used for
	 * binding purpose.  */
	const char *value;

	/* See type definitions above.  */
	NodePurpose purpose;
	NodeVisibility visibility;
	NodeEvaluator evaluator;

	/* Make this structure hashable, key is self->name.  */
	UT_hash_handle hh;

	/* A node is part of a tree.  */
	struct node *children;
} Node;

extern Node *new_node(TALLOC_CTX *context, const char *name, int type);
extern Node *add_new_child(Node *node, const char *name, int type);
extern size_t flush_subtree_cache(Node *node, bool show_size);
extern void print_tree(const Node *node, FILE *file);

/**
 * Add @child to @node's set of children.
 */
static inline void add_child(Node *node, Node *child)
{
	HASH_ADD_KEYPTR(hh, node->children, child->name, talloc_get_size(child->name), child);
}

#endif /* PROOT_VFS_NODE */
