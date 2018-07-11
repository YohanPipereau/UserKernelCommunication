/*
 *    Description:  userland part in charge of creating pipe and receving data
 *        Created:  09/07/2018
 *         Author:  Yohan Pipereau
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#define NB_MSG 10000
#define MAXSIZE 1024-sizeof(struct msg_hdr)
#define FILENAME "/dev/pipe_misc"
#define IOCTL_WRITE 0x2121

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

/* struct imported from Lustre project */
struct msg_hdr {
	int msglen;
	int seq;
};

static inline char *msg_data(const struct msg_hdr *msg)
{
	return (char*) msg + sizeof(struct msg_hdr);
}

int main ( int argc, char *argv[] )
{
	int pipefd[2];
	int misc_fd;
	struct msg_hdr *msg;
	int rc;
	char *buf;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;
	char trigger[] = "send";


	buf = malloc(MAXSIZE);

	if (pipe(pipefd) < 0)
		return errno;

	misc_fd = open(FILENAME, O_WRONLY);
	if (misc_fd < 0)
		return errno;

	rc = ioctl(misc_fd, IOCTL_WRITE, &pipefd[1]);
	if (rc < 0) {
		perror("failed ioctl");
		return errno;
	}

	//trigger the reception
	rc = write(misc_fd, trigger, strlen(trigger));
	if (rc < 0) {
		perror("failed writing");
		return errno;
	}

	start_timer();
	do {
		/*  Read header */
		rc = read(pipefd[0], buf, 1000+sizeof(struct msg_hdr));
		if (rc < 0) {
			perror("read failed");
			return errno;
		}

		msg = (struct msg_hdr*) buf;

		/* Message can't be bigger than maxsize */
		if (msg->msglen > MAXSIZE)
			return -EMSGSIZE;

		//printf("[seq=%d] %s\n", msg->seq, msg_data(msg));
	} while (msg->seq < NB_MSG-1);
	stop_timer();

	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printf("%lu clock cycles\n", (end-start));

	close(misc_fd);
	close(pipefd[0]);
	close(pipefd[1]);

	return EXIT_SUCCESS;
}


