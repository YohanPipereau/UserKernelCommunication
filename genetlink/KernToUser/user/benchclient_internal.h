#include "genlbench.h"

/* Create/Close socket */
struct nl_sock * bench_create_socket();
void bench_close_socket(struct nl_sock *socket);

/* Message receiving */
int recv_single_msg(struct nl_sock *socket);

/* Transactions */
int hsm_send_msg(struct nl_sock *socket, int family_id);

int ioc_transact(struct nl_sock *socket, const int family_id,
		 const unsigned int req, void *payload, int len);
int stats_transact(struct nl_sock *socket, const int family_id,
		   const unsigned int req);
