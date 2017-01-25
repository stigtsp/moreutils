#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Licensed under the GPL
 * Copyright (c) Miek Gieben, 2006
 */

/* like tee(1), but then connect to other programs using
 * pipes _and_ output to standard output
 */

int
close_pipes(FILE **p, size_t i) 
{
	int ret=EXIT_SUCCESS;
	size_t j;
	for (j = 0; j < i; j++) {
		int r = pclose(p[j]);
		if (WIFEXITED(r))
			ret |= WEXITSTATUS(r);
		else
			ret |= 1;
	}
	return ret;
}

int
main(int argc, char **argv) {
	int ignore_write_error = 1;
	int ignore_sigpipe = 1;
	size_t i, r;
	FILE **pipes;
	int *inactive_pipe;
	int inactive_pipes = 0;
	char buf[BUFSIZ];

	while(argc > 1) {
		if (!strcmp(argv[1], "--no-ignore-sigpipe")) {
			argc--, argv++;
			ignore_sigpipe = 0;
			continue;
		} else if (!strcmp(argv[1], "--ignore-sigpipe")) {
			argc--, argv++;
			ignore_sigpipe = 1;
			continue;
		} else if (!strcmp(argv[1], "--no-ignore-write-errors")) {
			argc--, argv++;
			ignore_write_error = 0;
			continue;
		} else if (!strcmp(argv[1], "--ignore-write-errors")) {
			argc--, argv++;
			ignore_write_error = 1;
			continue;
		}
		break;
	}

	if (ignore_sigpipe && (signal(SIGPIPE, SIG_IGN) == SIG_ERR)) {
		fprintf(stderr, "Unable to ignore SIGPIPE\n");
		exit(EXIT_FAILURE);
	}

	pipes = malloc(((argc - 1) * sizeof *pipes));
	inactive_pipe = calloc((argc - 1), (sizeof *inactive_pipe));
	if (!pipes || !inactive_pipe)
		exit(EXIT_FAILURE);

	for (i = 1; i < argc; i++) {
		pipes[i - 1] = popen(argv[i], "w");
		if (!pipes[i - 1]) {
			fprintf(stderr, "Can not open pipe to '%s\'\n", argv[i]);
			close_pipes(pipes, argc);

			exit(EXIT_FAILURE);
		}

		setbuf(pipes[i - 1], NULL);
	}
	argc--;

	for (;;) {
		r = read(STDIN_FILENO, buf, BUFSIZ);

		/* Interrupted by signal? Try again. */
		if (r == -1 && errno == EINTR)
			continue;
		
		/* Other error or EOF. */
		if (r < 1)
			break;
		
		for(i = 0; i < argc; i++) {
			if (inactive_pipe[i])
				continue;

			if (fwrite(buf, sizeof(char), r, pipes[i]) == r)
				continue;

			inactive_pipes++;

			if (!ignore_write_error)
				fprintf(stderr, "Write error to `%s\'\n",
					argv[i + 1]);

			if (!ignore_write_error || (inactive_pipes == argc)) {
				close_pipes(pipes, argc);
				exit(EXIT_FAILURE);
			}

			inactive_pipe[i] = 1;
		}
	}
	exit(close_pipes(pipes, argc));
}
