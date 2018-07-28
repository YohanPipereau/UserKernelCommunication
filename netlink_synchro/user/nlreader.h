#define MAX_PAYLOAD 1024 /* maximum payload size*/
#define MYMGRP 21
#define NB_MSG 100000

#ifndef SOL_NETLINK
	#define SOL_NETLINK 270
#endif

#define start_timer() asm volatile ("CPUID\n\t" \
"RDTSC\n\t" \
"mov %%edx, %0\n\t" \
"mov %%eax, %1\n\t": "=r" (cycles_high), \
"=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");

#define stop_timer() asm volatile("RDTSCP\n\t" \
"mov %%edx, %0\n\t" \
"mov %%eax, %1\n\t" \
"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: \
"%rax", "%rbx", "%rcx", "%rdx");

struct priv_msg {
	struct msghdr msg;
	struct nlmsghdr *nlh;
	struct nlmsgerr *err;
	struct iovec *iov;
};

static int conn_init();

static struct priv_msg* prepare_error_msg();
static struct priv_msg* prepare_msg(__u16 flags);

void release_priv_msg(struct priv_msg *priv);

static int send_err_msg(int fd, struct priv_msg *priv_main, struct priv_msg *priv_err);
static int send_sync_msg(int fd, struct priv_msg *priv_sync);

static void netlink_bsd_receive(int sock_fd);
