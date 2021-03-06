#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include "benchclient_internal.h"

/**
 * User space API for generic netlink.
 *
 * Sending:
 *   Default behaviour for sending is to send message and wait for an ACK for
 *   every message sent.
 *
 * Receiving:
 *   We determine which message has been received in recv_single_msg.
 *
 */

#define DEFAULT_MSG_LEN		1024

/* nla_policy structures defer between user space and kernel space */
/* attribute policy for STATS command */
static struct nla_policy bench_stats_attr_policy[BENCH_STATS_ATTR_MAX + 1] = {
	[STATS_REQUEST]	=	{.type = NLA_U32},
	[STATS_RAW] 	= 	{.type = NLA_BINARY},
};

/* attribute policy for IOC command */
static struct nla_policy bench_ioc_attr_policy[BENCH_IOC_ATTR_MAX + 1] = {
	[IOC_REQUEST]	=	{.type = NLA_U32},
	[IOC_RAW] 	= 	{.type = NLA_BINARY},
};

/* attribute policy for HSM command */
static struct nla_policy bench_hsm_attr_policy[BENCH_HSM_ATTR_MAX + 1] = {
	[HSM_MAGIC]		=	{.type = NLA_U16},
	[HSM_TRANSPORT]		= 	{.type = NLA_U8},
	[HSM_FLAGS]		= 	{.type = NLA_U8},
	[HSM_MSGTYPE]		= 	{.type = NLA_U16},
	[HSM_MSGLEN]		=	{.type = NLA_U16},
};

static struct nlattr**
bench_parse(struct nlmsghdr *rhdr, int cmd_max_attr, struct nla_policy* policy)
{
	struct nlattr **attrs;
	int rc;

	attrs = malloc(sizeof(struct nlattr)*cmd_max_attr);
	if(!attrs)
		return NULL;

	rc = genlmsg_parse(rhdr, 0, (void *)attrs, cmd_max_attr, policy);
	if (rc < 0) {
		fprintf(stderr, "parsing failed\n");
		free(attrs);
		attrs = NULL;
	}

	return attrs;
}

static inline void
display_sent_message(struct nlmsghdr *rhdr)
{
	fprintf(stderr,"SENT: [len=%d,type=%d,flags=%d,seq=%d]\n",
		rhdr->nlmsg_len, rhdr->nlmsg_type, rhdr->nlmsg_flags,
		rhdr->nlmsg_seq);
}

static inline void
display_ack_message(struct nlmsghdr *rhdr, int err)
{
	fprintf(stderr,"HDR: [len=%d,type=%d,flags=%d,seq=%d]\n",
		rhdr->nlmsg_len, rhdr->nlmsg_type, rhdr->nlmsg_flags,
		rhdr->nlmsg_seq);
	fprintf(stderr,"ACK PAY: [error=%s]\n", strerror(-err));
}

static void
stats_display_message(struct nlmsghdr *rhdr, int len)
{
	struct nlattr **attrs;

	attrs = bench_parse(rhdr, BENCH_STATS_ATTR_MAX,
			    bench_stats_attr_policy);
	if (!attrs)
		goto out;

	fprintf(stderr,"HDR: [len=%d,type=%d,flags=%d,seq=%d]\n", len,
		rhdr->nlmsg_type, rhdr->nlmsg_flags, rhdr->nlmsg_seq);
	fprintf(stderr,"MSG: []\n");

out:
	free(attrs);
}

static void
hsm_display_message(struct nlmsghdr *rhdr, int len)
{
	struct nlattr **attrs;

	attrs = bench_parse(rhdr, BENCH_HSM_ATTR_MAX, bench_hsm_attr_policy);
	if (!attrs)
		goto out;

	fprintf(stderr,"HDR : [len=%d,type=%d,flags=%d,seq=%d]\n", len,
		rhdr->nlmsg_type, rhdr->nlmsg_flags, rhdr->nlmsg_seq);
	fprintf(stderr,"MSG : []");

out:
	free(attrs);
}

/**
 * __bench_msg_alloc - Allocate message for transaction.
 * @param socket Generic netlink socket.
 * @param family_id ID of Generic netlink family.
 * @param
 */
static struct nl_msg *
__bench_msg_alloc(struct nl_sock *socket, const int family_id, const int cmd)
{
	struct nl_msg *msg;

	//TODO : This allocate PAGESIZE Bytes. Allocate for required length
	msg = nlmsg_alloc();
	if (!msg)
		return NULL;

	if (!genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id, 0, 0, cmd,
		    BENCH_GENL_VERSION)) {
		nlmsg_free(msg);
		return NULL;
	}

	return msg;
}

/**
 * ans_transact - Send request to Kernel and wait for kernel family answer.
 * @param socket Generic netlink socket.
 * @param msg Message to be sent.
 */
static int
ans_transact(struct nl_sock *socket, struct nl_msg *msg)
{
	int rc;

	rc = nl_send_auto(socket, msg);
	if (rc < 0) {
		fprintf(stderr, "fail sending message\n");
		nlmsg_free(msg);
		return rc;
	}

	display_sent_message(nlmsg_hdr(msg));

	rc = recv_single_msg(socket);
	if (rc < 0)
		fprintf(stderr, "Error reported on reception : %d\n", rc);

	nlmsg_free(msg);

	return rc;
}

/**
 * ack_transact - Send request to Kernel and wait for NLMSG_ERR response.
 * @param socket Generic netlink socket.
 * @param msg Message to be sent.
 */
static int
ack_transact(struct nl_sock *socket, struct nl_msg *msg)
{
	int rc;

	rc = nl_send_auto(socket, msg);
	if (rc < 0) {
		fprintf(stderr, "fail sending message\n");
		nlmsg_free(msg);
		return rc;
	}

	display_sent_message(nlmsg_hdr(msg));

	rc = recv_single_msg(socket);
	if (rc < 0)
		fprintf(stderr, "Error reported on reception : %d\n", rc);

	nlmsg_free(msg);

	return rc;
}

/**
 * ioc_transact - Send a IOC request on a Netlink socket and wait for kernel
 * response.
 * @param socket Generic netlink socket.
 * @param family_id ID of Generic netlink family.
 * @param req IOC request code.
 * @param payload Data you want to send to the kernel.
 * @param len Length of the payload provided.
 */
int ioc_transact(struct nl_sock *socket, const int family_id,
		 const unsigned int req, void *payload, int len)
{
	struct nl_msg *msg;
	int rc;

	msg = __bench_msg_alloc(socket, family_id, BENCH_CMD_IOC);
	if (!msg)
		return -ENOMEM;

	/* IOC attributes */
	rc = nla_put_u32(msg, IOC_REQUEST, req);
	if (rc < 0) {
		fprintf(stderr, "IOC attribute error\n");
		nlmsg_free(msg);
		return -EINVAL;
	}

	return ack_transact(socket, msg);
}

/**
 * stats_transact - Send a STATS request on a Netlink socket and wait for
 * kernel response
 * @param socket Generic netlink socket.
 * @param family_id ID of Generic netlink family.
 * @param req STATS request code.
 */
int stats_transact(struct nl_sock *socket, const int family_id,
		   const unsigned int req)
{
	struct nl_msg *msg;
	int rc;

	msg = __bench_msg_alloc(socket, family_id, BENCH_CMD_STATS);
	if (!msg)
		return -ENOMEM;

	/* IOC attributes */
	rc = nla_put_u32(msg, STATS_REQUEST, req);
	if (rc < 0) {
		fprintf(stderr, "IOC attribute error\n");
		nlmsg_free(msg);
		return -EINVAL;
	}

	return ans_transact(socket, msg);
}

/**
 * hsm_send_msg - Send an HSM message to trigger NLMSG_ERROR message.
 * @param socket Generic netlink socket.
 * @param family_id ID of Generic netlink family.
 */
int hsm_send_msg(struct nl_sock *socket, int family_id)
{
	struct nl_msg *msg;
	int rc;

	msg = __bench_msg_alloc(socket, family_id, BENCH_CMD_HSM);
	if (!msg)
		return -ENOMEM;

	/* Only one HSM attribute used to test */
	rc = nla_put_u16(msg, HSM_MAGIC, 10);
	if (rc < 0) {
		fprintf(stderr, "HSM attribute error");
		nlmsg_free(msg);
		return -EINVAL;
	}

	return ack_transact(socket, msg);
}

/**
 * recv_single_msg - Receive a single message.
 * @param socket Generic netlink socket.
 */
int recv_single_msg(struct nl_sock *socket)
{
	struct sockaddr_nl *nla = NULL;
	unsigned char *buf = NULL;
	struct nlmsghdr *rhdr; /* return message header */
	struct genlmsghdr *genlhdr;
	struct nlmsgerr *e;
	int rc;

	nla = malloc(sizeof(*nla));
	if (!nla) {
		rc = errno;
		goto out_failure;
	}

retry:
	rc = nl_recv(socket, nla, &buf, NULL); /*  allocate buf */
	if (rc == -ENOBUFS) //typically return ENOBUF
		goto retry;
	else if (rc < 0)
		goto out_failure;
	else if (rc == 0) {
		fprintf(stderr, "No more messages\n");
		goto out;
	}

	/* A message has been received */
	rhdr = (struct nlmsghdr *) buf;
	/* check if msg has been truncated */
	if (!nlmsg_ok(rhdr, rc)) {
		rc = -ENOBUFS;
		goto out_failure;
	}

	if (rhdr->nlmsg_type == NLMSG_ERROR) {
		e = nlmsg_data(rhdr);
		rc = e->error;
		display_ack_message(rhdr, rc);
		goto out;
	}

	genlhdr = genlmsg_hdr(rhdr);
	switch(genlhdr->cmd) {
	case BENCH_CMD_STATS:
		stats_display_message(rhdr, rc);
		break;
	case BENCH_CMD_IOC:
		fprintf(stderr, "IOC should only reply ERR MSG\n");
		break;
	case BENCH_CMD_HSM:
		hsm_display_message(rhdr, rc);
		break;
	default:
		fprintf(stderr, "Unknown message type\n");
		rc = -EOPNOTSUPP;
		goto out_failure;
	}

	goto out;

out_failure:
	perror("receiving message failed");
out:
	free(nla);
	free(buf);
	return rc;
}

/**
 * genl_close_socket - Close generic netlink socket.
 */
void bench_close_socket(struct nl_sock *socket)
{
	nl_close(socket);
	nl_socket_free(socket);
	socket = NULL;
}

/**
 * genl_create_socket - Create generic netlink socket.
 */
struct nl_sock * bench_create_socket()
{
	struct nl_sock *socket;
	int rc;

	/* Open socket to kernel */
	socket = nl_socket_alloc();
	rc = genl_connect(socket);
	if (rc < 0) {
		fprintf(stderr, "Fail connecting : %s\n", strerror(-rc));
		bench_close_socket(socket);
		return NULL;
	}

	return socket;
}
