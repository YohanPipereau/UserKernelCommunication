#include <assert.h>
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
#define SOL_NETLINK 270 /* XXX: fetch the value using getprotobyname */

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

int main(int argc, char *argv[])
{
	struct sockaddr_nl src_addr;
	struct nlmsghdr *nlh = NULL;
	struct msghdr msg = {0};
	struct iovec iov;
	int sock_fd;
	int group = MYMGRP;
        int seq;
	int rc;
	int a = 212992;
	unsigned int cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;

	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
	if (sock_fd < 0) {
		perror ("socket creation failed\n");
		return errno;
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_groups = MYMGRP;
	src_addr.nl_pid = getpid();

	rc = bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
        if (rc < 0) {
		perror("bind");
		return errno;
	}

	rc = setsockopt(sock_fd, SOL_NETLINK, NETLINK_ADD_MEMBERSHIP, &group,
                    sizeof(group));
        if (rc < 0) {
		perror("setsockopt: registering to a netlink group");
		return errno;
	}

	rc = setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &a, sizeof(a));
        if (rc < 0) {
		perror("setsockopt: increasing the reception buffer's size");
		return errno;
	}

	nlh = malloc(NLMSG_SPACE(MAX_PAYLOAD));
        if (!nlh) {
                fprintf(stderr, "allocating a netlink header: %s\n",
                                strerror(ENOMEM));
                return ENOMEM;
        }
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));

	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_LENGTH(MAX_PAYLOAD);
	msg.msg_iov = &iov;
        /* Store only one message in the iovec */
	msg.msg_iovlen = 1;

	printf("Waiting for messages from nlkern...\n");
	for (size_t seq = 0; seq < NB_MSG; seq++) {
		rc = recvmsg(sock_fd, &msg, 0);
		if (seq == 0)
			start_timer();
		if (rc == -1) {
            close(sock_fd);
			fprintf(stderr, "recvmsg failed after msg #%d\n", seq);
            return errno;
        }
        assert(seq == nlh->nlmsg_seq);
	}
	stop_timer();

	start = ((uint64_t)cycles_high << 32) | cycles_low;
	end = ((uint64_t)cycles_high1 << 32) | cycles_low1;
	printf("%lu clock cycles\n", end - start);

	close(sock_fd);
        return EXIT_SUCCESS;
}
