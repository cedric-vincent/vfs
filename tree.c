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

#include <stdio.h>	/* fprintf(3), */
#include <errno.h>	/* EBUSY, */
#include <dirent.h>	/* DT_*, */
#include <talloc.h>
#include <uthash.h>
#include "vfs/tree.h"
#include "vfs/node.h"

/**
 * Print in @file a human readable format of @root, then perform
 * recursively the same for @root node's children.
 */
void print_tree_(const Node *root, FILE *file, size_t indentation)
{
	Node *child;
	size_t i;

	for (i = 0; i < indentation; i++)
		fprintf(file, " ");

	fprintf(file, "%s [type: ", root->name);

	switch (root->type) {
	case DT_REG:	fprintf(file, "regular");	break;
	case DT_DIR:	fprintf(file, "directory");	break;
	case DT_LNK:	fprintf(file, "symlink");	break;
	case DT_BLK:	fprintf(file, "block dev.");	break;
	case DT_CHR:	fprintf(file, "char. dev.");	break;
	case DT_FIFO:	fprintf(file, "fifo");		break;
	case DT_SOCK:	fprintf(file, "socket");	break;
	default: fprintf(file, "unknown (%x)", root->type); break;
	}

	fprintf(file, "; actual path: %s",	root->path_.actual);
	fprintf(file, "; virtual path: %s",	root->path_.virtual);
	fprintf(file, "; evaluator: %p",	root->evaluator);
	fprintf(file, "; special: %d",		root->special);

	fprintf(file, "]\n");

	/* No child deletion, so no need for HASH_ITER.  */
	for (child = root->children; child != NULL; child = child->hh.next)
		print_tree_(child, file, indentation + 2);
}

/**
 * Delete recursively @parent's children.  This function returns
 * -EBUSY if a children is referenced elsewhere, otherwise the number
 * of deleted nodes.
 */
static ssize_t delete_children(Node *parent)
{
	size_t nb_deleted_nodes = 0;
	ssize_t status;
	Node *child;
	Node *tmp;

	HASH_ITER(hh, parent->children, child, tmp) {
		size_t reference_count;

		status = delete_children(child);
		if (status < 0)
			return status;
		nb_deleted_nodes += status;

		reference_count = talloc_reference_count(child);
		if (reference_count > 1)
			return -EBUSY;

		HASH_DEL(parent->children, child);
		TALLOC_FREE(child);

		nb_deleted_nodes++;
	}

	return nb_deleted_nodes;
}

/**
 * Delete recursively @root.  This function returns -EBUSY if @root
 * node or one of its children is referenced elsewhere, otherwise the
 * number of deleted nodes.  */
ssize_t delete_tree(Node *root)
{
	size_t nb_deleted_nodes = 0;
	size_t reference_count;
	ssize_t status;

	status = delete_children(root);
	if (status < 0)
		return status;
	nb_deleted_nodes += status;

	reference_count = talloc_reference_count(root);
	if (reference_count > 1)
		return -EBUSY;

	TALLOC_FREE(root);
	nb_deleted_nodes++;

	return nb_deleted_nodes;
}
