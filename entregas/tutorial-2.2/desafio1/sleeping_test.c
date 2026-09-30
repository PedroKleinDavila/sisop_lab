#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SYS_LIST_SLEEPING_PROCESSES 386
#define CAPACITY 4096

static int contains(const pid_t *pids, long count, pid_t expected)
{
	long i;
	for (i = 0; i < count; ++i)
		if (pids[i] == expected)
			return 1;
	return 0;
}

int main(void)
{
	static pid_t pids[CAPACITY];
	struct timespec delay = { .tv_sec = 0, .tv_nsec = 200000000 };
	int pipefd[2];
	pid_t child;
	long count;
	char byte = 'x';

	if (pipe(pipefd) < 0) {
		perror("pipe");
		return EXIT_FAILURE;
	}
	child = fork();
	if (child < 0) {
		perror("fork");
		return EXIT_FAILURE;
	}
	if (child == 0) {
		close(pipefd[1]);
		read(pipefd[0], &byte, 1);
		_exit(0);
	}
	close(pipefd[0]);
	nanosleep(&delay, NULL);
	count = syscall(SYS_LIST_SLEEPING_PROCESSES, pids, CAPACITY);
	if (count < 0) {
		perror("listSleepingProcesses");
		goto fail;
	}
	printf("Processos dormindo: %ld; filho %ld: %s\n",
		count, (long)child, contains(pids, count, child) ? "presente" : "ausente");
	if (!contains(pids, count, child))
		goto fail;
	errno = 0;
	if (syscall(SYS_LIST_SLEEPING_PROCESSES, (void *)1, CAPACITY) != -1 ||
	    errno != EFAULT) {
		fprintf(stderr, "Ponteiro inválido deveria retornar EFAULT\n");
		goto fail;
	}
	write(pipefd[1], &byte, 1);
	close(pipefd[1]);
	waitpid(child, NULL, 0);
	return EXIT_SUCCESS;
fail:
	write(pipefd[1], &byte, 1);
	close(pipefd[1]);
	waitpid(child, NULL, 0);
	return EXIT_FAILURE;
}
