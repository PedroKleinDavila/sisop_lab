#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define SYS_LOG_USER_MESSAGE 387

int main(void)
{
	char longest[256];
	char excessive[257];
	long result;

	result = syscall(SYS_LOG_USER_MESSAGE, "desafio 2 OK");
	if (result != (long)strlen("desafio 2 OK")) {
		perror("logUserMessage");
		return EXIT_FAILURE;
	}
	memset(longest, 'a', 255);
	longest[255] = '\0';
	if (syscall(SYS_LOG_USER_MESSAGE, longest) != 255)
		return EXIT_FAILURE;
	memset(excessive, 'b', 256);
	excessive[256] = '\0';
	errno = 0;
	if (syscall(SYS_LOG_USER_MESSAGE, excessive) != -1 || errno != E2BIG)
		return EXIT_FAILURE;
	errno = 0;
	if (syscall(SYS_LOG_USER_MESSAGE, "") != -1 || errno != EINVAL)
		return EXIT_FAILURE;
	errno = 0;
	if (syscall(SYS_LOG_USER_MESSAGE, (char *)1) != -1 || errno != EFAULT)
		return EXIT_FAILURE;
	puts("Testes da syscall concluídos; confira dmesg | grep 'SISOP user message'.");
	return EXIT_SUCCESS;
}
