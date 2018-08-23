#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>

#define FAMILY_NAME "bench"

int hsm_send_msg()
{
}

int hsm_recv_msg()
{
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

	family_id = genl_ctrl_resolve(socket, FAMILY_NAME);
	if (family_id < 0) {
		fprintf(stderr, "Fail resolving module : %s\n",
			strerror(-family_id));
		return family_id;
	};

	return 0;
}

