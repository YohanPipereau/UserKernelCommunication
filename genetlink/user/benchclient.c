#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
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
	struct ioc_example_struct *ex_struct;

	ex_struct = malloc(sizeof(struct ioc_example_struct));
	if (!ex_struct)
		return;
	ex_struct->example_int = 1;
	ex_struct->example_short = 2;

	rc = ioc_transact(socket, family_id, IOC_REQUEST_EXAMPLE, &ex_struct,
			  sizeof(*ex_struct));
	if (rc < 0)
		return;

	free(ex_struct);
	assert(rc == 0);
}

/* test3 - Send STATS request and check that transaction is handled correctly */
static void test3(struct nl_sock *socket, const int family_id)
{
	int rc;

	rc = stats_transact(socket, family_id, STATS_REQUEST_EXAMPLE);
	if (rc < 0)
		return;

	assert(rc == 0);
}

/* Test4 - Send a bunch of message */
static void test4(struct nl_sock *socket, const int family_id)
{
	int rc;
	int i;

	for (i = 0; i < 100; i++) {
		rc = stats_transact(socket, family_id, STATS_REQUEST_EXAMPLE);
		if (rc < 0)
			return;
		assert(rc == 0);
	}
}

/* Test5 - Multi processus sending */
//TODO : treat the problem of sequence number
static void test5(struct nl_sock *socket, const int family_id)
{
	pid_t pid;
	int status;
	int rc;

	pid = fork();
	if (pid == 0) { //child
		rc = stats_transact(socket, family_id, STATS_REQUEST_EXAMPLE);
		if (rc < 0)
			return;
		assert(rc == 0);
		exit(0);
	} else if (pid > 0) { //parent
		rc = stats_transact(socket, family_id, IOC_REQUEST_EXAMPLE);
		if (rc < 0)
			return;
		wait(&status);
		assert(rc == 0);
	}
}


int main()
{
	struct nl_sock *socket;
	int family_id;

	socket = bench_create_socket();
	if (!socket)
		return -ENOMEM;

	family_id = genl_ctrl_resolve(socket, BENCH_GENL_FAMILY_NAME);
	if (family_id < 0) {
		fprintf(stderr, "Fail resolving module : %s\n",
			strerror(-family_id));
		bench_close_socket(socket);
		return family_id;
	};

	printf("** Test1: Unsupported HSM request **\n");
	test1(socket, family_id);
	printf("Test1: OK\n\n");
	printf("** Test2: Test simple IOC request **\n");
	test2(socket, family_id);
	printf("Test2: OK\n\n");
	printf("** Test3: Test simple STATS request **\n");
	test3(socket, family_id);
	printf("Test3: OK\n\n");
	printf("** Test4: Send multiple STATS messages **\n");
	test4(socket, family_id);
	printf("Test4: OK\n\n");
	printf("** Test5: Multi processus message sending **\n");
	test5(socket, family_id);
	printf("Test5: OK\n\n");

	bench_close_socket(socket);

	return 0;
}
