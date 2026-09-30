#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEVICE "/dev/xtea_driver"
#define PLAINTEXT "1234567890123456"

static int exchange(int fd, const char *operation, const char *input,
		    char *output, size_t output_size)
{
	char command[128];
	ssize_t length;
	int written = snprintf(command, sizeof(command), "%s 8 %s",
			       operation, input);
	if (written < 0 || (size_t)written >= sizeof(command))
		return -1;
	if (write(fd, command, written) != written)
		return -1;
	length = read(fd, output, output_size - 1);
	if (length <= 0)
		return -1;
	output[length] = '\0';
	if (length > 0 && output[length - 1] == '\n')
		output[length - 1] = '\0';
	return 0;
}

int main(void)
{
	char cipher[64], recovered[64];
	int fd = open(DEVICE, O_RDWR);
	if (fd < 0) {
		perror("open " DEVICE);
		return EXIT_FAILURE;
	}
	if (exchange(fd, "enc", PLAINTEXT, cipher, sizeof(cipher)) ||
	    !strcmp(cipher, PLAINTEXT) ||
	    exchange(fd, "dec", cipher, recovered, sizeof(recovered)) ||
	    strcmp(recovered, PLAINTEXT)) {
		fprintf(stderr, "Falha no teste de ida e volta XTEA (errno=%d)\n", errno);
		close(fd);
		return EXIT_FAILURE;
	}
	printf("Cifrado: %s\nDecifrado: %s\n", cipher, recovered);
	errno = 0;
	if (write(fd, "enc 7 00000000000000", 20) != -1 || errno != EINVAL) {
		fprintf(stderr, "Tamanho inválido deveria retornar EINVAL\n");
		close(fd);
		return EXIT_FAILURE;
	}
	close(fd);
	puts("Teste XTEA concluído.");
	return EXIT_SUCCESS;
}
