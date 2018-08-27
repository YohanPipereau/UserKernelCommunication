#include "genlbench.h"


int hsm_send_msg(struct nl_sock *socket, int family_id);
int recv_single_msg(struct nl_sock *socket);
void genl_close_socket(struct nl_sock *socket);
int ioc_transact(struct nl_sock *socket, int family_id, void *payload, int len);
