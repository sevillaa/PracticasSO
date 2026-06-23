#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_COMMANDS 1024
#define MAX_LINE 1024

pid_t launch_command(char **argv)
{
	pid_t pid;

	fflush(stdout);

	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid == 0) {
		execvp(argv[0], argv);
		perror("execvp");
		exit(EXIT_FAILURE);
	}

	return pid;
}

char **parse_command(const char *cmd, int *argc)
{
	size_t argv_size = 10;
	const char *end;
	size_t arg_len;
	int arg_count = 0;
	const char *start = cmd;
	char **argv = malloc(argv_size * sizeof(char *));

	if (argv == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	while (*start && isspace(*start))
		start++;

	while (*start) {
		if (arg_count >= argv_size - 1) {
			argv_size *= 2;
			argv = realloc(argv, argv_size * sizeof(char *));
			if (argv == NULL) {
				perror("realloc");
				exit(EXIT_FAILURE);
			}
		}

		end = start;
		while (*end && !isspace(*end))
			end++;

		arg_len = end - start;
		argv[arg_count] = malloc(arg_len + 1);
		if (argv[arg_count] == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}

		strncpy(argv[arg_count], start, arg_len);
		argv[arg_count][arg_len] = '\0';
		arg_count++;

		start = end;
		while (*start && isspace(*start))
			start++;
	}

	argv[arg_count] = NULL;
	*argc = arg_count;

	return argv;
}

void free_command(char **cmd_argv)
{
	int i;

	for (i = 0; cmd_argv[i] != NULL; i++)
		free(cmd_argv[i]);

	free(cmd_argv);
}

void trim_newline(char *line)
{
	line[strcspn(line, "\n")] = '\0';
}

void terminal_mode(void)
{
	char buffer[MAX_LINE];
	char **comando;
	int comando_argc;
	pid_t pid;
	int status;

	while (1) {
		printf("> ");
		fflush(stdout);

		if (fgets(buffer, sizeof(buffer), stdin) == NULL)
			break;

		trim_newline(buffer);

		if (strcmp(buffer, "exit") == 0)
			break;

		comando = parse_command(buffer, &comando_argc);

		if (comando[0] != NULL) {
			pid = launch_command(comando);
			if (waitpid(pid, &status, 0) == -1) {
				perror("waitpid");
				exit(EXIT_FAILURE);
			}
		}

		free_command(comando);
	}
}

void run_one_command(char *cmd)
{
	char **comando;
	int comando_argc;
	pid_t pid;
	int status;

	comando = parse_command(cmd, &comando_argc);

	if (comando[0] != NULL) {
		pid = launch_command(comando);
		if (waitpid(pid, &status, 0) == -1) {
			perror("waitpid");
			exit(EXIT_FAILURE);
		}
	}

	free_command(comando);
}

void run_commands_file(char *filename, int background)
{
	FILE *file;
	char buffer[MAX_LINE];
	char command_line[MAX_LINE];
	char **comando;
	int comando_argc;
	pid_t pid;
	int status;
	int command_num = 0;
	pid_t pids[MAX_COMMANDS];
	int command_nums[MAX_COMMANDS];
	int num_children = 0;

	file = fopen(filename, "r");
	if (file == NULL) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	while (fgets(buffer, sizeof(buffer), file) != NULL) {
		strncpy(command_line, buffer, sizeof(command_line) - 1);
		command_line[sizeof(command_line) - 1] = '\0';
		trim_newline(command_line);

		comando = parse_command(buffer, &comando_argc);

		if (comando[0] != NULL) {
			printf("@@ Running command #%d: %s\n", command_num, command_line);
			pid = launch_command(comando);

			if (background) {
				if (num_children >= MAX_COMMANDS) {
					fprintf(stderr, "Too many commands\n");
					fclose(file);
					free_command(comando);
					exit(EXIT_FAILURE);
				}

				pids[num_children] = pid;
				command_nums[num_children] = command_num;
				num_children++;
			} else {
				if (waitpid(pid, &status, 0) == -1) {
					perror("waitpid");
					fclose(file);
					free_command(comando);
					exit(EXIT_FAILURE);
				}

				printf("@@ Command #%d terminated (pid: %d, status: %d)\n",
				       command_num, (int)pid, status);
			}

			command_num++;
		}

		free_command(comando);
	}

	fclose(file);

	if (background) {
		int finished;

		for (finished = 0; finished < num_children; finished++) {
			pid_t ended_pid;
			int i;
			int ended_command_num = -1;

			ended_pid = wait(&status);
			if (ended_pid == -1) {
				perror("wait");
				exit(EXIT_FAILURE);
			}

			for (i = 0; i < num_children; i++) {
				if (pids[i] == ended_pid) {
					ended_command_num = command_nums[i];
					break;
				}
			}

			printf("@@ Command #%d terminated (pid: %d, status: %d)\n",
			       ended_command_num, (int)ended_pid, status);
		}
	}
}

int main(int argc, char *argv[])
{
	int opt;
	int background = 0;
	int terminal = 0;
	char *command = NULL;
	char *script = NULL;

	while ((opt = getopt(argc, argv, "x:s:bt")) != -1) {
		switch (opt) {
		case 'x':
			command = optarg;
			break;
		case 's':
			script = optarg;
			break;
		case 'b':
			background = 1;
			break;
		case 't':
			terminal = 1;
			break;
		default:
			fprintf(stderr, "Usage: %s [-x command] [-s file] [-b] [-t]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (terminal) {
		terminal_mode();
		return EXIT_SUCCESS;
	}

	if (command != NULL)
		run_one_command(command);

	if (script != NULL)
		run_commands_file(script, background);

	return EXIT_SUCCESS;
}
