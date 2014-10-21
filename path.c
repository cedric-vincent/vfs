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

#include <string.h>	/* str*(3), */
#include <assert.h>	/* assert(3), */
#include <talloc.h>
#include "path.h"

/**
 * Allocate a new component with given @name, with specified @length,
 * then push it into @path.  This function returns NULL if there's not
 * enough memory.
 */
Component *push_new_component(Path *path, const char *name, size_t length)
{
	Component *component;

	component = talloc_zero(path, Component);
	if (component == NULL)
		return NULL;

	component->name = talloc_strndup(component, name, length);
	if (component->name == NULL)
		return NULL;

	push_component(path, component);

	return component;
}

/**
 * Convert @path_ into a tailq structure.  This function returns NULL
 * if there's not enough memory.
 */
Path *new_path(TALLOC_CTX *context, const char *path_)
{
	Component *component;
	Path *path;

	const char *cursor;
	size_t length;

	path = talloc_zero(context, Path);
	if (path == NULL)
		return NULL;

	TAILQ_INIT(path);

	if (path_[0] == '/') {
		component = push_new_component(path, "/", 2);
		if (component == NULL)
			return NULL;
	}

	for (cursor = path_; cursor[0] != '\0'; cursor += length) {
		length = strcspn(cursor, "/");
		if (length == 0)
			goto skip_slash;

		component = push_new_component(path, cursor, length);
		if (component == NULL)
			return NULL;

	skip_slash:
		length += strspn(cursor, "/");
	}

	return path;
}

/**
 * Convert @path -- upto @last component -- into a C string.  This
 * function returns NULL if there's not enough memory.
 */
char *stringify_path(TALLOC_CTX *context, const Path *path, const Component *last)
{
	const Component *component;
	size_t offset;
	size_t size;
	char *path_;

	/* Pre-compute size of the path.  */
	size = 0;
	TAILQ_FOREACH(component, path, link) {
		size += talloc_get_size(component->name);

		if (component == last)
			break;
	}

	path_ = talloc_size(context, size);
	if (path_ == NULL)
		return NULL;

	/* Concatene names of components, separated by '/'.  */
	offset = 0;
	TAILQ_FOREACH(component, path, link) {
		size_t length;

		if (strcmp(component->name, "/") != 0) {
			length = talloc_get_size(component->name) - 1;
			memcpy(path_ + offset, component->name, length);
		}
		else {
			assert(offset == 0);
			length = 0;
		}

		path_[offset + length] = '/';

		offset += length + 1;

		if (component == last)
			break;
	}

	/* Replace trailing '/' with a terminating null byte.  */
	if (offset > 1)
		path_[offset - 1] = '\0';

	return path_;
}
