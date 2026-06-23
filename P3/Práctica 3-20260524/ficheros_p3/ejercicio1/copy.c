#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFFER_SIZE 512

void copy(int fdo, int fdd)
{
	unsigned char buffer[BUFFER_SIZE];
	ssize_t leido, escrito;

	while ((leido = read(fdo, buffer, BUFFER_SIZE)) > 0) {
		escrito = write(fdd, buffer, leido);
		if (escrito != leido)
			err(3, "write() failed");
	}

	if (leido == -1)
		err(2, "read() failed");
}

void copy_regular(char *orig, char *dest)
{
	int fdo;
	int fdd;

	if ((fdo = open(orig, O_RDONLY)) == -1)
		err(2, "The input file %s could not be opened", orig);

	if ((fdd = open(dest, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
		close(fdo);
		err(2, "The output file %s could not be opened", dest);
	}

	copy(fdo, fdd);

	close(fdd);
	close(fdo);
}

void copy_link(char *orig, char *dest)
{
	/* To be completed */
	printf("symlink\n");
}

int main(int argc, char *argv[])
{
	int opt;
	struct stat sb;

	while ((opt = getopt(argc, argv, "hs:")) != -1) {
		switch (opt) {
		case 'h':
			fprintf(stderr, "Usage: %s [ -h | -s <input_file> <output_file> ]\n", argv[0]);
			exit(0);
		case 's':
			if (argc - optind != 1) {
				fprintf(stderr, "Numero incorrecto de archivos, debe ser uno de entrada y uno de salida\n");
				exit(1);
			}

			if (lstat(optarg, &sb) == -1) {
				perror("lstat");
				exit(EXIT_FAILURE);
			}

			switch (sb.st_mode & S_IFMT) {
			case S_IFLNK:
				copy_link(optarg, argv[optind]);
				break;
			case S_IFREG:
				printf("regular file\n");
				copy_regular(optarg, argv[optind]);
				break;
			default:
				printf("unknown?\n");
				break;
			}
			break;
		default:
			fprintf(stderr, "Usage: %s [ -h | -s <input_file> <output_file> ]\n", argv[0]);
			exit(1);
		}
	}

	return 0;
}
