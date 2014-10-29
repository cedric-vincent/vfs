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

#include <dirent.h>	/* DIR, DT_DIR, struct dirent, opendir(3), closedir(3), */
#include <assert.h>	/* assert(3), */
#include <errno.h>	/* errno(3), ENOMEM, */
#include <string.h>	/* str*(), */
#include <stdio.h>	/* fprintf(3), */
#include <talloc.h>
#include <uthash.h>
#include "vfs/children.h"
#include "vfs/path.h"
#include "vfs/node.h"

/**
 * Fill @parent->children with the directory entries of @parent actual
 * path.  This function return -errno if an error occurred, otherwise
 * 0.
 */
int fill_children(Node *parent)
{
	DIR *dir = NULL;

	struct dirent *entry;
	const char *path;
	int status;

	assert(!parent->children_filled);
	assert(parent->type == DT_DIR);

	path = get_path(parent, ACTUAL_PATH);
	if (path == NULL)
		return -ENOMEM;

	dir = opendir(path);
	if (dir == NULL)
		return -errno;

	while (1) {
		Node *child;

		errno = 0;
		entry = readdir(dir);
		if (entry == NULL) {
			status = -errno;
			goto end;
		}

		if (   strcmp(entry->d_name, ".") == 0
		    || strcmp(entry->d_name, "..") == 0)
			continue;

		HASH_FIND_STR(parent->children, entry->d_name, child);
		if (child != NULL) {
			if (!child->special)
				fprintf(stderr, "Entry '%s' aldready filled in '%s'\n",
					entry->d_name, get_path(parent, ACTUAL_PATH));
			continue;
		}

		child = add_new_child(parent, entry->d_name, -1, entry->d_type);
		if (child == NULL) {
			status = -ENOMEM;
			goto end;
		}
	}

end:
	parent->children_filled = true;

	if (dir != NULL)
		(void) closedir(dir);

	return status;
}

/**
 * Delete recursively all @parent's children that are not "special"
 * and without external references.  If @show_size is true, @parent
 * tree size is printed on stderr.  This function returns number of
 * deleted nodes.
 */
size_t flush_children(Node *parent, bool show_size)
{
	size_t nb_flushed_nodes = 0;
	size_t total_size;
	Node *child;
	Node *tmp;

	if (show_size)
		total_size = talloc_total_size(parent);

	HASH_ITER(hh, parent->children, child, tmp) {
		size_t reference_count;

		nb_flushed_nodes += flush_children(child, false);

		reference_count = talloc_reference_count(child);
		if (reference_count > 1 || child->special)
			continue;

		HASH_DEL(parent->children, child);
		TALLOC_FREE(child);

		nb_flushed_nodes++;
	}

	if (show_size) {
		fprintf(stderr, "number of flushed nodes: %zd\n", nb_flushed_nodes);
		fprintf(stderr, "size before flush: %zd\n", total_size);
		fprintf(stderr, "size after flush:  %zd\n", talloc_total_size(parent));
	}

	parent->children_filled = false;

	return nb_flushed_nodes;
}
