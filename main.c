#include <dirent.h>	/* DT_*, */
#include <stdio.h>	/* stdout, */
#include "node.h"
#include "path.h"

int main(void)
{
	Node *root;
	Node *node;
	Node *node2;

	const char *path_ = "///etc///..///fstab/////.";
	const char *path2_ = "etc///..///fstab/////.";
	Path *path;

	root = new_node(NULL, "/", DT_DIR);
	if (root == NULL)
		exit(EXIT_FAILURE);

	node = add_new_child(root, "etc", DT_DIR);
	if (node == NULL)
		exit(EXIT_FAILURE);

	node2 = add_new_child(node, "fstab", DT_REG);
	if (node2 == NULL)
		exit(EXIT_FAILURE);

	node2->purpose = CACHE;

	node2 = add_new_child(node, "inittab", DT_REG);
	if (node2 == NULL)
		exit(EXIT_FAILURE);

	print_tree(root, stdout);

	flush_subtree_cache(root, true);

	print_tree(root, stdout);

	path = new_path(NULL, path_);
	if (path == NULL)
		exit(EXIT_FAILURE);

	printf(">>> '%s' -> '%s'\n", path_, stringify_path(NULL, path, NULL));

	path = new_path(NULL, path2_);
	if (path == NULL)
		exit(EXIT_FAILURE);

	printf(">>> '%s' -> '%s'\n", path2_, stringify_path(NULL, path, NULL));

	exit(EXIT_SUCCESS);
}
