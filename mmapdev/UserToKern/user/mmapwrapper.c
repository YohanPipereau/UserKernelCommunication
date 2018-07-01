/*
 *    Description: mmap wrapper for shared memory char dev
 *        Created:  26/06/2018
 *         Author:  Yohan Pipereau
 */


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#define FILENAME "/dev/mmdev"
#define MAP_LEN 4096
#define MSG_SIZE 1000

/*  Start time measurement; CPUID prevent out of order execution */
#define start_timer() asm volatile ("CPUID\n\t" \
		"RDTSC\n\t" \
		"mov %%edx, %0\n\t" \
		"mov %%eax, %1\n\t": "=r" (cycles_high), \
		"=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");


#define stop_timer() asm volatile("RDTSCP\n\t" \
		"mov %%edx, %0\n\t" \
		"mov %%eax, %1\n\t" \
		"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: \
		"%rax", "%rbx", "%rcx", "%rdx");

struct record {
	char msg[MSG_SIZE];
	int pending;
};

int main ( int argc, char *argv[] )
{
	int fd;
	struct record *map;
	char buffer[MSG_SIZE];
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;
	int seq = 0;
	struct record *rec;

	fd = open(FILENAME, O_RDWR);
	if (fd == -1) {
		perror("failed opening " FILENAME);
		return errno;
	}

	fgets((char*) buffer, MSG_SIZE, stdin);
	start_timer();

	rec = malloc(sizeof(struct record));
	if (rec == NULL) {
		printf("malloc failed \n");
		return EXIT_FAILURE;
	}
	map = (struct record*) mmap(NULL, MAP_LEN, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		close(fd);
		perror("mmap failed");
		return errno;
	}

	if (!ftruncate(fd, MAP_LEN)) {
		perror("ftruncate failed");
		return errno;
	}

	strcpy(rec->msg, buffer);
	rec->pending = 1;
	if (map->pending == 0) {
		memcpy(map, rec, sizeof(struct record));
		seq++;
	}
	stop_timer();
	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printf("First sending : %lu clock cycles\n", end-start);

	while (1) {
		fgets((char*) buffer, MSG_SIZE, stdin);
		start_timer();
		if (map->pending == 0) {
			strcpy(rec->msg, buffer);
			rec->pending = 1;
			memcpy(map, rec, sizeof(struct record));
			seq++;
		}
		stop_timer();
		start = ( ((uint64_t)cycles_high << 32) | cycles_low );
		end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
		printf("Sending n° %d, %lu clock cycles\n", seq, end-start);
	}

	/*
	 * msync not required :
	 * https://www.spinics.net/lists/linux-c-programming/msg01027.html
	 */

	free(rec);
	rec = NULL;
	if (munmap(map, MAP_LEN) == -1) {
		close(fd);
		perror("failed munmap");
		return errno;
	}

	close(fd);

	return EXIT_SUCCESS;
}
