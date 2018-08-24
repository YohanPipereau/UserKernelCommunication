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

static inline
void display_error_message(struct nlmsghdr *rhdr, struct nlmsgerr *e, int len)
{
	fprintf(stderr,"[len=%d,type=%d,flags=%d,seq=%d] %s\n", len,
		rhdr->nlmsg_type, rhdr->nlmsg_flags, rhdr->nlmsg_seq,
	       	strerror(-e->error));
}

static
void hsm_display_message(struct nlmsghdr *rhdr, unsigned char *msg, int len)
{
	fprintf(stderr,"[len=%d,type=%d,flags=%d,seq=%d] %s\n", len,
		rhdr->nlmsg_type, rhdr->nlmsg_flags, rhdr->nlmsg_seq, msg);
}

/**
 * recv_msg - Receive a message.
 * @param socket Generic netlink socket.
 */
int recv_msg(struct nl_sock *socket)
{
	struct sockaddr_nl *nla = NULL;
	unsigned char *buf = NULL;
	struct nlmsghdr *rhdr; /* return message header */
	struct nlmsgerr *e;
	struct nlattr *attrs[BENCH_HSM_ATTR_MAX];
	int rc;

	nla = malloc(sizeof(*nla));
	if (!nla) {
		perror("nla alloc failed");
		rc = errno;
		goto out_failure;
	}

	//TODO : change this. We want to read message size first then allocate
	//accordingly.
	buf = malloc(DEFAULT_MSG_LEN);
	if (!buf) {
		perror("buff alloc failed");
		rc = errno;
		goto out_failure;
	}

	rc = nl_recv(socket, nla, &buf, NULL);
	if (rc < 0) {
		goto out_failure;
	} else if (rc == 0) {
		fprintf(stderr, "No more messages\n");
		goto out;
	}

	/* A message has been received */
	rhdr = (struct nlmsghdr *) buf;

	while (nlmsg_ok(rhdr, rc)) {
		if (rhdr->nlmsg_type == NLMSG_ERROR) {
			e = nlmsg_data(rhdr);
			display_error_message(rhdr, e, rc);
			rhdr = nlmsg_next(rhdr, &rc);
			continue;
		}

		rc = nlmsg_parse(rhdr, BENCH_HSM_ATTR_SIZE, (void *)attrs,
				 BENCH_HSM_ATTR_MAX, bench_hsm_attr_policy);
		if (rc < 0) {
			fprintf(stderr, "parsing failed : %s", strerror(rc));
			rhdr = nlmsg_next(rhdr, &rc);
			continue;
		}

		hsm_display_message(rhdr, buf, rc);
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
static void genl_close_socket(struct nl_sock *socket)
{
	nl_close(socket);
}

int main()
{
	struct nl_sock *socket;
	int family_id;
	int rc;

	/* Open socket to kernel */
	socket = nl_socket_alloc();
	rc = genl_connect(socket);
	if (rc < 0) {
		fprintf(stderr, "Fail connecting : %s\n", strerror(-rc));
		return rc;
	}

	family_id = genl_ctrl_resolve(socket, BENCH_GENL_FAMILY_NAME);
	if (family_id < 0) {
		fprintf(stderr, "Fail resolving module : %s\n",
			strerror(-family_id));
		return family_id;
	};

	rc = hsm_send_msg(socket, family_id);
	if (rc < 0)
		return rc;

	rc = recv_msg(socket);
	if (rc < 0)
		return rc;

	genl_close_socket(socket);

	return 0;
}

