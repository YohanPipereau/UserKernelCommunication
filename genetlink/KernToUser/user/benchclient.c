#include <stdio.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "benchclient_internal.h"

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

	rc = recv_single_msg(socket);
	if (rc < 0)
		return rc;

	genl_close_socket(socket);

	return 0;
}

