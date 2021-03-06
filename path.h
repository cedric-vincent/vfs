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

#include "vfs/node.h"

typedef enum {
	ACTUAL_PATH,
	VIRTUAL_PATH,
} PathClass;

extern const char *get_path(Node *node, PathClass class);
extern void flush_path(Node *node, PathClass class);
extern int set_actual_path(Node *node, const char *path);

#endif /* PROOT_VFS_PATH */
