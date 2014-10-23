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
#include <talloc.h>	/* TALLOC_CTX, */
#include <uthash.h>	/* UT_hash_handle, */

typedef struct node
{
	/**********************************************************************
	 * General info.: shouldn't be written outside vfs/                   *
	 **********************************************************************/

	/* Short name of this node.  */
	const char *name;

	/* Node type, as in linux_dirent->d_type.  */
	int type;

	/* A node is part of a tree.  */
	struct node *parent;
	struct node *children;


	/**********************************************************************
	 * General info.: can be written outside vfs/, with care.             *
	 **********************************************************************/

	/* Nodes explicitly modified by user need special care: for
	 * instance they should not be flushed, ... */
	bool special;

	/* Resolve a virtual node, as in /proc.  */
	int (* evaluator)(struct node *node);


	/**********************************************************************
	 * State info.: shouldn't be read or written outside vfs/             *
	 **********************************************************************/

	/* Whether "regular" children were created.  */
	bool children_filled;

	/* Make this structure hashable, key is self->name.  */
	UT_hash_handle hh;


	/**********************************************************************
	 * Lazily evaluated info., have to read or written through accessors. *
	 **********************************************************************/

	/* Canonicalized paths.  */
	struct {
		char *actual;
		char *virtual;
	} path_;

	/* Symbolic link content, when self->type == DT_LNK.  */
	char *symlink_;
} Node;

extern Node *new_node(TALLOC_CTX *context, const char *name, ssize_t length, int type);
extern Node *add_new_child(Node *node, const char *name, ssize_t length, int type);

#endif /* PROOT_VFS_NODE */
