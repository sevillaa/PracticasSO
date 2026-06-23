#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
#include <err.h>

/* Forward declaration */
int get_size_dir(char *fname, size_t *blocks);

/* Gets in the blocks buffer the size of file fname using lstat. If fname is a
 * directory get_size_dir is called to add the size of its contents.
 */
int get_size(char *fname, size_t *blocks)
{
	struct stat sb;

	if (lstat(fname, &sb) == -1)
		return -1;

	*blocks += sb.st_blocks;

	if (S_ISDIR(sb.st_mode))
		return get_size_dir(fname, blocks);

	return 0;
}


/* Gets the total number of blocks occupied by all the files in a directory. If
 * a contained file is a directory a recursive call to get_size_dir is
 * performed. Entries . and .. are conveniently ignored.
 */
int get_size_dir(char *dname, size_t *blocks)
{
	DIR *dir;
	struct dirent *entry;

	dir = opendir(dname);
	if (dir == NULL)
		return -1;

	while ((entry = readdir(dir)) != NULL) {
		char *path;

		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0)
			continue;

		path = malloc(strlen(dname) + strlen(entry->d_name) + 2);
		if (path == NULL) {
			closedir(dir);
			return -1;
		}

		sprintf(path, "%s/%s", dname, entry->d_name);

		if (get_size(path, blocks) == -1) {
			free(path);
			closedir(dir);
			return -1;
		}

		free(path);
	}

	if (closedir(dir) == -1)
		return -1;

	return 0;
}

/* Processes all the files in the command line calling get_size on them to
 * obtain the number of 512 B blocks they occupy and prints the total size in
 * kilobytes on the standard output
 */
int main(int argc, char *argv[])
{
	int i;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <file> [file ...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	for (i = 1; i < argc; i++) {
		size_t blocks = 0;

		if (get_size(argv[i], &blocks) == -1)
			err(EXIT_FAILURE, "%s", argv[i]);

		printf("%zuK %s\n", (blocks + 1) / 2, argv[i]);
	}

	return 0;
}
