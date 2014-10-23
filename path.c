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

#include <string.h>	/* strc*(), */
#include <assert.h>	/* assert(0), */
#include <errno.h>	/* ENOMEM, */
#include <talloc.h>
#include "vfs/path.h"
#include "vfs/node.h"
#include "vfs/children.h"

/**
 * Allocate for @context a new @class path built from @node.  This
 * function returns NULL if there's not enough memory.
 */
static char *new_path_from_node(TALLOC_CTX *context, const Node *node, PathClass class)
{
	char *path = NULL;
	size_t size;

	size = 0;
	while (1) {
		char *old_path = path;
		const char *prefix;

		switch (class) {
		case ACTUAL_PATH:
			prefix = node->path_.actual ?: node->name;
			break;

		case VIRTUAL_PATH:
			prefix = node->path_.virtual ?: node->name;
			break;

		default:
			assert(0);
		}

		size += talloc_get_size(prefix);

		path = talloc_size(context, size);
		if (path == NULL) {
			TALLOC_FREE(old_path);
			return NULL;
		}

		strcpy(path, prefix);

		if (old_path != NULL) {
			if (strcmp(prefix, "/") != 0)
				strcat(path, "/");
			strcat(path, old_path);
			TALLOC_FREE(old_path);
		}

		if (prefix != node->name || node->parent == node)
			break;

		node = node->parent;
	}

	talloc_set_name_const(path, path);

	return path;
}

/**
 * Get @node->path_.@class, however this function is similar to
 * new_path_from_node(@node, @node, @class) if it was not computed
 * yet.
 */
const char *get_path(Node *node, PathClass class)
{
	switch (class) {
	case ACTUAL_PATH:
		if (node->path_.actual != NULL)
			return node->path_.actual;

		node->path_.actual = new_path_from_node(node, node, class);
		if (node->path_.actual == NULL)
			return NULL;

		talloc_set_name_const(node->path_.actual, "$path.actual");

		return node->path_.actual;

	case VIRTUAL_PATH:
		if (node->path_.virtual != NULL)
			return node->path_.virtual;

		node->path_.virtual = new_path_from_node(node, node, class);
		if (node->path_.virtual == NULL)
			return NULL;

		talloc_set_name_const(node->path_.virtual, "$path.virtual");

		return node->path_.virtual;

	default:
		assert(0);
	}
}

/**
 * Delete @node->path_.@class if @node is not special, then perform
 * recursively the same for @node's children.
 */
void flush_path(Node *node, PathClass class)
{
	Node *child;

	switch (class) {
	case ACTUAL_PATH:
		if (!node->special)
			TALLOC_FREE(node->path_.actual);
		break;

	case VIRTUAL_PATH:
		TALLOC_FREE(node->path_.virtual);
		break;

	default:
		assert(0);
	}

	/* No child deletion, so no need for HASH_ITER.  */
	for (child = node->children; child != NULL; child = child->hh.next)
		flush_path(child, class);
}

/**
 * Flush everything that depends on @node->path_.actual, then mark
 * @node as special and set @node->path_.actual to a copy of @path.
 * There is no set_virtual_path() since it makes no sense to force the
 * value of @node->path_.virtual.  This function returns -errno if an
 * error occurred, otherwise 0.
 */
int set_actual_path(Node *node, const char *path)
{
	char *copy_path;

	copy_path = talloc_strdup(node, path);
	if (copy_path == NULL)
		return -ENOMEM;

	flush_children(node, false);

	flush_path(node, ACTUAL_PATH);

	node->path_.actual = copy_path;
	node->special = true;

	return 0;
}
