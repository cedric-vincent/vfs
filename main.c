#include <dirent.h>	/* DT_*, */
#include <stdio.h>	/* *printf(3), stdout, */
#include <errno.h>	/* ENOMEM, */
#include <fcntl.h>	/* O_NOFOLLOW, */
#include <talloc.h>
#include <uthash.h>
#include "vfs/node.h"
#include "vfs/path.h"
#include "vfs/symlink.h"
#include "vfs/children.h"
#include "vfs/find.h"
#include "vfs/tree.h"

static int dive_into_tree(Node *node)
{
	Node *child;
	int status;

	if (get_path(node, ACTUAL_PATH) == NULL)
		return -ENOMEM;

	if (get_path(node, VIRTUAL_PATH) == NULL)
		return -ENOMEM;

	if (node->type == DT_LNK) {
		if (get_symlink(node, &status) == NULL)
			return status;
	}

	if (node->type != DT_DIR)
		return 0;

	if (!node->children_filled) {
		status = fill_children(node);
		if (status < 0)
			return status;
	}

	for (child = node->children; child != NULL; child = child->hh.next) {
		status = dive_into_tree(child);
		if (status < 0) {
			fprintf(stderr, "can't dive into: %s (actually %s)\n",
				get_path(child, VIRTUAL_PATH),
				get_path(child, ACTUAL_PATH));
			return status;
		}
	}

	return 0;
}

int main(void)
{
	const char *new_root = "/usr/local/cedric/rootfs/slackware-8.1";
	Node *root;
	Node *node;
	Node *node2;
	int error;

	talloc_enable_leak_report_full();

	root = new_node(NULL, "/", -1, DT_DIR);
	if (root == NULL)
		exit(EXIT_FAILURE);

	node  = find_node(root, root, "/usr/tmp", O_NOFOLLOW, &error);
	node2 = find_node(root, root, "/usr/tmp", 0, &error);

	printf("actual  /usr/tmp: %s -> %s\n",
		get_path(node, ACTUAL_PATH), get_path(node2, ACTUAL_PATH));
	printf("virtual /usr/tmp: %s -> %s\n\n",
		get_path(node, VIRTUAL_PATH), get_path(node2, VIRTUAL_PATH));

	printf("*** changing rootfs to %s\n\n", new_root);

	set_actual_path(root, new_root);

	node  = find_node(root, root, "/usr/tmp", O_NOFOLLOW, &error);
	node2 = find_node(root, root, "/usr/tmp", 0, &error);

	printf("actual  /usr/tmp: %s -> %s\n",
		get_path(node, ACTUAL_PATH), get_path(node2, ACTUAL_PATH));
	printf("virtual /usr/tmp: %s -> %s\n\n",
		get_path(node, VIRTUAL_PATH), get_path(node2, VIRTUAL_PATH));

	node = find_node(root, root, "/usr", 0, &error);

	(void) dive_into_tree(root);

	flush_children(root, true);

	print_tree(root, stdout);

	printf("\n*** binding actual /bin over virtual /usr\n\n");

	node = find_node(root, root, "/usr", 0, &error);
	set_actual_path(node, "/bin");

	node = find_node(root, root, "/usr/tmp", O_NOFOLLOW, &error);

	printf("actual  /usr/tmp: %s\n", node != NULL
		? get_path(node, ACTUAL_PATH)
		: strerror(-error));
	printf("virtual /usr/tmp: %s\n\n", node != NULL
		? get_path(node, VIRTUAL_PATH)
		: strerror(-error));

	node = find_node(root, root, "/var/tmp", O_NOFOLLOW, &error);

	printf("actual  /var/tmp: %s\n", get_path(node, ACTUAL_PATH));
	printf("virtual /var/tmp: %s\n\n", get_path(node, VIRTUAL_PATH));

	node = find_node(root, root, "/usr/true", O_NOFOLLOW, &error);

	printf("actual  /usr/true: %s\n", get_path(node, ACTUAL_PATH));
	printf("virtual /usr/true: %s\n\n", get_path(node, VIRTUAL_PATH));

	delete_tree(root);

	exit(EXIT_SUCCESS);
}
