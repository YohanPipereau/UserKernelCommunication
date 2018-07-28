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

#include "nlreader.h"

/** Create socket with binding and set multicast opt. */
static int conn_init()
{
	struct sockaddr_nl src_addr, dest_addr;
	int sock_size = 212992;
	int group = MYMGRP;
	int enobuf = 1;
	int sock_fd;
	int rc;

	sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USERSOCK);
	if(sock_fd < 0)
		goto out_fail;

	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_groups = MYMGRP;
	src_addr.nl_pid = getpid(); /* self pid */

	rc = bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
	if (rc < 0)
		goto out_fail;

	rc = setsockopt(sock_fd, 270, NETLINK_ADD_MEMBERSHIP, &group,
			sizeof(group));
	if(rc < 0)
		goto out_fail;

	rc = setsockopt(sock_fd, SOL_SOCKET, SO_RCVBUF, &sock_size,
			sizeof(sock_size));
	if (rc < 0)
		goto out_fail;

	/* Do not ignore ENOBUF error after kernel send */
	rc = setsockopt(sock_fd, SOL_NETLINK, NETLINK_BROADCAST_ERROR, &enobuf,
			sizeof(enobuf));
	if (rc < 0)
		goto out_fail;

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0; /* For Linux Kernel */
	/*  Prevent remote address checking after every reception */
	rc = connect(sock_fd,(struct sockaddr *) &dest_addr, sizeof(dest_addr));
	if (rc < 0)
		goto out_fail;

	return sock_fd;

out_fail:
	fprintf(stderr,"Init socket failed");
	return errno;
}

static struct priv_msg* prepare_error_msg()
{
	struct priv_msg *priv_err = NULL;
	struct nlmsghdr *nlherr = NULL;
	struct iovec *ioverr = NULL;
	struct nlmsgerr *err = NULL;
	struct msghdr msgerr = {0};

	priv_err = malloc(sizeof(struct priv_msg));
	if (!priv_err)
		goto free_mem;

	nlherr = malloc(NLMSG_SPACE(MAX_PAYLOAD));
	if (!nlherr)
		goto free_mem;

	ioverr = malloc(sizeof(struct iovec));
	if (!ioverr)
		goto free_mem;

	err = malloc(sizeof(struct nlmsgerr));
	if (!err)
		goto free_mem;

	memset(nlherr, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nlherr->nlmsg_type = NLMSG_ERROR;
	ioverr->iov_base = nlherr;
	ioverr->iov_len = NLMSG_LENGTH(MAX_PAYLOAD);
	msgerr.msg_iov = ioverr;
	msgerr.msg_iovlen = 1; //1 message in iov
	err->error = 1; //error

	priv_err->msg = msgerr,
	priv_err->nlh = nlherr;
	priv_err->err = err;
	priv_err->iov = ioverr;

	return priv_err;

free_mem:
	free(priv_err);
	free(nlherr);
	free(ioverr);
	free(err);
	exit(EXIT_FAILURE);
}

static struct priv_msg* prepare_msg(__u16 flags)
{
	struct priv_msg *priv = NULL;
	struct nlmsghdr *nlh = NULL;
	struct msghdr msg = {0};
	struct iovec *iov;

	priv = malloc(sizeof(struct priv_msg));
	if (!priv)
		goto free_mem;

	nlh = malloc(NLMSG_SPACE(MAX_PAYLOAD));
	if (!nlh)
		goto free_mem;

	iov = malloc(sizeof(struct iovec));
	if (!iov)
		goto free_mem;

	memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
	iov->iov_base = nlh;
	iov->iov_len = NLMSG_LENGTH(MAX_PAYLOAD);
	msg.msg_iov = iov;
	msg.msg_iovlen = 1; //1 message in iov
	nlh->nlmsg_flags = flags;

	priv->msg = msg;
	priv->nlh = nlh;
	priv->err = NULL;
	priv->iov = iov;

	return priv;

free_mem:
	free(priv);
	free(nlh);
	free(iov);
	exit(EXIT_FAILURE);
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

static int send_err_msg(int fd, struct priv_msg *priv_main, struct priv_msg *priv_err)
{
	fprintf(stderr, "[seq=%d] send error msg to kernel\n",
		priv_main->nlh->nlmsg_seq);

	//priv_err->nlh->nlmsg_seq;
	priv_err->err->msg = *priv_main->nlh;
	memcpy(NLMSG_DATA(priv_err->nlh), priv_err->err,
	       sizeof(struct nlmsgerr));

	return sendmsg(fd, &priv_err->msg, 0);
}

static int send_sync_msg(int fd, struct priv_msg *priv_sync)
{
	fprintf(stderr, "send sync message\n");

	return sendmsg(fd, &priv_sync->msg, 0);
}

static void netlink_bsd_receive(int sock_fd) {
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	struct priv_msg *priv_err, *priv_main, *priv_sync;
	uint64_t start, end;
	int *data;
	int rc;

	priv_main = prepare_msg(0);
	priv_sync = prepare_msg(NLM_F_DUMP | NLM_F_REQUEST);

	/* Receive from kernel */
	printf("Waiting for message from kernel\n");

	start_timer();
	while(priv_main->nlh->nlmsg_seq < NB_MSG - 1) {
		rc = recvmsg(sock_fd, &priv_main->msg, MSG_ERRQUEUE);
		if (rc < 0 && errno == ENOBUFS) {
			printf("[seq=%d] ENOBUF & rc = -1\n",
			       priv_main->nlh->nlmsg_seq);
			send_sync_msg(sock_fd, priv_sync);
			rc = 0;
			//errno = 0;
		} else if (rc == -1) {
			printf("[seq=%d] ENOBUF & errno=%d\n",
			       priv_main->nlh->nlmsg_seq, errno);
		} else if (rc > 0) {
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
