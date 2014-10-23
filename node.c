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

#include <string.h>	/* strlen(3), */
#include <talloc.h>
#include <uthash.h>
#include "vfs/node.h"

/**
 * Add @child to @node's children list, and set @child's parent to
 * @node.
 */
static void add_child(Node *node, Node *child)
{
	HASH_ADD_KEYPTR(hh, node->children, child->name, talloc_get_size(child->name) - 1, child);
	child->parent = node;
}

/**
 * Allocate for @context a new node with given @name and @type.  This
 * function returns NULL if there's not enough memory.
 */
Node *new_node(TALLOC_CTX *context, const char *name, ssize_t length, int type)
{
	Node *node;

	node = talloc_zero(context, Node);
	if (node == NULL)
		return NULL;

	if (length < 0)
		length = strlen(name);

	node->name = talloc_strndup(node, name, length);
	if (node->name == NULL)
		return NULL;

	talloc_set_name_const(node->name, "$name");

	node->type   = type;
	node->parent = node;

	return node;
}

/**
 * Allocate a new node with given @name and @type, then add it to
 * @node's children list, and set its parent to @node.  This function
 * returns NULL if there's not enough memory.
 */
Node *add_new_child(Node *node, const char *name, ssize_t length, int type)
{
	Node *child;

	child = new_node(node, name, length, type);
	if (child == NULL)
		return NULL;

	add_child(node, child);

	return child;
}
