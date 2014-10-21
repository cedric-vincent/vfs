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

#include <dirent.h>	/* DT_, */
#include <stdio.h>	/* *printf(3), */
#include <stdbool.h>	/* bool, */
#include <talloc.h>
#include <uthash.h>
#include "node.h"

/**
 * See print_tree().
 */
static void print_tree2(const Node *node, FILE *file, size_t depth)
{
	Node *child;
	size_t i;

	for (i = 0; i < depth; i++)
		fprintf(file, " ");

	fprintf(file, "%s [purpose: ", node->name);

	switch(node->purpose) {
	case BINDING:
		fprintf(file, "binding (%s)", node->value);
		break;
	case FILLER:
		fprintf(file, "filler");
		break;
	case CACHE:
		fprintf(file, "cache");
		break;
	default:
		fprintf(file, "unknown (%x)", node->purpose);
		break;
	}

	fprintf(file, "; visibility: ");

	switch(node->purpose) {
	case DEFAULT:
		fprintf(file, "default");
		break;
	case ADDED:
		fprintf(file, "added");
		break;
	case REMOVED:
		fprintf(file, "removed");
		break;
	default:
		fprintf(file, "unknown (%x)", node->purpose);
		break;
	}

	fprintf(file, "; type: ");

	switch (node->type) {
	case DT_REG:
		fprintf(file, "regular");
		break;
	case DT_DIR:
		fprintf(file, "directory");
		break;
	case DT_LNK:
		fprintf(file, "symlink");
		break;
	case DT_BLK:
		fprintf(file, "block dev.");
		break;
	case DT_CHR:
		fprintf(file, "char. dev.");
		break;
	case DT_FIFO:
		fprintf(file, "fifo");
		break;
	case DT_SOCK:
		fprintf(file, "socket");
		break;
	default:
		fprintf(file, "unknown (%x)", node->type);
		break;
	}

	if (node->evaluator != NULL)
		fprintf(file, "; lazy evaluated");

	fprintf(file, "]\n");

	for (child = node->children; child != NULL; child = child->hh.next)
		print_tree2(child, file, depth + 1);
}

/**
 * Print @node's tree in @file.
 */
void print_tree(const Node *node, FILE *file)
{
	print_tree2(node, file, 0);
}

/**
 * Remove and free cache nodes from @node's subtree.  Note that @node
 * is *not* flushed.  If @show_size is true, @node's subtree size is
 * printed on stderr.  This function returns number of detected
 * anomalies in @node's subtree.
 */
size_t flush_subtree_cache(Node *node, bool show_size)
{
	size_t anomalies = 0;
	size_t total_size;
 	Node *child;
	Node *tmp;

	if (show_size)
		total_size = talloc_total_size(node);

	HASH_ITER(hh, node->children, child, tmp) {
		flush_subtree_cache(child, false);

		if (child->purpose == CACHE) {
			/* Cache nodes' children should be cache nodes
			 * too, thus they should all be flushed.  */
			if (child->children != NULL)
				anomalies++;

			HASH_DELETE(hh, node->children, child);
			TALLOC_FREE(child);
		}
	}

	if (show_size) {
		fprintf(stderr, "size before cache flush: %zd\n", total_size);
		fprintf(stderr, "size after cache flush:  %zd\n", node != NULL
			? talloc_total_size(node) : 0);
	}

	return anomalies;
}

/**
 * Allocate a new node with given @name and @type.  This function
 * returns NULL if there's not enough memory.
 */
Node *new_node(TALLOC_CTX *context, const char *name, int type)
{
	Node *node;

	node = talloc_zero(context, Node);
	if (node == NULL)
		return NULL;

	node->name = talloc_strdup(node, name);
	if (node->name == NULL)
		return NULL;

	node->type = type;

	return node;
}

/**
 * Allocate a new node with given @name and @type, then add it to
 * @node's set of children.  This function returns NULL if there's not
 * enough memory.
 */
Node *add_new_child(Node *node, const char *name, int type)
{
	Node *child;

	child = new_node(node, name, type);
	if (child == NULL)
		return NULL;

	add_child(node, child);

	return child;
}
