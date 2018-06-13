/*
 *    Description:
 *        Created:  10/06/2018
 *         Author:  Yohan Pipereau
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define SIZE 1024
#define NB_MSG 100

#define start_count() \
	asm volatile("CPUID\n\t" \
			"RDTSC\n\t" \
			"mov %%rdx, %0\n\t" \
			"mov %%rax, %1\n\t": "=r" (cycles_high), \
			"=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");

#define end_count() \
	asm volatile("RDTSCP\n\t" \
			"mov %%rdx, %0\n\t" \
			"mov %%rax, %1\n\t" \
			"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1)::\
			"%rax", "%rbx", "%rcx", "%rdx");

int main ( int argc, char *argv[] )
{
	unsigned long cycles_high, cycles_high1, cycles_low, cycles_low1;
	char *buff = malloc(1024);
	uint64_t start, end;
	unsigned long send = 0;
	unsigned long prepare = 0;

	int fd = open("/dev/bmb", O_WRONLY);
	for (int i = 0; i < NB_MSG; i++) {

		/*  Prepare message */
		start_count();
		memset(buff, 'a', SIZE);
		end_count();
		start = ( ((uint64_t)cycles_high << 32) | cycles_low );
		end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
		prepare += end-start;

		/* Send messgae */
		start_count();
		write(fd, buff, SIZE);
		end_count();
		start = ( ((uint64_t)cycles_high << 32) | cycles_low );
		end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );

		send += end-start;
	}

	printf("Preparation of %d msges lasted: %lu clock cycles\n",
			NB_MSG, prepare);
	printf("Sending of %d msges lasted: \t %lu clock cycles \n",
			 NB_MSG, send );
	return EXIT_SUCCESS;
}


