#include <stdio.h>
#include <unistd.h> /* for getopt() */
#include <stdlib.h> /* for EXIT_SUCCESS, EXIT_FAILURE */
#include <string.h>
#include "defs.h"

/* Assume lines in the text file are no larger than 100 chars */
#define MAXLEN_LINE_FILE 100

char *loadstr(FILE *file)
{
	long pos_actual;
	int c;
	size_t count = 0;
	char *str;

	pos_actual = ftell(file);
	if (pos_actual == -1)
		return NULL;

	while ((c = fgetc(file)) != EOF) {
		count++;
		if (c == '\0')
			break;
	}

	if (c == EOF)
		return NULL;

	if (fseek(file, pos_actual, SEEK_SET) != 0)
		return NULL;

	str = malloc(count);
	if (str == NULL)
		return NULL;

	if (fread(str, 1, count, file) != count) {
		free(str);
		return NULL;
	}

	return str;
}

void free_students(student_t *students, int nr_entries)
{
	int i;

	for (i = 0; i < nr_entries; i++) {
		free(students[i].first_name);
		free(students[i].last_name);
	}

	free(students);
}

student_t *read_student_text_file(FILE *students, int *nr_entries)
{
	student_t *array;
	char line[MAXLEN_LINE_FILE];
	int i;

	if (fgets(line, MAXLEN_LINE_FILE, students) == NULL)
		return NULL;

	*nr_entries = atoi(line);

	array = malloc(*nr_entries * sizeof(student_t));
	if (array == NULL)
		return NULL;

	for (i = 0; i < *nr_entries; i++) {
		char *token;

		if (fgets(line, MAXLEN_LINE_FILE, students) == NULL) {
			free_students(array, i);
			return NULL;
		}

		token = strtok(line, ":");
		if (token == NULL) {
			free_students(array, i);
			return NULL;
		}
		array[i].student_id = atoi(token);

		token = strtok(NULL, ":");
		if (token == NULL) {
			free_students(array, i);
			return NULL;
		}
		strcpy(array[i].NIF, token);

		token = strtok(NULL, ":");
		if (token == NULL) {
			free_students(array, i);
			return NULL;
		}
		array[i].first_name = strdup(token);
		if (array[i].first_name == NULL) {
			free_students(array, i);
			return NULL;
		}

		token = strtok(NULL, ":\n");
		if (token == NULL) {
			free(array[i].first_name);
			free_students(array, i);
			return NULL;
		}
		array[i].last_name = strdup(token);
		if (array[i].last_name == NULL) {
			free(array[i].first_name);
			free_students(array, i);
			return NULL;
		}
	}

	return array;
}

student_t *read_student_binary_file(FILE *students, int *nr_entries)
{
	student_t *array;
	int i;

	if (fread(nr_entries, sizeof(int), 1, students) != 1)
		return NULL;

	array = malloc(*nr_entries * sizeof(student_t));
	if (array == NULL)
		return NULL;

	for (i = 0; i < *nr_entries; i++) {
		if (fread(&array[i].student_id, sizeof(int), 1, students) != 1) {
			free_students(array, i);
			return NULL;
		}

		if (fread(array[i].NIF, sizeof(char), MAX_CHARS_NIF + 1, students) != MAX_CHARS_NIF + 1) {
			free_students(array, i);
			return NULL;
		}

		array[i].first_name = loadstr(students);
		if (array[i].first_name == NULL) {
			free_students(array, i);
			return NULL;
		}

		array[i].last_name = loadstr(students);
		if (array[i].last_name == NULL) {
			free(array[i].first_name);
			free_students(array, i);
			return NULL;
		}
	}

	return array;
}

int print_text_file(char *path)
{
	FILE *file;
	student_t *students;
	int nr_entries;
	int i;

	file = fopen(path, "r");
	if (file == NULL) {
		perror("fopen");
		return EXIT_FAILURE;
	}

	students = read_student_text_file(file, &nr_entries);
	fclose(file);

	if (students == NULL)
		return EXIT_FAILURE;

	for (i = 0; i < nr_entries; i++) {
		printf("[Entry #%d]\n", i);
		printf("\tstudent_id=%d\n", students[i].student_id);
		printf("\tNIF=%s\n", students[i].NIF);
		printf("\tfirst_name=%s\n", students[i].first_name);
		printf("\tlast_name=%s\n", students[i].last_name);
	}

	free_students(students, nr_entries);
	return EXIT_SUCCESS;
}

int print_binary_file(char *path)
{
	FILE *file;
	student_t *students;
	int nr_entries;
	int i;

	file = fopen(path, "rb");
	if (file == NULL) {
		perror("fopen");
		return EXIT_FAILURE;
	}

	students = read_student_binary_file(file, &nr_entries);
	fclose(file);

	if (students == NULL)
		return EXIT_FAILURE;

	for (i = 0; i < nr_entries; i++) {
		printf("[Entry #%d]\n", i);
		printf("\tstudent_id=%d\n", students[i].student_id);
		printf("\tNIF=%s\n", students[i].NIF);
		printf("\tfirst_name=%s\n", students[i].first_name);
		printf("\tlast_name=%s\n", students[i].last_name);
	}

	free_students(students, nr_entries);
	return EXIT_SUCCESS;
}

int write_binary_file(char *input_file, char *output_file)
{
	FILE *file;
	FILE *file_w;
	student_t *students;
	int nr_entries;
	int i;

	if (output_file == NULL) {
		fprintf(stderr, "Must specify one output file as an argument of -o\n");
		return EXIT_FAILURE;
	}

	file = fopen(input_file, "r");
	if (file == NULL) {
		perror("fopen");
		return EXIT_FAILURE;
	}

	students = read_student_text_file(file, &nr_entries);
	fclose(file);

	if (students == NULL)
		return EXIT_FAILURE;

	file_w = fopen(output_file, "wb");
	if (file_w == NULL) {
		perror("fopen");
		free_students(students, nr_entries);
		return EXIT_FAILURE;
	}

	if (fwrite(&nr_entries, sizeof(int), 1, file_w) != 1) {
		fclose(file_w);
		free_students(students, nr_entries);
		return EXIT_FAILURE;
	}

	for (i = 0; i < nr_entries; i++) {
		if (fwrite(&students[i].student_id, sizeof(int), 1, file_w) != 1 ||
		    fwrite(students[i].NIF, sizeof(char), MAX_CHARS_NIF + 1, file_w) != MAX_CHARS_NIF + 1 ||
		    fwrite(students[i].first_name, sizeof(char), strlen(students[i].first_name) + 1, file_w) != strlen(students[i].first_name) + 1 ||
		    fwrite(students[i].last_name, sizeof(char), strlen(students[i].last_name) + 1, file_w) != strlen(students[i].last_name) + 1) {
			fclose(file_w);
			free_students(students, nr_entries);
			return EXIT_FAILURE;
		}
	}

	fclose(file_w);
	printf("%d student records written successfully to binary file %s\n", nr_entries, output_file);

	free_students(students, nr_entries);
	return EXIT_SUCCESS;
}

int append_records_file(char *path, int num_new_records, char **new_records)
{
	FILE *file;
	student_t *students;
	int num_students;
	int total_students;
	int i;
	int len;

	if (num_new_records <= 0) {
		fprintf(stderr, "Must specify at least one student record after -a\n");
		return EXIT_FAILURE;
	}

	len = strlen(path);

	if (len >= 4 && strcmp(path + len - 4, ".txt") == 0) {
		file = fopen(path, "r");
		if (file == NULL) {
			perror("fopen");
			return EXIT_FAILURE;
		}

		students = read_student_text_file(file, &num_students);
		fclose(file);

		if (students == NULL)
			return EXIT_FAILURE;

		file = fopen(path, "w");
		if (file == NULL) {
			perror("fopen");
			free_students(students, num_students);
			return EXIT_FAILURE;
		}

		total_students = num_students + num_new_records;
		fprintf(file, "%d\n", total_students);

		for (i = 0; i < num_students; i++) {
			fprintf(file, "%d:%s:%s:%s\n",
				students[i].student_id,
				students[i].NIF,
				students[i].first_name,
				students[i].last_name);
		}

		for (i = 0; i < num_new_records; i++)
			fprintf(file, "%s\n", new_records[i]);

		free_students(students, num_students);
		fclose(file);

		printf("%d records written successfully to existing text file\n", num_new_records);
		return EXIT_SUCCESS;
	}

	if (len >= 4 && strcmp(path + len - 4, ".bin") == 0) {
		file = fopen(path, "r+b");
		if (file == NULL) {
			perror("fopen");
			return EXIT_FAILURE;
		}

		if (fread(&num_students, sizeof(int), 1, file) != 1) {
			fclose(file);
			return EXIT_FAILURE;
		}

		total_students = num_students + num_new_records;

		fseek(file, 0, SEEK_SET);
		fwrite(&total_students, sizeof(int), 1, file);
		fseek(file, 0, SEEK_END);

		for (i = 0; i < num_new_records; i++) {
			char line[MAXLEN_LINE_FILE];
			char *token;
			int student_id;
			char NIF[MAX_CHARS_NIF + 1];
			char *first_name;
			char *last_name;

			strncpy(line, new_records[i], MAXLEN_LINE_FILE - 1);
			line[MAXLEN_LINE_FILE - 1] = '\0';

			token = strtok(line, ":");
			if (token == NULL) {
				fclose(file);
				return EXIT_FAILURE;
			}
			student_id = atoi(token);

			token = strtok(NULL, ":");
			if (token == NULL) {
				fclose(file);
				return EXIT_FAILURE;
			}
			memset(NIF, 0, sizeof(NIF));
			strncpy(NIF, token, MAX_CHARS_NIF);

			first_name = strtok(NULL, ":");
			last_name = strtok(NULL, ":\n");
			if (first_name == NULL || last_name == NULL) {
				fclose(file);
				return EXIT_FAILURE;
			}

			fwrite(&student_id, sizeof(int), 1, file);
			fwrite(NIF, sizeof(char), MAX_CHARS_NIF + 1, file);
			fwrite(first_name, sizeof(char), strlen(first_name) + 1, file);
			fwrite(last_name, sizeof(char), strlen(last_name) + 1, file);
		}

		fclose(file);

		printf("%d records written successfully to existing binary file\n", num_new_records);
		return EXIT_SUCCESS;
	}

	fprintf(stderr, "Input file must end with .txt or .bin\n");
	return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
	int ret_code, opt;
	struct options options;

	options.input_file = NULL;
	options.output_file = NULL;
	options.action = NONE_ACT;
	ret_code = EXIT_SUCCESS;

	while ((opt = getopt(argc, argv, "hi:po:ba")) != -1)
	{
		switch (opt)
		{
		case 'h':
			fprintf(stderr, "Usage: %s [ -h | -p | -i file | -o <output_file> | -b | -a ]\n", argv[0]);
			exit(EXIT_SUCCESS);
		case 'i':
			options.input_file = optarg;
			break;
		case 'p':
			options.action = PRINT_TEXT_ACT;
			break;
		case 'o':
			options.action = WRITE_BINARY_ACT;
			options.output_file = optarg;
			break;
		case 'b':
			options.action = PRINT_BINARY_ACT;
			break;
		case 'a':
			options.action = APPEND_ACT;
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}

	if (options.input_file == NULL)
	{
		fprintf(stderr, "Must specify one record file as an argument of -i\n");
		exit(EXIT_FAILURE);
	}

	switch (options.action)
	{
	case NONE_ACT:
		fprintf(stderr, "Must indicate one of the following options: -p, -o, -b, -a \n");
		ret_code = EXIT_FAILURE;
		break;
	case PRINT_TEXT_ACT:
		ret_code = print_text_file(options.input_file);
		break;
	case WRITE_BINARY_ACT:
		ret_code = write_binary_file(options.input_file, options.output_file);
		break;
	case PRINT_BINARY_ACT:
		ret_code = print_binary_file(options.input_file);
		break;
	case APPEND_ACT:
		ret_code = append_records_file(options.input_file, argc - optind, &argv[optind]);
		break;
	default:
		ret_code = EXIT_FAILURE;
		break;
	}

	exit(ret_code);
}
