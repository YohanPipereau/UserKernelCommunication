#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>

#include "benchclient_internal.h"

/* test1 - Send HSM request despite Kernel does not implement reception
 * operation
 * @param socket Generic netlink socket.
 * @param family_id ID of Generic netlink family.
 */
static void test1(struct nl_sock *socket, int family_id)
{
	int rc;

	rc = hsm_send_msg(socket, family_id);
	if (rc < 0)
		return;

	rc = recv_single_msg(socket);
	if (rc < 0)
		return;

	assert(rc == -EOPNOTSUPP);
}

/* test2 - Send IOC request and check that transaction is handled correctly */
static void test2(struct nl_sock *socket, int family_id)
{
	int rc;
	struct ioc_example_struct {
		int	example_int;
		short	example_short;
	} ex_struct = { 44, 44};

	rc = ioc_transact(socket, family_id, &ex_struct, sizeof(ex_struct));
	if (rc < 0)
		return;

	assert(rc == 0);
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

	test1(socket, family_id);
	test2(socket, family_id);

	genl_close_socket(socket);

	return 0;
}
