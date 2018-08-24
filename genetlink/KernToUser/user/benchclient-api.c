#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdio.h>

#include "benchclient_internal.h"

#define DEFAULT_MSG_LEN		1024

/**
 * hsm_send_msg - Send an HSM message to trigger NLMSG_ERROR message.
 * @param socket Generic netlink socket.
 * @param family_id ID of Generic netlink family.
 */
int hsm_send_msg(struct nl_sock *socket, int family_id)
{
	struct nl_msg *msg;
	void *payload;
	int rc;

	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;

	payload = genlmsg_put(msg, NL_AUTO_PORT, NL_AUTO_SEQ, family_id,
			      0, 0, BENCH_CMD_HSM, BENCH_GENL_VERSION);
	if (!payload)
		goto out_failure;

	/* Only one HSM attribute used to test */
	rc = nla_put_u16(msg, HSM_MAGIC, 10);
	if (rc < 0)
		goto out_failure;

	rc = nl_send_auto(socket, msg);
	if (rc < 0)
		goto out_failure;

	return 0;

out_failure:
	fprintf(stderr, "fail sending message");
	nlmsg_free(msg);
	return -ENOMEM; //TODO : right error code?
}

static inline void
display_error_message(struct nlmsghdr *rhdr, struct nlmsgerr *e, int len)
{
	fprintf(stderr,"[len=%d,type=%d,flags=%d,seq=%d] %s\n", len,
		rhdr->nlmsg_type, rhdr->nlmsg_flags, rhdr->nlmsg_seq,
	       	strerror(-e->error));
}

static inline void
stats_display_message(struct nlmsghdr *rhdr, unsigned char *msg, int len)
{
	fprintf(stderr,"[len=%d,type=%d,flags=%d,seq=%d] %s\n", len,
		rhdr->nlmsg_type, rhdr->nlmsg_flags, rhdr->nlmsg_seq, msg);
}

static void
ioc_display_message(struct nlmsghdr *rhdr, unsigned char *msg, int len)
{
	struct nlattr *attrs[BENCH_IOC_ATTR_MAX];

	if (nlmsg_parse(rhdr, BENCH_IOC_ATTR_SIZE, (void *)attrs,
			BENCH_IOC_ATTR_MAX, bench_ioc_attr_policy) < 0) {
		fprintf(stderr, "parsing failed\n");
		return;
	}
	fprintf(stderr,"[len=%d,type=%d,flags=%d,seq=%d] %s\n", len,
		rhdr->nlmsg_type, rhdr->nlmsg_flags, rhdr->nlmsg_seq, msg);
}

static void
hsm_display_message(struct nlmsghdr *rhdr, unsigned char *msg, int len)
{
	struct nlattr *attrs[BENCH_HSM_ATTR_MAX];

	if (nlmsg_parse(rhdr, BENCH_HSM_ATTR_SIZE, (void *)attrs,
			BENCH_HSM_ATTR_MAX, bench_hsm_attr_policy) < 0) {
		fprintf(stderr, "parsing failed\n");
		return;
	}
	fprintf(stderr,"[len=%d,type=%d,flags=%d,seq=%d] %s\n", len,
		rhdr->nlmsg_type, rhdr->nlmsg_flags, rhdr->nlmsg_seq, msg);
}

/**
 * recv_single_msg - Receive a signle message.
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

	//TODO : change this. We want to read message size first then allocate
	//accordingly.
	buf = malloc(DEFAULT_MSG_LEN);
	if (!buf) {
		rc = errno;
		goto out_failure;
	}

retry:
	rc = nl_recv(socket, nla, &buf, NULL);
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
		display_error_message(rhdr, e, rc);
		return rc;
	}

	genlhdr = genlmsg_hdr(rhdr);
	switch(genlhdr->cmd) {
	case BENCH_CMD_STATS:
		stats_display_message(rhdr, buf, rc);
		break;
	case BENCH_CMD_IOC:
		ioc_display_message(rhdr, buf, rc);
		break;
	case BENCH_CMD_HSM:
		hsm_display_message(rhdr, buf, rc);
		break;
	default:
		fprintf(stderr, "Unknown message type\n");
		rc = -EOPNOTSUPP;
		goto out_failure;
	}

	free(nla);
	free(buf);

	return rc;

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
void genl_close_socket(struct nl_sock *socket)
{
	nl_close(socket);
}
