#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#define MAX_PAYLOAD 1024 /* maximum payload size*/
#define MYMGRP 21
#define NB_MSG 100000

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

struct priv_msg {
	struct msghdr msg;
	struct nlmsghdr *nlh;
	struct nlmsgerr *err;
	struct iovec *iov;
};

/** Create socket with binding and set multicast opt. */
static int conn_init()
{
	int sock_fd;
	struct sockaddr_nl src_addr, dest_addr;
	int group = MYMGRP;
	int a = 212992;
	int enobuf = 1;

	sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
	if(sock_fd<0) {
		fprintf(stderr,"socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_groups = MYMGRP;
	src_addr.nl_pid = getpid(); /* self pid */
	if (bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) < 0) {
		fprintf(stderr,"binding failed");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(sock_fd, 270, NETLINK_ADD_MEMBERSHIP, &group,
				sizeof(group)) < 0) {
		fprintf(stderr,"setsockopt < 0\n");
		exit(EXIT_FAILURE);
	}

	if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &a, sizeof(a)) == -1) {
		fprintf(stderr,"increase buffer failed");
		exit(EXIT_FAILURE);
	}

	/* Don't ignore NOBUFS errors as return code of kernel sending */
	if (setsockopt(sock_fd, SOL_NETLINK, NETLINK_BROADCAST_ERROR, &enobuf,
				sizeof(enobuf)) == -1) {
		fprintf(stderr,"ENOBUFS option activation failed");
		exit(EXIT_FAILURE);
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0; /* For Linux Kernel */
	/*  Prevent remote address checking after every reception */
	if (connect(sock_fd,(struct sockaddr *) &dest_addr,
				sizeof(dest_addr)) == -1) {
		fprintf(stderr, "connect failed\n");
		exit(EXIT_FAILURE);
	}

	return sock_fd;
}

static struct priv_msg* prepare_error_msg()
{
	struct msghdr msgerr = {0};
	struct iovec *ioverr;
	struct nlmsghdr *nlherr = NULL;
	struct nlmsgerr *err;
	struct priv_msg  *priv_err;

	priv_err = malloc(sizeof(struct priv_msg));
	nlherr = malloc(NLMSG_SPACE(MAX_PAYLOAD));
	ioverr = malloc(sizeof(struct iovec));
	err = malloc(sizeof(struct nlmsgerr));
	memset(nlherr, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlherr->nlmsg_type = NLMSG_ERROR;
	ioverr->iov_base = (void *)nlherr;
	ioverr->iov_len = NLMSG_LENGTH(MAX_PAYLOAD);
	msgerr.msg_iov = ioverr;
	msgerr.msg_iovlen = 1; //1 message in iov
	err->error = 1; //error

	priv_err->msg = msgerr,
	priv_err->nlh = nlherr;
	priv_err->err = err;
	priv_err->iov = ioverr;

	return priv_err;
}

static struct priv_msg* prepare_sync_msg()
{
	struct msghdr msg = {0};
	struct iovec *iov;
	struct nlmsghdr *nlh = NULL;
	struct priv_msg *priv_sync;

	priv_sync = malloc(sizeof(struct priv_msg));
	nlh = malloc(NLMSG_SPACE(MAX_PAYLOAD));
	iov = malloc(sizeof(struct iovec));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	iov->iov_base = (void *)nlh;
	iov->iov_len = NLMSG_LENGTH(MAX_PAYLOAD);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1; //1 message in iov
	nlh->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;
	priv_sync->msg = msg;
	priv_sync->nlh = nlh;
	priv_sync->err = NULL;
	priv_sync->iov = iov;
}

static struct priv_msg* prepare_main_msg()
{
	struct msghdr msg = {0};
	struct iovec *iov;
	struct nlmsghdr *nlh = NULL;
	struct priv_msg *priv_main;

	priv_main = malloc(sizeof(struct priv_msg));
	nlh = malloc(NLMSG_SPACE(MAX_PAYLOAD));
	iov = malloc(sizeof(struct iovec));
	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	iov->iov_base = (void *)nlh;
	iov->iov_len = NLMSG_LENGTH(MAX_PAYLOAD);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1; //1 message in iov

	priv_main->msg = msg;
	priv_main->nlh = nlh;
	priv_main->err = NULL;
	priv_main->iov = iov;

	return priv_main;
}

/* Release priv_msg ressources */
void release_priv_msg(struct priv_msg *priv)
{
	free(priv->iov);
	priv->iov = NULL;
	free(priv->nlh);
	priv->nlh = NULL;
	free(priv->err);
	priv->err = NULL;
	free(priv);
	priv = NULL;

	return;
}

void send_err_msg(int fd, struct priv_msg *priv_main, struct priv_msg *priv_err)
{
	fprintf(stderr, "[seq=%d] send error msg to kernel\n",
			priv_main->nlh->nlmsg_seq);
	priv_err->nlh->nlmsg_seq;
	priv_err->err->msg = *priv_main->nlh;
	memcpy(NLMSG_DATA(priv_err->nlh), priv_err->err,
			sizeof(struct nlmsgerr));
	sendmsg(fd, &priv_err->msg, 0);
}

void send_sync_msg(int fd, struct priv_msg *priv_sync)
{
	fprintf(stderr, "send sync message\n");
	sendmsg(fd, &priv_sync->msg, 0);
}

static void netlink_bsd_receive(int sock_fd) {
	struct priv_msg *priv_err, *priv_main, *priv_sync;
	int res;
	int *data;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	uint64_t start, end;

	priv_main = prepare_main_msg();
	priv_err = prepare_error_msg();
	priv_sync = prepare_sync_msg();

	/* Receive from kernel */

	printf("Waiting for message from kernel\n");
	start_timer();
	while(priv_main->nlh->nlmsg_seq < NB_MSG - 1) {
		res = recvmsg(sock_fd, &priv_main->msg, MSG_ERRQUEUE);
		if (res == -1 && errno == ENOBUFS) {
			printf("[seq=%d] ENOBUF & res = -1\n",
					priv_main->nlh->nlmsg_seq);
			send_sync_msg(sock_fd, priv_sync);
			res = 0;
			errno = 0;
		} else if (res == -1) {
			printf("[seq=%d] ENOBUF & errno=%d\n",
					priv_main->nlh->nlmsg_seq, errno);
		} else if (res > 0) {
			//message has been received
			data = NLMSG_DATA(priv_main->nlh);
			printf("[pid=%d, seq=%d] %d\n",
				priv_main->nlh->nlmsg_pid,
				priv_main->nlh->nlmsg_seq, *data);
		}
	}
	stop_timer();

	printf("Finish with sequence number %d\n", priv_main->nlh->nlmsg_seq);

	start = ( ((uint64_t)cycles_high << 32) | cycles_low );
	end = ( ((uint64_t)cycles_high1 << 32) | cycles_low1 );
	printf("%lu clock cycles\n", (end-start));

	release_priv_msg(priv_err);
	release_priv_msg(priv_main);
}

/** Launch as root. */
int main()
{
	int sock_fd;

	sock_fd = conn_init();

	netlink_bsd_receive(sock_fd);

	close(sock_fd);
}
