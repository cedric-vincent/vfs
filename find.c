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

#include <sys/param.h>	/* MAXSYMLINKS, */
#include <errno.h>	/* E*, */
#include <dirent.h>	/* DT_*, */
#include <assert.h>	/* assert(3), */
#include <string.h>	/* str*spn(3), */
#include <stdbool.h>	/* bool, */
#include <fcntl.h>	/* O_NOFOLLOW, O_CREATE, */
#include "vfs/find.h"
#include "vfs/node.h"
#include "vfs/symlink.h"
#include "vfs/children.h"

/**
 * Find in @root file-system the node pointed to by @node.  This
 * function returns NULL if an error occurred, and *@error is set to
 * -errno.
 */
static Node *follow_symlink_node(Node *root, Node *node, int *error, size_t symlink_count)
{
	const char *symlink;

	if (symlink_count++ > MAXSYMLINKS) {
		*error = -ELOOP;
		return NULL;
	}

	symlink = get_symlink(node, error);
	if (symlink == NULL)
		return NULL;

	return find_node_(root, node->parent, symlink, 0, error, symlink_count);
}

/**
 * Get @node's child with given @name.  This function handles special
 * names "." and "..", respectively @node and @node->parent.  Also, it
 * fills @node's children list if needed.
 */
static Node *get_child(Node *node, const char *name, ssize_t length)
{
	Node *child = NULL;

	assert(node->type == DT_DIR);

	if (length < 0)
		length = strlen(name);

	if (length == 1 && strncmp(name, ".", length) == 0)
		return node;

	if (length == 2 && strncmp(name, "..", length) == 0)
		return node->parent;

	if (!node->children_filled)
		(void) fill_children(node);

	HASH_FIND(hh, node->children, name, length, child);
	return child;
}

/**
 * Find in @root file-system the node for @path, relatively to @from
 * if not absolute.  @flags is a bit mask that can contain O_NOFOLLOW
 * and/or O_CREATE.  This function returns NULL if an error occurred,
 * and *@error is set to -errno.
 */
Node *find_node_(Node *root, Node *from, const char *path, int flags,
		int *error, size_t symlink_count)
{
	bool follow_symlink = ((flags & O_NOFOLLOW) == 0);
	bool create = ((flags & O_CREAT) != 0);
	Node *node;

	if (path[0] == '/')
		node = root;
	else
		node = from;

	while (path[0] != '\0') {
		Node *parent_node;
		size_t length;
		bool is_final;

		if (node->type != DT_DIR) {
			*error = -ENOTDIR;
			return NULL;
		}

		/* Find component boundaries.  */
		path += strspn(path, "/");
		length = strcspn(path, "/");
		is_final = (path[length] == '\0');

		parent_node = node;
		node = get_child(node, path, length);
		if (node == NULL) {
			if (!is_final) {
				*error = -ENOTDIR;
				return NULL;
			}

			if (!create) {
				*error = -ENOENT;
				return NULL;
			}

			node = add_new_child(parent_node, path, length, 0 /* DT_UNKNOWN */);
			if (node == NULL) {
				*error = -ENOMEM;
				return NULL;
			}

			/* Early exit; it's the final component
			 * anyway.  */
			break;
		}

		/* Move to the next component.  */
		path += length;

		if (node->type == DT_LNK && (!is_final || follow_symlink)) {
			node = follow_symlink_node(root, node, error, symlink_count);
			if (node == NULL)
				return NULL;
		}
	}

	return node;
}
