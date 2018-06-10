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
	struct sockaddr_nl src_addr, dest_addr;
	struct nlmsghdr *nlh = NULL;
	int seq = 0;
	int res;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;

	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
	if(sock_fd<0) {
		perror ("socket creation failed\n");
		return errno;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid(); /* self pid */

	if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0) {
		perror("binding failed");
		return errno;
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0; /* For Linux Kernel */

	/*  This removes the need for the remote address to be explicitly
	 *  checked every time a datagram is received */
	if (connect(sock_fd,(struct sockaddr *) &dest_addr,
				sizeof(dest_addr)) == -1) {
		perror("connect failed\n");
		return errno;
	}

	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlh->nlmsg_len = NLMSG_LENGTH(MAX_PAYLOAD);
	nlh->nlmsg_pid = getpid();
	nlh->nlmsg_flags = 0;

	printf("Type message to send\n");
	while(1) {
		fgets((char*) NLMSG_DATA(nlh), MAX_PAYLOAD, stdin);
		nlh->nlmsg_seq = seq;
		seq ++;
		iov.iov_base = (void *)nlh;
		iov.iov_len = nlh->nlmsg_len;
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;
		start_timer();
		res = sendmsg(sock_fd,&msg,0);
		stop_timer()
		if (res == -1) {
			perror("sendmsg failed");
			return errno;
		}
		start = ( ((uint64_t)cycles_high << 32) | cycles_low );
		end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
		printf("sending time %lu \n", end-start);
	}
	free(nlh);
	nlh = NULL;
	close(sock_fd);
}
