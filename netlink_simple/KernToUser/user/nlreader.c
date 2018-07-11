#include <sys/socket.h>
#include <unistd.h>
#include <linux/netlink.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#define MAX_PAYLOAD 1024 /* maximum payload size*/
#define MYMGRP 21
#define NB_MSG 1024

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

int main()
{
	struct msghdr msg = {0}; //must be initialized to 0
	int sock_fd;
	struct iovec iov;
	struct sockaddr_nl src_addr;
	struct nlmsghdr *nlh = NULL;
	int group = MYMGRP;
	int res;
	int a = 212992;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;

	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
	if(sock_fd<0) {
		perror ("socket creation failed\n");
		return errno;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_groups = MYMGRP;
	src_addr.nl_pid = getpid(); /* self pid */

	if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0) {
		perror("binding failed");
		return errno;
	}

	if (setsockopt(sock_fd, 270, NETLINK_ADD_MEMBERSHIP, &group,
				sizeof(group)) < 0) {
		perror("setsockopt < 0\n");
		return -1;
	}

	if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &a, sizeof(a)) == -1) {
		perror("increase buffer failed");
		return errno;
	}

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));

	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_LENGTH(MAX_PAYLOAD);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1; //1 message in iov

	printf("Waiting for message from kernel\n");

	while(nlh->nlmsg_seq < NB_MSG - 1) {
		/* Read message from kernel */
		res = recvmsg(sock_fd, &msg, 0);
		if (nlh->nlmsg_seq == 0)
			start_timer();
		if (res == -1) {
			fprintf(stderr, "%d recv failed\n", nlh->nlmsg_seq);
		}
		/* printf("[pid=%d, seq=%d] %s %d\n", nlh->nlmsg_pid,
		   nlh->nlmsg_seq, (char *)NLMSG_DATA(nlh), res); */
	}
	stop_timer();

	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printf("%lu clock cycles\n", (end-start));

	printf("Finish with sequence number %d\n", nlh->nlmsg_seq);
	close(sock_fd);
}
