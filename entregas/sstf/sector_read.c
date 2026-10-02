#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define DISK_BYTES (1024ULL * 1024 * 1024)
#define PAGE_BYTES 4096
#define DEFAULT_WORKERS 16
#define DEFAULT_READS 16

static int worker(unsigned int number, int barrier, int reads)
{
	void *buffer;
	uint64_t bytes;
	unsigned int seed = 0x5a17u + number * 7919u;
	int fd, index;
	char signal;

	fd = open("/dev/sdb", O_RDONLY | O_DIRECT);
	if (fd < 0 || ioctl(fd, BLKGETSIZE64, &bytes) < 0 ||
	    bytes != DISK_BYTES) {
		perror("/dev/sdb");
		if (fd >= 0)
			close(fd);
		return 1;
	}
	if (posix_memalign(&buffer, PAGE_BYTES, PAGE_BYTES)) {
		close(fd);
		return 1;
	}
	if (read(barrier, &signal, 1) != 1) {
		free(buffer);
		close(fd);
		return 1;
	}
	for (index = 0; index < reads; ++index) {
		off_t offset = (rand_r(&seed) % (DISK_BYTES / PAGE_BYTES)) *
			       PAGE_BYTES;
		if (pread(fd, buffer, PAGE_BYTES, offset) != PAGE_BYTES) {
			perror("pread");
			free(buffer);
			close(fd);
			return 1;
		}
	}
	free(buffer);
	close(fd);
	return 0;
}

int main(int argc, char **argv)
{
	int workers = DEFAULT_WORKERS, reads = DEFAULT_READS;
	int start[2], index, status, failed = 0;
	pid_t child;

	if (argc > 1)
		workers = atoi(argv[1]);
	if (argc > 2)
		reads = atoi(argv[2]);
	if (argc > 3 || workers < 1 || workers > 32 ||
	    reads < 1 || reads > 64) {
		fprintf(stderr, "Uso: %s [trabalhadores 1..32] [leituras 1..64]\n",
			argv[0]);
		return EXIT_FAILURE;
	}
	if (pipe(start) < 0) {
		perror("pipe");
		return EXIT_FAILURE;
	}
	for (index = 0; index < workers; ++index) {
		child = fork();
		if (child < 0) {
			perror("fork");
			failed = 1;
			break;
		}
		if (child == 0) {
			close(start[1]);
			_exit(worker(index, start[0], reads));
		}
	}
	close(start[0]);
	for (int released = 0; released < index; ++released)
		if (write(start[1], "x", 1) != 1)
			failed = 1;
	close(start[1]);
	for (int waited = 0; waited < index; ++waited) {
		if (wait(&status) < 0 || !WIFEXITED(status) ||
		    WEXITSTATUS(status) != 0)
			failed = 1;
	}
	printf("Trabalhadores: %d; leituras por trabalhador: %d; %s\n",
		index, reads, failed ? "falha" : "concluído");
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
