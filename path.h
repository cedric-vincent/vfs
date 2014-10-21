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

#ifndef PROOT_VFS_PATH
#define PROOT_VFS_PATH

#include <sys/queue.h>

typedef struct component
{
	char *name;
	TAILQ_ENTRY(component) link;
} Component;

typedef TAILQ_HEAD(path, component) Path;

extern Path *new_path(TALLOC_CTX *context, const char *path);
extern char *stringify_path(TALLOC_CTX *context, const Path *path, const Component *last);

/**
 * Push @component into @path.
 */
static inline void push_component(Path *path, Component *component)
{
	TAILQ_INSERT_TAIL(path, component, link);
}

#endif /* PROOT_VFS_PATH */
