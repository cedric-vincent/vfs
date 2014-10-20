#include <talloc.h>	/* talloc*, */
#include <sys/queue.h>	/* TAILQ*, */
#include <string.h>	/* str*(3), mem*(3), */
#include <sys/types.h>	/* lstat(2), */
#include <sys/stat.h>	/* lstat(2), */
#include <unistd.h>	/* lstat(2), */
#include <stdio.h>	/* *printf(3), */
#include <stdbool.h>	/* bool, true,  */
#include <assert.h>	/* assert(3),  */
#include <errno.h>	/* E*,  */

typedef enum {
	ERROR = 0,	/* missing initialization.  */
	BINDING,	/* remap a host path over a guest path.  */
	FILLER,		/* fill gap between a guest leaf and a binding leaf.  */
	CACHE,		/* remember translation result.  */
} VFSNodePurpose;

typedef enum {
	DEFAULT = 0,	/* Trust the actual file-system.  */
	ADDED,		/* Add to parent's dentries.  */
	REMOVED,	/* Hide from parent's dentries.  */
} VFSNodeVisibility;

struct vfs_node;
typedef int (* VFSNodeEvaluator)(struct vfs_node *node);

typedef struct vfs_node
{
	const char *name;
	const char *value;
	mode_t type;

	VFSNodePurpose purpose;
	VFSNodeVisibility visibility;
	VFSNodeEvaluator evaluator; /* Lazy evaluation, à la /proc.  */

	/* Internally used to maintain the VFS structure.  */
	TAILQ_HEAD(, vfs_node) _children;
	TAILQ_ENTRY(vfs_node) _siblings;
} VFSNode;

/* A path is just a tree where all nodes have either one or zero
 * child.  */
#define TAILQ_NEXT_NODE(node) \
	TAILQ_FIRST(&((node)->_children))

#define TAILQ_FOREACH_CHILD(iterator, parent) \
	TAILQ_FOREACH((iterator), &((parent)->_children), _siblings)

#define TAILQ_INSERT_CHILD(parent, child) \
	TAILQ_INSERT_TAIL(&((parent)->_children), (child), _siblings)

#define TAILQ_REMOVE_CHILD(parent, child) \
	TAILQ_REMOVE(&((parent)->_children), (child), _siblings)

/**
 * See vfs_print().
 */
static void vfs_print2(FILE *file, const VFSNode *parent, size_t depth)
{
	VFSNode *child;
	size_t i;

	for (i = 0; i < depth; i++)
		fprintf(file, " ");

	fprintf(file, "%s [purpose: ", parent->name);

	switch(parent->purpose) {
	case BINDING:
		fprintf(file, "binding (%s)", parent->value);
		break;
	case FILLER:
		fprintf(file, "filler");
		break;
	case CACHE:
		fprintf(file, "cache");
		break;
	default:
		fprintf(file, "unknown (%x)", parent->purpose);
		break;
	}

	fprintf(file, "; visibility: ");

	switch(parent->purpose) {
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
		fprintf(file, "unknown (%x)", parent->purpose);
		break;
	}

	fprintf(file, "; type: ");

	if (S_ISREG(parent->type))
		fprintf(file, "regular");
	else if (S_ISDIR(parent->type))
		fprintf(file, "directory");
	else if (S_ISLNK(parent->type))
		fprintf(file, "symlink");
	else if (S_ISBLK(parent->type))
		fprintf(file, "block dev.");
	else if (S_ISCHR(parent->type))
		fprintf(file, "char. dev.");
	else if (S_ISFIFO(parent->type))
		fprintf(file, "fifo");
	else if (S_ISSOCK(parent->type))
		fprintf(file, "socket");
	else
		fprintf(file, "unknown (%x)", parent->type);

	if (parent->evaluator != NULL)
		fprintf(file, "; lazy evaluated");

	fprintf(file, "]\n");

	TAILQ_FOREACH_CHILD(child, parent)
		vfs_print2(file, child, depth + 1);
}

/**
 * Print content of @parent in @file.
 */
void vfs_print(FILE *file, const VFSNode *parent)
{
	vfs_print2(file, parent, 0);
}

/**
 * Find the first child of @parent with the given @name.
 */
VFSNode *vfs_find_child(const VFSNode *parent, const char *name)
{
	VFSNode *child;

	TAILQ_FOREACH_CHILD(child, parent) {
		if (strcmp(child->name, name) == 0)
			return child;
	}

	return NULL;
}

/**
 * Free and remove all children of @parent that were used for caching
 * purpose.  Note @parent is not freed even if it is used for caching
 * purpose.  The size of the VFS pointed to by @parent is printed to
 * stderr when @show_size is true.  This function returns the number
 * of detected anomalies in this VFS.
 */
size_t vfs_flush_cache(VFSNode *parent, bool show_size)
{
	size_t anomalies = 0;
	VFSNode *child;
	size_t total_size;

	if (show_size)
		total_size = talloc_total_size(parent);

	child = TAILQ_FIRST(&parent->_children);
	while (child != NULL) {
		VFSNode *next = TAILQ_NEXT(child, _siblings);

		vfs_flush_cache(child, false);

		if (child->purpose == CACHE) {
			/* A CACHE node can only have CACHE children,
			 * thus its list of children is expected to be
			 * empty now.  */
			if (TAILQ_EMPTY(&child->_children)) {
				TAILQ_REMOVE_CHILD(parent, child);
				TALLOC_FREE(child);
			}
			else
				anomalies++;
		}

		child = next;
	}

	if (show_size) {
		fprintf(stderr, "VFS size before cache flush: %zd\n", total_size);
		fprintf(stderr, "VFS size after cache flush:  %zd\n", parent != NULL
			? talloc_total_size(parent) : 0);
	}

	return anomalies;
}

/**
 * Allocate a new VFS node.  The Talloc parent of this new node is
 * @context.  This function returns NULL if an error occured.
 */
VFSNode *vfs_new_node(TALLOC_CTX *context)
{
	VFSNode *node;

	node = talloc_zero(context, VFSNode);
	if (node == NULL)
		return NULL;

	TAILQ_INIT(&node->_children);

	return node;
}

/**
 * Allocate a new VFS node with given @name and @type.  See
 * vfs_new_node() for the purpose of @context and for error handling.
 */
VFSNode *vfs_new_node2(TALLOC_CTX *context, const char *name, mode_t type)
{
	VFSNode *node;

	node = vfs_new_node(context);
	if (node == NULL)
		return NULL;

	node->name = talloc_strdup(node, name);
	if (node->name == NULL)
		return NULL;

	node->type = type;

	return node;
}

/**
 * Move @node into @new_parent, that is, @node becomes a child of
 * @new_parent.  This function returns -errno if an error occured,
 * otherwise 0.
 */
int vfs_move(VFSNode *node, VFSNode *new_parent)
{
	VFSNode *iterator;

	if (!S_ISDIR(new_parent->type))
		return -ENOTDIR;

	/* Ensure @new_parent is not moved into itself.  */
	for (iterator = new_parent; iterator != NULL; iterator = talloc_parent(iterator)) {
		if (iterator == node)
			return -EINVAL;
	}

	talloc_steal(new_parent, node);
	TAILQ_INSERT_CHILD(new_parent, node);

	return 0;
}

/**
 * Remove @node from its parent children list, then free its memory.
 */
void vfs_remove(VFSNode *node)
{
	VFSNode *parent;

	parent = talloc_parent(node);
	if (parent != NULL)
		TAILQ_REMOVE_CHILD(parent, node);

	TALLOC_FREE(node);
}

/**
 * Split @path into a list of VFS nodes.  The Talloc memory of the
 * first node is attached to @context.  This function returns NULL if
 * an error occured.
 */
VFSNode *vfs_split_path(TALLOC_CTX *context, const char *path)
{
	VFSNode *previous = NULL;
	VFSNode *head = NULL;

	VFSNode *current;
	const char *cursor;
	size_t length;

	if (path[0] == '/') {
		head = vfs_new_node2(context, "/", S_IFDIR);
		if (head == NULL)
			return NULL;

		previous = head;
	}

	for (cursor = path, length = 0; cursor[0] != '\0'; cursor += length) {
		length = strcspn(cursor, "/");
		if (length == 0)
			goto skip_slash;

		current = vfs_new_node(previous ?: context);
		if (current == NULL)
			return NULL;

		current->name = talloc_strndup(current, cursor, length);
		if (current->name == NULL)
			return NULL;

		if (previous != NULL)
			TAILQ_INSERT_CHILD(previous, current);

		previous = current;

		if (head == NULL)
			head = current;

	skip_slash:
		length += strspn(cursor, "/");
	}

	return head;
}

/**
 * Combine the given list of VFS nodes.  The Talloc memory of the
 * reconstructed path is attached to @context.  This function returns
 * NULL if an error occured.
 */
char *vfs_combine_path(TALLOC_CTX *context, const VFSNode *nodes)
{
	const VFSNode *node;
	size_t offset;
	size_t size;
	char *path;

	/* Pre-compute size of the path.  */
	size = 0;
	for (node = nodes; node != NULL; node = TAILQ_NEXT_NODE(node))
		size += talloc_get_size(node->name);

	path = talloc_size(context, size);
	if (path == NULL)
		return NULL;

	/* Concatene names of all nodes, separated by '/'.  */
	offset = 0;
	for (node = nodes; node != NULL; node = TAILQ_NEXT_NODE(node)) {
		size_t length;

		if (strcmp(node->name, "/") != 0) {
			length = talloc_get_size(node->name) - 1;
			memcpy(path + offset, node->name, length);
		}
		else {
			assert(offset == 0);
			length = 0;
		}

		path[offset + length] = '/';

		offset += length + 1;
	}

	/* Replace trailing '/' with a terminating null byte.  */
	if (offset > 1)
		path[offset - 1] = '\0';

	return path;
}

int main(void)
{
	VFSNode *nodes1;
	VFSNode *nodes2;

	nodes1 = vfs_split_path(NULL, "/this/is/an/absolute/path");
	if (nodes1 == NULL)
		exit(EXIT_FAILURE);

	nodes2 = vfs_split_path(NULL, "not/an/absolute/path");
	if (nodes2 == NULL)
		exit(EXIT_FAILURE);

	printf(">>> %d\n", vfs_move(nodes2, nodes1));
	printf(">>> %d\n", vfs_move(nodes1, nodes2));

	vfs_remove(TAILQ_NEXT_NODE(TAILQ_NEXT_NODE(TAILQ_NEXT_NODE(nodes1))));

	printf("!!! %zd\n", vfs_flush_cache(nodes1, true));

	TAILQ_NEXT_NODE(TAILQ_NEXT_NODE(nodes1))->purpose = CACHE;
	//TAILQ_NEXT_NODE(nodes1)->purpose = CACHE;

	printf("!!! %zd\n", vfs_flush_cache(nodes1, true));

	vfs_print(stdout, nodes1);

	vfs_remove(TAILQ_NEXT_NODE(nodes1));
	vfs_remove(TAILQ_NEXT_NODE(nodes1));

	vfs_print(stdout, nodes1);

	printf("!!! %zd %zd\n", vfs_flush_cache(nodes1, true), sizeof(VFSNode));

	talloc_report_full(nodes1, stdout);

	//vfs_print(stdout, nodes1);
	//vfs_print(stdout, nodes1);

	exit(EXIT_SUCCESS);
}
