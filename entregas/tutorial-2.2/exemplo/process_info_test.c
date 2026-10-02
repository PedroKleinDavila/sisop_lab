#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SYS_LIST_PROCESS_INFO 385

int main(int argc, char **argv)
{
	char buffer[256];
	long pid = argc > 1 ? strtol(argv[1], NULL, 10) : getpid();
	long result = syscall(SYS_LIST_PROCESS_INFO, pid, buffer, sizeof(buffer));

	if (result < 0) {
		perror("listProcessInfo");
		return EXIT_FAILURE;
	}
	printf("%s", buffer);
	return EXIT_SUCCESS;
}
