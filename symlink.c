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

#include <dirent.h>	/* DT_LNK, */
#include <unistd.h>	/* readlink(2), */
#include <errno.h>	/* E*, errno(3), */
#include <talloc.h>
#include "vfs/symlink.h"
#include "vfs/node.h"
#include "vfs/path.h"

/**
 * Allocate for @context a new symlink built from @node.  This
 * function returns NULL if there's not enough memory, and *@error is
 * set to -errno.
 */
static char *new_symlink_from_node(TALLOC_CTX *context, Node *node, int *error)
{
	char *symlink = NULL;
	const char *path;
	ssize_t result;
	ssize_t size;
	char *tmp;

	if (node->type != DT_LNK) {
		*error = -EINVAL;
		return NULL;
	}

	path = get_path(node, ACTUAL_PATH);
	if (path == NULL) {
		*error = -ENOMEM;
		return NULL;
	}

	/* Start from a reasonable size.  */
	size = 128;
	do {
		size *= 2;

		symlink = talloc_size(context, size);
		if (symlink == NULL) {
			*error = -ENOMEM;
			goto free_symlink;
		}

		result = readlink(path, symlink, size);
		if (result < 0) {
			*error = -errno;
			goto free_symlink;
		}
	} while (result == size && size > 0);

	if (size < 0) {
		*error = -ENOMEM;
		goto free_symlink;
	}

	/* Adjust to the right size.  */
	tmp = talloc_realloc_size(context, symlink, result + 1);
	if (tmp == NULL) {
		*error = -ENOMEM;
		goto free_symlink;
	}

	symlink = tmp;
	symlink[result] = '\0';

	talloc_set_name_const(symlink, symlink);

	return symlink;

free_symlink:
	TALLOC_FREE(symlink);
	return NULL;
}

/**
 * Get @node->symlink, however this function is similar to
 * new_symlink_from_node(@node, @node) if it was not computed yet.
 */
const char *get_symlink(Node *node, int *error)
{
	if (node->symlink_ != NULL)
		return node->symlink_;

	node->symlink_ = new_symlink_from_node(node, node, error);
	if (node->symlink_ == NULL)
		return NULL;

	talloc_set_name_const(node->symlink_, "$symlink");

	return node->symlink_;
}
