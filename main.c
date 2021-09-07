/* © Mikhail Gozhev <m@gozhev.ru> / Autumn 2021 / Moscow, Russia */

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct {
	char** argv;
	int pid;
} prog_t;

typedef struct {
	prog_t* data;
	int size;
} prog_list_t;

void prog_list_release(prog_list_t* prog_list)
{
	free(prog_list->data);
	prog_list->data = NULL;
	prog_list->size = 0;
}

void print_usage(char const* prog_path)
{
	char* last_slash = strrchr(prog_path, '/');
	char const* prog_name = NULL;
	if (last_slash == NULL) {
		prog_name = prog_path;
	} else {
		prog_name = last_slash + 1;
	}
	printf("Usage: %s [-h] program1 [ARG ...] ; "
			"program2 [ARG ...] ...\n", prog_name);
}

int scan_opts(int argc, char* const* argv)
{
	char const optstring[] = "+h";
	int opt = 0;
	while ((opt = getopt(argc, argv, optstring)) != -1) {
		switch (opt) {
		case 'h':
			print_usage(*argv);
			return 0;
			break;
		default: /* '?' */
			print_usage(*argv);
			return -1;
			break;
		}
	}
	return optind;
}

int scan_args(int argc, char** argv, int arg_ind, prog_list_t* prog_list)
{
	int count = 0;
	prog_t* list = 0;
	bool next = true;
	int prog_ind = 0;
	for (int i = arg_ind; i < argc; ++i) {
		if (strcmp(argv[i], ";") == 0) {
			argv[i] = NULL;
			next = true;
		} else if (next) {
			++count;
			next = false;
		}
	}
	if (count == 0) {
		fprintf(stderr, "no programs given\n\n");
		print_usage(*argv);
		return -1;
	}
	list = calloc(count, sizeof(prog_t));
	if (list == NULL) {
		fprintf(stderr, "failed to allocate memory\n");
		return -1;
	}
	next = true;
	for (int i = arg_ind; i < argc; ++i) {
		if (argv[i] == NULL) {
			next = true;
		} else if (next) {
			list[prog_ind++].argv = argv + i;
			next = false;
		}
	}
	prog_list->size = count;
	prog_list->data = list;
	return 0;
}

int run_program(prog_t* prog)
{
	int retval = 0;
	if ((retval = fork()) == -1) {
		perror("failed to fork");
		goto exit;
	} else if (retval == 0) { /* child */
		char* prog_path = prog->argv[0];
		if ((retval = execv(prog_path, prog->argv)) == -1) {
			fprintf(stderr, "failed to execute '%s': %s\n",
					prog_path, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	prog->pid = retval;
	retval = 0;
exit:
	return retval;
}

int start_processes(prog_list_t* prog_list)
{
	int retval = 0;
	for (int i = 0; i < prog_list->size; ++i) {
		if ((retval = run_program(&prog_list->data[i])) == -1) {
			goto exit;
		}
	}
exit:
	return retval;
}

int stop_processes(prog_list_t const* prog_list)
{
	int retval = 0;
	for (int i = 0; i < prog_list->size; ++i) {
		kill(prog_list->data[i].pid, SIGKILL);
	}
	for (;;) {
		if ((retval = wait(NULL)) == -1) {
			if (errno == ECHILD) {
				break;
			}
		}
	}
	return 0;
}

int set_up_signals(sigset_t* sigset)
{
	int retval = 0;
	if ((retval = sigemptyset(sigset)) == -1) {
		perror("failed to init a signal set");
		goto exit;
	}
	if ((retval = sigaddset(sigset, SIGCHLD)) == -1) {
		perror("failed to add a signal");
		goto exit;
	}
	if ((retval = sigprocmask(SIG_BLOCK, sigset, NULL)) == -1) {
		perror("failed to set a signal mask");
		goto exit;
	}
exit:
	return retval;
}

int handle_signals(sigset_t const* sigset, prog_list_t const* prog_list)
{
	int retval = 0;
	siginfo_t siginfo = {0};
	for (;;) {
		if ((retval = sigwaitinfo(sigset, &siginfo)) == -1) {
			fprintf(stderr, "failed to wait for a signal: %s\n",
					strerror(retval));
			goto exit;
		}
		/*
		 * TODO: add here an ability to restart a program
		 * depending on the options
		 */
		(void) prog_list;
		retval = 0;
		goto exit;
	}
exit:
	return retval;
}

int main(int argc, char** argv)
{
	int retval = 0;
	prog_list_t prog_list = {0};
	sigset_t sigset = {0};
	if ((retval = scan_opts(argc, argv)) <= 0) {
		goto exit;
	}
	if ((retval = scan_args(argc, argv, retval, &prog_list)) == -1) {
		goto exit;
	}
	if ((retval = set_up_signals(&sigset)) == -1) {
		goto exit_release_prog_list;
	}
	if ((retval = start_processes(&prog_list)) == -1) {
		goto exit_release_prog_list;
	}
	if ((retval = handle_signals(&sigset, &prog_list)) == -1) {
		goto exit_stop_processes;
	}
exit_stop_processes:
	stop_processes(&prog_list);
exit_release_prog_list:
	prog_list_release(&prog_list);
exit:
	exit(retval);
}
