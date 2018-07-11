/*
 *    Description:  Wrapper to read kernel info sentt using relay API.
 *        Created:  05/07/2018
 *         Author:  Yohan Pipereau
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>

#define SUBBUF_SIZE 262144
#define N_SUBBUFS 4
#define NB_MSG 100
#define MSG_SIZE 1024

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

int main ( int argc, char *argv[] )
{
	int nb_proc = get_nprocs();
	int fd[nb_proc];
	char *buf[nb_proc];
	char base[] = "/sys/kernel/debug/relay/relay";
	char *filename;
	int size;
	int cmpt=0;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;

	/*  Opening */
	for (int i = 0; i < nb_proc; i++) {
		filename = malloc(strlen(base)+i/10);
		sprintf(filename, "%s%d",base,i);
		fd[i] = open(filename, O_RDONLY);
		if (fd[i] < 0) {
			perror("failed opening");
			return errno;
		}
		free(filename);
		filename = NULL;
		if (!fd[i]) {
			perror("failed opening");
			return errno;
		}
		buf[i] = malloc(MSG_SIZE);
	}

	start_timer();
	while (cmpt < NB_MSG) {
		//#pragma omp parallel
		for (int i = 0; i < nb_proc; i++) {
			size = read(fd[i], buf[i], MSG_SIZE);
			if (size < 0) {
				fprintf(stderr,"read failed");
				exit(EXIT_FAILURE);
			}
		}
		cmpt++;
	}
	stop_timer();

	printf("received %d messages\n", cmpt);
	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printf("%lu clock cycles\n", (end-start));


	/* Closing */
	for (int i = 0; i < nb_proc; i++) {
		free(buf[i]);
		buf[i] = NULL;
		close(fd[i]);
		printf("File relay/relay%d closed\n", i);
	}

	return EXIT_SUCCESS;
}


