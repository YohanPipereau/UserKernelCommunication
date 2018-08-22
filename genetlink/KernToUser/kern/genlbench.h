#define BENCH_ATTR_MAX		3 		/* arbitrary */
#define BENCH_GENL_FAMILY_NAME	"bench"
#define BENCH_GENL_VERSION 	0x1

/* Configuration policy attributes, used for STATS */
enum {
	STATS_TREE,
	__BENCH_STATS_ATTR_MAX,
};
#define BENCH_STATS_ATTR_MAX (__BENCH_STATS_ATTR_MAX - 1)

/* Configuration policy attributes, used for IOC */
enum {
	REQUEST,
	ARGS,
	__BENCH_IOC_ATTR_MAX,
};
#define BENCH_IOC_ATTR_MAX (__BENCH_IOC_ATTR_MAX - 1)

/* Configuration policy attributes, used for  HSM */
enum {
	MAGIC,
	TRANSPORT,
	FLAGS,
	MSGTYPE,
	MSGLEN,
	__BENCH_HSM_ATTR_MAX,
};
#define BENCH_HSM_ATTR_MAX (__BENCH_HSM_ATTR_MAX - 1)
#define BENCH_MAX_ATTR BENCH_HSM_ATTR_MAX

/* Available commands for Lustre */
enum {
	BENCH_CMD_STATS,
	BENCH_CMD_IOCTL,
	BENCH_CMD_HSM,
	__BENCH_TYPE_MAX,
};
#define BENCH_TYPE_MAX (__BENCH_TYPE_MAX -1)

/* Send unicast message */
static int genlbench_stats_unicast(char *msg, int len, int pid);
static int genlbench_ioc_unicast(char *msg, int len, int pid);
static int genlbench_hsm_unicast(char *msg, int len, int pid);

/* Callbacks for command reception */
//TODO : Should stats be pulled by userland process when needed only???
static int genlbench_stats_echo(struct sk_buff *skb, struct genl_info *info);
static int genlbench_ioc_echo(struct sk_buff *skb, struct genl_info *info);
static int genlbench_hsm_recv(struct sk_buff *skb, struct genl_info *info);
